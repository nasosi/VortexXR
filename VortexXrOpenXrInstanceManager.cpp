// VortexXR
// Copyright (C) 2026  Athanasios Iliopoulos

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.

// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "VortexXrOpenXrInstanceManager.h"
#include "VortexXrMessageDispatch.h"

#include <TSData.h>

#include <C4Json.h>


namespace VortexXr
{
    static const char EnterTerminatorMsg[] = "Enter Terminator.";
    static const char ExitTerminatorMsg[]  = "Exit Terminator.";
    static const char EnterTrampolineMsg[] = "Enter Trampoline.";
    static const char ExitTrampolineMsg[]  = "Exit Trampoline.";

    //
    namespace Json = C4::Json;


    struct ApiLayer
    {
            String<>                     name;
            Holder<DynamicLibrary>       library;
            Array<XrExtensionProperties> extensionPropertiesArray;
            PFN_xrGetInstanceProcAddr    xrGetInstanceProcAddr;
            PFN_xrCreateApiLayerInstance xrcreateApiLayerInstance;
    };


    struct RuntimeDescriptor
    {
            String<> libraryPath;
            String<> name;
    };

#define OPENXR_RUNTIME_DESCRIPTOR_PROTO "library_path", object.libraryPath, "name", object.name
    DEFINE_JSON4C4_FUNCTIONS( RuntimeDescriptor, OPENXR_RUNTIME_DESCRIPTOR_PROTO )


    struct OpenXrRuntimeManifest
    {
            String<>          fileFormatVersion;
            RuntimeDescriptor runtimeDescriptor;
    };

#define OPENXR_MANIFEST_DESCRIPTOR_PROTO "file_format_version", object.fileFormatVersion, "runtime", object.runtimeDescriptor
    DEFINE_JSON4C4_FUNCTIONS( OpenXrRuntimeManifest, OPENXR_MANIFEST_DESCRIPTOR_PROTO )

    struct ApiLayerInstanceExtensionDescriptor
    {
            String<> name;
            String<> extensionVersion;
    };
#define OPENXR_API_LATER_INSTANCE_EXTENSION_DESCRIPTOR_PROTO "name", object.name, "extension_version", object.extensionVersion
    DEFINE_JSON4C4_FUNCTIONS( ApiLayerInstanceExtensionDescriptor, OPENXR_API_LATER_INSTANCE_EXTENSION_DESCRIPTOR_PROTO )

    struct ApiLayerDescriptor
    {
            String<>                                   libraryPath;
            String<>                                   name;
            String<>                                   apiVersion;
            String<>                                   implementationVersion;
            String<>                                   description;
            Json::ObjectMap<String<>>                  functionMap;
            Array<ApiLayerInstanceExtensionDescriptor> instanceExtensionArray;
            String<>                                   enableEnvironment;
            String<>                                   disableEnvironment;

            XrVersion                                  xrApiVersion = 0;
    };

#define OPENXR_API_LAYER_DESCRIPTOR_PROTO                                                                                                                                                              \
    "library_path", object.libraryPath, "name", object.name, "api_version", object.apiVersion, "implementation_version", object.implementationVersion, "description", object.description, "functions", \
        object.functionMap, Json::optional, "instance_extensions", object.instanceExtensionArray, Json::optional, "enable_environment", object.enableEnvironment, Json::optional,                      \
        "disable_environment", object.disableEnvironment
    DEFINE_JSON4C4_FUNCTIONS( ApiLayerDescriptor, OPENXR_API_LAYER_DESCRIPTOR_PROTO )


    struct OpenXrApiLayerManifest
    {
            String<>                     fileFormatVersion;
            ApiLayerDescriptor           apiLayerDescriptor;
            String<>                     parentPath;

            Array<XrExtensionProperties> GetXrExtensionPropertiesArray( )
            {
                Array<XrExtensionProperties> xrExtensionPropertiesArray;

                for ( const auto& instanceExtension : apiLayerDescriptor.instanceExtensionArray )
                {
                    XrExtensionProperties xrExtensionProperties;
                    xrExtensionProperties.type = XR_TYPE_EXTENSION_PROPERTIES;

                    Text::CopyText( instanceExtension.name, xrExtensionProperties.extensionName, XR_MAX_EXTENSION_NAME_SIZE - 1 );

                    xrExtensionProperties.extensionVersion = Data::StringToInt32( instanceExtension.extensionVersion );
                    xrExtensionPropertiesArray.AppendArrayElement( xrExtensionProperties );
                }

                return xrExtensionPropertiesArray;
            }
    };

#define OPENXR_API_LAYER_MANIFEST_PROTO "file_format_version", object.fileFormatVersion, "api_layer", object.apiLayerDescriptor
    DEFINE_JSON4C4_FUNCTIONS( OpenXrApiLayerManifest, OPENXR_API_LAYER_MANIFEST_PROTO )


    static XrResult PopulateOpenXrRuntimeManifestArray( const char* functionNameToReport, Array<OpenXrRuntimeManifest>& outOpenXrManifestArray )
    {
        Array<String<>> activeRuntimeJsonPathArray;
        XrResult        xrResult = GetActiveRuntimeJsonPathArray( activeRuntimeJsonPathArray );

        for ( const String<>& activeRuntimeJsonPath : activeRuntimeJsonPathArray )
        {
            Json::StructuredData jsonStructuredData;
            Json::ParseResult    parseResult = jsonStructuredData.Parse( activeRuntimeJsonPath );

            if ( parseResult.status != Json::Status::kOk )
            {

                Log::Error( functionNameToReport, FormatString( "Manifest file parse error: File: %, Line: %, Column %: %", activeRuntimeJsonPath, parseResult.errorLine, parseResult.errorColumn,
                                                                Json::StatusToString( parseResult.status ) ) );

                continue;
            }

            OpenXrRuntimeManifest openXrManifest;

            if ( Json::Validate( jsonStructuredData, openXrManifest ) != Json::Status::kOk )
            {
                Log::Error( functionNameToReport, FormatString( "Manifest file JSON validation error. File: %", activeRuntimeJsonPath ) );

                continue;
            }

            if ( jsonStructuredData.DeserializeTo( openXrManifest ) != Json::Status::kOk )
            {
                Log::Error( functionNameToReport, FormatString( "Manifest file deserialization. File: %", activeRuntimeJsonPath ) );

                continue;
            }

            openXrManifest.runtimeDescriptor.libraryPath = GetParentPath( activeRuntimeJsonPath ) + openXrManifest.runtimeDescriptor.libraryPath;

            outOpenXrManifestArray.AppendArrayElement( openXrManifest );
        }

        return outOpenXrManifestArray.GetArrayElementCount( ) == 0 ? XR_ERROR_RUNTIME_UNAVAILABLE : XR_SUCCESS;
    }


    static XrResult PopulateOpenXrApiLayerManifestArrays( const char*                            functionNameToReport,
                                                          Array<Holder<OpenXrApiLayerManifest>>& openXrImplicitApiLayerManifestArray,
                                                          Array<Holder<OpenXrApiLayerManifest>>& openEnvironmentVariableXrApiLayerManifestArray,
                                                          Array<Holder<OpenXrApiLayerManifest>>& openXrExplicitApiLayerManifestArray ) // Pointer beacuse Terathon::Map is not copyable
    {
        // Load API layers
        Array<String<>> implicitApiLayerManifestFileArray;
        Array<String<>> explicitApiLayerManifestFileArray;
        Array<String<>> environmentVariableApiLayerManifestFileArray;

        XrResult        result = GetApiLayerPathArray( functionNameToReport, implicitApiLayerManifestFileArray, explicitApiLayerManifestFileArray, environmentVariableApiLayerManifestFileArray );

        if ( XR_FAILED( result ) )
        {
            return result;
        }

        auto appendApiLayerManifest = [ &functionNameToReport ]( const Array<String<>>& apiLayerManifestFileArray, Array<Holder<OpenXrApiLayerManifest>>& openXrApiLayerManifestArray )
        {
            for ( const String<>& apiLayerManifestFileName : apiLayerManifestFileArray )
            {
                Json::StructuredData jsonStructuredData;
                Json::ParseResult    parseResult = jsonStructuredData.Parse( apiLayerManifestFileName );

                if ( parseResult.status != Json::Status::kOk )
                {
                    Log::Error( functionNameToReport, FormatString( "Manifest file parse error: File: %, Line: %, Column %: %", apiLayerManifestFileName, parseResult.errorLine,
                                                                    parseResult.errorColumn, Json::StatusToString( parseResult.status ) ) );

                    continue;
                }

                OpenXrApiLayerManifest* apiLayerManifest = new OpenXrApiLayerManifest;

                if ( Json::Validate( jsonStructuredData, *apiLayerManifest ) != Json::Status::kOk )
                {
                    Log::Error( functionNameToReport, FormatString( "Manifest file JSON validation error. File: %", apiLayerManifestFileName ) );

                    delete apiLayerManifest;

                    continue;
                }

                if ( jsonStructuredData.DeserializeTo( *apiLayerManifest ) != Json::Status::kOk )
                {
                    Log::Error( functionNameToReport, "Api layer Manifest Deserialization error." );

                    delete apiLayerManifest;

                    continue;
                }

                if ( apiLayerManifest->apiLayerDescriptor.disableEnvironment.GetStringLength( ) == 0 )
                {
                    Log::Error( functionNameToReport, FormatString( "'disable_environment' variable missing in the json manifest file: %.", apiLayerManifestFileName ) );

                    delete apiLayerManifest;

                    continue;
                }

                Array<String<>> version;
                SplitString( apiLayerManifest->apiLayerDescriptor.apiVersion, '.', version );

                if ( version.GetArrayElementCount( ) != 2 )
                {
                    Log::Error( functionNameToReport, FormatString( "Api layer Manifest structure validation error for file %.", apiLayerManifestFileName ) );

                    delete apiLayerManifest;

                    continue;
                }

                const char* a     = static_cast<const char*>( version[ 0 ] );
                const char* b     = static_cast<const char*>( version[ 1 ] );
                int32_t     major = 0;
                int32_t     minor = 0;

                if ( Int32DataType::ParseValue( a, &major ) != kDataOkay || Int32DataType::ParseValue( b, &minor ) != kDataOkay )
                {
                    Log::Error( functionNameToReport, FormatString( "Api layer Manifest structure validation error: Invalid Api version format in file %", apiLayerManifestFileName ) );

                    delete apiLayerManifest;

                    continue;
                }

                apiLayerManifest->apiLayerDescriptor.xrApiVersion = XR_MAKE_VERSION( major, minor, 0 );

                apiLayerManifest->parentPath                      = GetParentPath( apiLayerManifestFileName );
                openXrApiLayerManifestArray.AppendArrayElement( apiLayerManifest );
            }
        };

        appendApiLayerManifest( implicitApiLayerManifestFileArray, openXrImplicitApiLayerManifestArray );
        appendApiLayerManifest( environmentVariableApiLayerManifestFileArray, openEnvironmentVariableXrApiLayerManifestArray );
        appendApiLayerManifest( explicitApiLayerManifestFileArray, openXrExplicitApiLayerManifestArray );

        return XR_SUCCESS;
    }

    // Terminators ---

    static XRAPI_ATTR XrResult XRAPI_CALL xrCreateInstance_Terminator( const char* functionNameToReport, const XrInstanceCreateInfo* createInfo, XrInstance* instance )
    {
        if ( XrResult result = TheOpenXrInstanceManager.EnsureIsInitialized( functionNameToReport ); XR_FAILED( result ) )
        {
            return result;
        }

        return TheOpenXrInstanceManager.GetOpenXrRuntimeManager( ).CreateXrInstance( functionNameToReport, createInfo, instance );
    }

    static XRAPI_ATTR XrResult XRAPI_CALL xrDestroyInstance_Terminator( XrInstance instance )
    {
        const char* functionNameToReport = "xrDestroyInstance";

        if ( !TheOpenXrInstanceManager.IsInitialized( ) )
        {
            return XR_ERROR_RUNTIME_UNAVAILABLE;
        }

        XrResult result = TheOpenXrInstanceManager.GetOpenXrRuntimeManager( ).DestroyXrInstance( functionNameToReport, instance );

        if ( XR_FAILED( result ) )
        {
            return result;
        }

        return result;
    }


    static XrResult XRAPI_CALL xrCreateApiLayerInstance_Terminator( const XrInstanceCreateInfo* createInfo, const struct XrApiLayerCreateInfo*, XrInstance* xrInstance )
    {
        return xrCreateInstance_Terminator( "xrCreateApiLayerInstance", createInfo, xrInstance );
    }


    static XRAPI_ATTR XrResult XRAPI_CALL xrSetDebugUtilsObjectNameEXT_Terminator( XrInstance instance, const XrDebugUtilsObjectNameInfoEXT* nameInfo )
    {
        if ( !TheOpenXrInstanceManager.IsInitialized( ) )
        {
            return XR_ERROR_RUNTIME_UNAVAILABLE;
        }

        return TheOpenXrInstanceManager.xrSetDebugUtilsObjectNameEXT_Terminator( instance, nameInfo );
    }

    static XRAPI_ATTR XrResult XRAPI_CALL xrSubmitDebugUtilsMessageEXT_Terminator( XrInstance                                  instance,
                                                                                   XrDebugUtilsMessageSeverityFlagsEXT         messageSeverity,
                                                                                   XrDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                                                                   const XrDebugUtilsMessengerCallbackDataEXT* callbackData )
    {
        if ( !TheOpenXrInstanceManager.IsInitialized( ) )
        {
            return XR_ERROR_RUNTIME_UNAVAILABLE;
        }

        return TheOpenXrInstanceManager.xrSubmitDebugUtilsMessageEXT_Terminator( instance, messageSeverity, messageTypes, callbackData );
    }

    static XRAPI_ATTR XrResult XRAPI_CALL xrCreateDebugUtilsMessengerEXT_Terminator( XrInstance instance, const XrDebugUtilsMessengerCreateInfoEXT* createInfo, XrDebugUtilsMessengerEXT* messenger )
    {
        if ( !TheOpenXrInstanceManager.IsInitialized( ) )
        {
            return XR_ERROR_RUNTIME_UNAVAILABLE;
        }

        return TheOpenXrInstanceManager.xrCreateDebugUtilsMessengerEXT_Terminator( instance, createInfo, messenger );
    }

    static XRAPI_ATTR XrResult XRAPI_CALL xrDestroyDebugUtilsMessengerEXT_Terminator( XrDebugUtilsMessengerEXT messenger )
    {
        if ( !TheOpenXrInstanceManager.IsInitialized( ) )
        {
            return XR_ERROR_RUNTIME_UNAVAILABLE;
        }

        return TheOpenXrInstanceManager.xrDestroyDebugUtilsMessengerEXT_Terminator( messenger );
    }

    static XRAPI_ATTR XrResult XRAPI_CALL xrGetInstanceProcAddr_Terminator( XrInstance xrInstance, const char* functionName, PFN_xrVoidFunction* terminatorFunction )
    {
        // Note: Here we are not checking if xrInstance is valid. This will be checked by the terminator or the runtime.

        auto IsFunctionName = [ &functionName ]( const char* comp )
        {
            return VortexXr::CompareFunctionName( functionName, comp );
        };

        auto SetTerminatorFunctionIfNameIs = [ &terminatorFunction, &IsFunctionName ]( const char* comp, auto pfn )
        {
            if ( IsFunctionName( comp ) )
            {
                *terminatorFunction = reinterpret_cast<PFN_xrVoidFunction>( pfn );

                return true;
            }
            return false;
        };

        if ( SetTerminatorFunctionIfNameIs( "xrCreateInstance", xrCreateInstance_Terminator ) )
        {
            return XR_SUCCESS;
        }

        if ( SetTerminatorFunctionIfNameIs( "xrDestroyInstance", xrDestroyInstance_Terminator ) )
        {
            return XR_SUCCESS;
        }

        if ( SetTerminatorFunctionIfNameIs( "xrCreateApiLayerInstance", xrCreateApiLayerInstance_Terminator ) )
        {
            return XR_SUCCESS;
        }

        if ( SetTerminatorFunctionIfNameIs( "xrGetInstanceProcAddr", xrGetInstanceProcAddr_Terminator ) )
        {
            return XR_SUCCESS;
        }

        if ( SetTerminatorFunctionIfNameIs( "xrCreateDebugUtilsMessengerEXT", xrCreateDebugUtilsMessengerEXT_Terminator ) )
        {
            return XR_SUCCESS;
        }

        if ( SetTerminatorFunctionIfNameIs( "xrDestroyDebugUtilsMessengerEXT", xrDestroyDebugUtilsMessengerEXT_Terminator ) )
        {
            return XR_SUCCESS;
        }

        if ( SetTerminatorFunctionIfNameIs( "xrSubmitDebugUtilsMessageEXT", xrSubmitDebugUtilsMessageEXT_Terminator ) )
        {
            return XR_SUCCESS;
        }

        if ( SetTerminatorFunctionIfNameIs( "xrSetDebugUtilsObjectNameEXT", xrSetDebugUtilsObjectNameEXT_Terminator ) )
        {
            return XR_SUCCESS;
        }

        if ( XrResult result = TheOpenXrInstanceManager.EnsureIsInitialized( "xrGetInstanceProcAddr" ); XR_FAILED( result ) )
        {
            return result;
        }

        PFN_xrGetInstanceProcAddr runtimeGetInstanceProcAddr = TheOpenXrInstanceManager.GetOpenXrRuntimeManager( ).GetXrGetInstanceProcAddr( );

        return runtimeGetInstanceProcAddr( xrInstance, functionName, terminatorFunction );
    }


    // Trampolines ---

    static XRAPI_ATTR XrResult XRAPI_CALL xrSetDebugUtilsObjectNameEXT_Trampoline( XrInstance instance, const XrDebugUtilsObjectNameInfoEXT* nameInfo )
    {
        if ( !TheOpenXrInstanceManager.IsInitialized( ) )
        {
            Log::Error( "xrSetDebugUtilsObjectNameEXT", "There is no active XrInstance." );

            return XR_ERROR_RUNTIME_UNAVAILABLE;
        }

        return TheOpenXrInstanceManager.xrSetDebugUtilsObjectNameEXT_Trampoline( instance, nameInfo );
    }

    static XRAPI_ATTR XrResult XRAPI_CALL xrSubmitDebugUtilsMessageEXT_Trampoline( XrInstance                                  instance,
                                                                                   XrDebugUtilsMessageSeverityFlagsEXT         messageSeverity,
                                                                                   XrDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                                                                   const XrDebugUtilsMessengerCallbackDataEXT* callbackData )
    {
        if ( !TheOpenXrInstanceManager.IsInitialized( ) )
        {
            Log::Error( "xrSubmitDebugUtilsMessageEXT_Trampoline", "There is no active XrInstance." );

            return XR_ERROR_RUNTIME_UNAVAILABLE;
        }

        return TheOpenXrInstanceManager.xrSubmitDebugUtilsMessageEXT_Trampoline( instance, messageSeverity, messageTypes, callbackData );
    }

    static XRAPI_ATTR XrResult XRAPI_CALL xrCreateDebugUtilsMessengerEXT_Trampoline( XrInstance instance, const XrDebugUtilsMessengerCreateInfoEXT* createInfo, XrDebugUtilsMessengerEXT* messenger )
    {
        if ( !TheOpenXrInstanceManager.IsInitialized( ) )
        {
            Log::Error( "xrCreateDebugUtilsMessengerEXT", "There is no active XrInstance." );

            return XR_ERROR_RUNTIME_UNAVAILABLE;
        }

        return TheOpenXrInstanceManager.xrCreateDebugUtilsMessengerEXT_Trampoline( instance, createInfo, messenger );
    }

    static XRAPI_ATTR XrResult XRAPI_CALL xrDestroyDebugUtilsMessengerEXT_Trampoline( XrDebugUtilsMessengerEXT messenger )
    {
        if ( !TheOpenXrInstanceManager.IsInitialized( ) )
        {
            Log::Error( "xrDestroyDebugUtilsMessengerEXT", "There is no active XrInstance." );

            return XR_ERROR_RUNTIME_UNAVAILABLE;
        }

        return TheOpenXrInstanceManager.xrDestroyDebugUtilsMessengerEXT_Trampoline( messenger );
    }

    static XRAPI_ATTR XrResult XRAPI_CALL xrSessionInsertDebugUtilsLabelEXT_Trampoline( XrSession session, const XrDebugUtilsLabelEXT* labelInfo )
    {
        if ( !TheOpenXrInstanceManager.IsInitialized( ) )
        {
            Log::Error( "xrSessionInsertDebugUtilsLabelEXT", "There is no active XrInstance." );

            return XR_ERROR_RUNTIME_UNAVAILABLE;
        }

        return TheOpenXrInstanceManager.xrSessionInsertDebugUtilsLabelEXT_Trampoline( session, labelInfo );
    }

    static XRAPI_ATTR XrResult XRAPI_CALL xrSessionBeginDebugUtilsLabelRegionEXT_Trampoline( XrSession session, const XrDebugUtilsLabelEXT* labelInfo )
    {
        if ( !TheOpenXrInstanceManager.IsInitialized( ) )
        {
            return XR_ERROR_RUNTIME_UNAVAILABLE;
        }

        return TheOpenXrInstanceManager.xrSessionBeginDebugUtilsLabelRegionEXT_Trampoline( session, labelInfo );
    }

    static XRAPI_ATTR XrResult XRAPI_CALL xrSessionEndDebugUtilsLabelRegionEXT_Trampoline( XrSession session )
    {
        if ( !TheOpenXrInstanceManager.IsInitialized( ) )
        {
            return XR_ERROR_RUNTIME_UNAVAILABLE;
        }

        return TheOpenXrInstanceManager.xrSessionEndDebugUtilsLabelRegionEXT_Trampoline( session );
    }


    XrResult OpenXrLoader::Initialize( const char* functionNameToReport )
    {
        Log::Verbose( functionNameToReport, "Creating OpenXR Instance Manager." );

        Array<OpenXrRuntimeManifest> openXrManifestArray;
        XrResult                     xrResult = PopulateOpenXrRuntimeManifestArray( functionNameToReport, openXrManifestArray );

        if ( XR_FAILED( xrResult ) )
        {
            Log::Error( functionNameToReport, "Could not locate an active openxr runtime." );

            return xrResult;
        }

        XrResult runtimeLoadResult = XR_ERROR_RUNTIME_UNAVAILABLE;

        // Load the first valid runtime
        for ( const auto& openXrManifest : openXrManifestArray )
        {
            Log::Verbose( functionNameToReport, FormatString( "Loading OpenXR Runtime Library: %, Location: %", openXrManifest.runtimeDescriptor.name, openXrManifest.runtimeDescriptor.libraryPath ) );

            runtimeLoadResult = this->openXrRuntime.Load( functionNameToReport, openXrManifest.runtimeDescriptor.libraryPath );

            if ( XR_SUCCEEDED( runtimeLoadResult ) )
            {
                break;
            }
        }

        if ( XR_SUCCEEDED( runtimeLoadResult ) )
        {
            Log::Info( functionNameToReport, "OpenXR loader creation successful." );

            return XR_SUCCESS;
        }

        Log::Error( functionNameToReport, "Could not load an OpenXR runtime library." );

        return XR_ERROR_RUNTIME_UNAVAILABLE;
    }


    OpenXrLoader::OpenXrLoader( )
    {
        XrExtensionProperties debugUtilsExtensionProperties;
        debugUtilsExtensionProperties.type = XR_TYPE_EXTENSION_PROPERTIES;
        debugUtilsExtensionProperties.next = nullptr;

        Text::CopyText( XR_EXT_DEBUG_UTILS_EXTENSION_NAME, debugUtilsExtensionProperties.extensionName, XR_MAX_EXTENSION_NAME_SIZE );

        debugUtilsExtensionProperties.extensionVersion = XR_EXT_debug_utils_SPEC_VERSION;

        loaderSupportedExtensionPropertiesArray.AppendArrayElement( debugUtilsExtensionProperties );
    }

    OpenXrLoader::~OpenXrLoader( )
    {
        Log::Verbose( "OpenXrInstanceManager::~OpenXrInstanceManager( )", "Destroying OpenXR loader." );

        if ( IsXrInstanceActive( ) )
        {
            xrDestroyInstance( activeXrInstance );
        }
    }

    bool OpenXrLoader::IsInitialized( )
    {
        return openXrRuntime.IsRuntimeValid( );
    }

    XrResult OpenXrLoader::EnsureIsInitialized( const char* functionNameToReport )
    {
        if ( this->IsInitialized( ) )
        {
            return XR_SUCCESS;
        }

        return this->Initialize( functionNameToReport );
    }

    bool OpenXrLoader::IsXrInstanceActive( )
    {
        return activeXrInstance != XR_NULL_HANDLE;
    }

    const XrInstance& OpenXrLoader::GetActiveXrInstance( ) const
    {
        return activeXrInstance;
    }

    OpenXrRuntimeManager& OpenXrLoader::GetOpenXrRuntimeManager( )
    {
        return openXrRuntime;
    }


    bool OpenXrLoader::LoadAndAppendApiLayerLibraries( const char* callingXrFunctionName, const Array<Holder<OpenXrApiLayerManifest>>& apiLayerManifestArray )
    {
        for ( OpenXrApiLayerManifest* openXrApiLayerManifest : apiLayerManifestArray )
        {
            String<>        fullFilePath    = openXrApiLayerManifest->parentPath + "\\" + openXrApiLayerManifest->apiLayerDescriptor.libraryPath;
            DynamicLibrary* apiLayerLibrary = new DynamicLibrary( fullFilePath );
            String<>        apiLayerName    = openXrApiLayerManifest->apiLayerDescriptor.name;

            if ( !apiLayerLibrary->IsInitialized( ) )
            {
                String<> fallbackFilePath = openXrApiLayerManifest->apiLayerDescriptor.libraryPath;

                delete apiLayerLibrary;

                apiLayerLibrary = new DynamicLibrary( fallbackFilePath );

                if ( !apiLayerLibrary->IsInitialized( ) )
                {
                    Log::Warning( callingXrFunctionName, FormatString( "Api Library %, could not be found in either of the following paths: %, %.", apiLayerName, fullFilePath, fallbackFilePath ) );

                    delete apiLayerLibrary;

                    continue;
                }
            }

            Log::Verbose( callingXrFunctionName, FormatString( "Loaded Api library: %", apiLayerName ) );

            auto*    mapElement                         = openXrApiLayerManifest->apiLayerDescriptor.functionMap.FindMapElement( "xrNegotiateLoaderApiLayerInterface" );
            String<> negotiateFunctionName              = mapElement ? mapElement->data : String<>( "xrNegotiateLoaderApiLayerInterface" );
            auto     xrNegotiateLoaderApiLayerInterface = apiLayerLibrary->GetProcedureAddress<PFN_xrNegotiateLoaderApiLayerInterface>( negotiateFunctionName );

            if ( xrNegotiateLoaderApiLayerInterface == nullptr )
            {
                Log::Verbose( callingXrFunctionName, FormatString( "Could not get the xrNegotiateLoaderRuntimeInterface function address for Api layer %.", apiLayerName ) );

                delete apiLayerLibrary;

                continue;
            }

            Log::Verbose( callingXrFunctionName, "Negotiate function found" );

            XrNegotiateLoaderInfo negotiateLoaderInfo { };
            negotiateLoaderInfo.structType          = XR_LOADER_INTERFACE_STRUCT_LOADER_INFO;
            negotiateLoaderInfo.structVersion       = XR_LOADER_INFO_STRUCT_VERSION;
            negotiateLoaderInfo.structSize          = sizeof( XrNegotiateLoaderInfo );
            negotiateLoaderInfo.minInterfaceVersion = 1;
            negotiateLoaderInfo.maxInterfaceVersion = XR_CURRENT_LOADER_API_LAYER_VERSION;
            negotiateLoaderInfo.minApiVersion       = XR_MAKE_VERSION( 1, 0, 0 );
            negotiateLoaderInfo.maxApiVersion       = this->maxApiVersion;

            XrNegotiateApiLayerRequest negotiateApiLayerRequest { };
            negotiateApiLayerRequest.structType    = XR_LOADER_INTERFACE_STRUCT_API_LAYER_REQUEST;
            negotiateApiLayerRequest.structVersion = XR_API_LAYER_INFO_STRUCT_VERSION;
            negotiateApiLayerRequest.structSize    = sizeof( XrNegotiateApiLayerRequest );

            //
            XrResult result = xrNegotiateLoaderApiLayerInterface( &negotiateLoaderInfo, static_cast<const char*>( apiLayerName ), &negotiateApiLayerRequest );

            if ( XR_FAILED( result ) )
            {
                Log::Error( callingXrFunctionName, FormatString( "Negotiation with Api layer % failed with error: %.", apiLayerName, result ) );

                delete apiLayerLibrary;

                continue;
            }

            if ( negotiateApiLayerRequest.getInstanceProcAddr == nullptr )
            {
                Log::Error( callingXrFunctionName, FormatString( "Api layer % did not provide a valid getInstanceProcAddr", apiLayerName ) );

                delete apiLayerLibrary;

                continue;
            }

            auto apiLayerMajorVersion = XR_VERSION_MAJOR( negotiateApiLayerRequest.layerApiVersion );
            auto apiLayerMinorVersion = XR_VERSION_MINOR( negotiateApiLayerRequest.layerApiVersion );

            Log::Verbose( callingXrFunctionName, FormatString( "Api layer % initialized. OpenXR Api version: %.%. Interface version: %.", apiLayerName, apiLayerMajorVersion, apiLayerMinorVersion,
                                                               negotiateApiLayerRequest.layerInterfaceVersion ) );

            auto xrExtensionPropertiesArray = openXrApiLayerManifest->GetXrExtensionPropertiesArray( );

            apiLayerArray.AppendArrayElement(
                new ApiLayer { apiLayerName, apiLayerLibrary, xrExtensionPropertiesArray, negotiateApiLayerRequest.getInstanceProcAddr, negotiateApiLayerRequest.createApiLayerInstance } );
        }

        return true;
    }


    void OpenXrLoader::Uninitialize( )
    {
        SharedMutexExclusiveLocker locker( this->mutex );
        Uninitialize_AssumesLocked( );
    }

    void OpenXrLoader::Uninitialize_AssumesLocked( )
    {
        if ( this->defaultDebugUtilsMessenger != nullptr )
        {
            this->xrDestroyDebugUtilsMessengerEXT_Trampoline( this->defaultDebugUtilsMessenger );
            this->defaultDebugUtilsMessenger = nullptr;
        }

        {
            Holder<CoreXrFunctionPointerList> destroyer( Move( xrFunctionPointerList ) );
        }

        this->apiLayerArray.PurgeArray( );
        this->enabledExtensionsExtensionPropertiesArray.PurgeArray( );
        this->topmostGetInstanceProcAddr = nullptr;
        this->activeXrInstance           = XR_NULL_HANDLE;

        this->messageDispatcher.Uninitialize( );
        this->openXrRuntime.Unload( );
    }

    XrResult OpenXrLoader::xrCreateInstance( const XrInstanceCreateInfo* createInfo, XrInstance* instance )
    {
        static const char* functionNameToReport = "xrCreateInstance";

        Log::Verbose( functionNameToReport, "Called" );

        if ( createInfo == nullptr )
        {
            Log::Error( functionNameToReport, "createInfo argument should not be null." );

            return XR_ERROR_VALIDATION_FAILURE;
        }

        if ( instance == nullptr )
        {
            Log::Error( functionNameToReport, "xrInstance argument should not be null." );

            return XR_ERROR_VALIDATION_FAILURE;
        }

        if ( createInfo->enabledApiLayerCount > 0 && createInfo->enabledApiLayerNames == nullptr )
        {
            Log::Error( functionNameToReport, "createInfo->enabledApiLayerNames argument cannot be a null pointer if createInfo->enabledApiLayerCount is larger than 0." );

            return XR_ERROR_VALIDATION_FAILURE;
        }

        if ( createInfo->enabledExtensionCount > 0 && createInfo->enabledExtensionNames == nullptr )
        {
            Log::Error( functionNameToReport, "createInfo->enabledExtensionNames argument cannot be a null pointer if createInfo->enabledExtensionCount is larger than 0." );

            return XR_ERROR_VALIDATION_FAILURE;
        }

        auto appSemanticVersion = XR_MAKE_VERSION( XR_VERSION_MAJOR( createInfo->applicationInfo.apiVersion ), XR_VERSION_MINOR( createInfo->applicationInfo.apiVersion ), 0 );

        if ( appSemanticVersion > OpenXrLoader::apiSemanticVersion )
        {
            Log::Error( functionNameToReport, FormatString( "The requested OpenXR API version (%.%) is higher than the maximum supported version (%.%).", XR_VERSION_MAJOR( appSemanticVersion ),
                                                            XR_VERSION_MINOR( appSemanticVersion ), OpenXrLoader::apiMajorVersion, OpenXrLoader::apiMinorVersion ) );

            return XR_ERROR_API_VERSION_UNSUPPORTED;
        }


        auto CreateDefaultDebugUtilsMessenger = [ & ]( )
        {
            const auto*                               next                          = reinterpret_cast<const XrBaseInStructure*>( createInfo->next );
            const XrDebugUtilsMessengerCreateInfoEXT* debugUtilsMessengerCreateInfo = nullptr;

            while ( next != nullptr )
            {
                if ( next->type == XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT )
                {
                    Log::Info( functionNameToReport, "next chain of createInfo contains an XrDebugUtilsMessengerCreateInfoEXT. Creating default DebugUtilsMessenger." );

                    XrDebugUtilsMessengerEXT messenger;

                    debugUtilsMessengerCreateInfo = reinterpret_cast<const XrDebugUtilsMessengerCreateInfoEXT*>( next );

                    XrResult result               = this->xrCreateDebugUtilsMessengerEXT_Trampoline( activeXrInstance, debugUtilsMessengerCreateInfo, &messenger );

                    if ( XR_FAILED( result ) )
                    {
                        return result;
                    }

                    this->defaultDebugUtilsMessenger = messenger;

                    break;
                }
                next = reinterpret_cast<const XrBaseInStructure*>( next->next );
            }

            return XR_SUCCESS;
        };

        SharedMutexExclusiveLocker xrLoaderSharedMutexExclusiveLocker( this->mutex );

        if ( this->IsXrInstanceActive( ) )
        {
            Log::Error( functionNameToReport, "An XrInstance is already active and having more than one active is not allowed." );

            return XR_ERROR_LIMIT_REACHED;
        }


        if ( XrResult result = this->EnsureIsInitialized( functionNameToReport ); XR_FAILED( result ) )
        {
            this->Uninitialize_AssumesLocked( );

            return result;
        }

        Array<Holder<OpenXrApiLayerManifest>> implicitApiLayerManifestArray;
        Array<Holder<OpenXrApiLayerManifest>> environmentVariableApiLayerManifestArray;
        Array<Holder<OpenXrApiLayerManifest>> explicitApiLayerManifestArray;
        Array<Holder<OpenXrApiLayerManifest>> enabledExplicitApiLayerManifestArray;

        XrResult result = PopulateOpenXrApiLayerManifestArrays( functionNameToReport, implicitApiLayerManifestArray, environmentVariableApiLayerManifestArray, explicitApiLayerManifestArray );

        bool     atLeastOneLayerWasNotFound = false;

        for ( uint32 rId = 0; rId != createInfo->enabledApiLayerCount; rId++ )
        {
            bool  layerWasFound = false;
            int32 exId          = 0;

            while ( exId < explicitApiLayerManifestArray.GetArrayElementCount( ) )
            {
                if ( Text::CompareText( createInfo->enabledApiLayerNames[ rId ], explicitApiLayerManifestArray[ exId ]->apiLayerDescriptor.name, XR_MAX_API_LAYER_NAME_SIZE ) )
                {
                    enabledExplicitApiLayerManifestArray.AppendArrayElement( Move( explicitApiLayerManifestArray[ exId ] ) );
                    explicitApiLayerManifestArray.RemoveArrayElement( exId );

                    layerWasFound = true;

                    break;
                }

                exId++;
            }

            if ( !layerWasFound )
            {
                Log::Error( functionNameToReport, FormatString( "Requested Explicit API layer % was not found in the list of explicit layers.", createInfo->enabledApiLayerNames[ rId ] ) );
                atLeastOneLayerWasNotFound = true;
            }
        }

        if ( atLeastOneLayerWasNotFound )
        {
            this->Uninitialize_AssumesLocked( );

            return XR_ERROR_API_LAYER_NOT_PRESENT;
        }

        this->LoadAndAppendApiLayerLibraries( functionNameToReport, implicitApiLayerManifestArray );
        this->LoadAndAppendApiLayerLibraries( functionNameToReport, environmentVariableApiLayerManifestArray );
        this->LoadAndAppendApiLayerLibraries( functionNameToReport, enabledExplicitApiLayerManifestArray );

        Array<const char*> finalEnabledExtensionNameArray;

        this->enabledExtensionsExtensionPropertiesArray.PurgeArray( );

        for ( uint32 a = 0; a != createInfo->enabledExtensionCount; a++ )
        {
            bool        extensionFound       = false;
            const char* enabledExtensionName = createInfo->enabledExtensionNames[ a ];

            if ( IsExtensionEnabled( enabledExtensionName ) )
            { // In case the user has provided the same extension twice
                continue;
            }

            for ( const auto& runtimeExtensionProperties : this->openXrRuntime.GetXrExtensionPropertiesArray( ) )
            {
                if ( Text::CompareText( runtimeExtensionProperties.extensionName, enabledExtensionName, XR_MAX_EXTENSION_NAME_SIZE ) )
                {
                    this->enabledExtensionsExtensionPropertiesArray.AppendArrayElement( runtimeExtensionProperties );
                    finalEnabledExtensionNameArray.AppendArrayElement( runtimeExtensionProperties.extensionName );

                    extensionFound = true;

                    break;
                }
            }

            if ( extensionFound )
            {
                continue;
            }

            for ( const auto& loaderSupportedExtensionProperties : this->loaderSupportedExtensionPropertiesArray )
            {
                if ( Text::CompareText( loaderSupportedExtensionProperties.extensionName, enabledExtensionName, XR_MAX_EXTENSION_NAME_SIZE ) )
                {
                    this->enabledExtensionsExtensionPropertiesArray.AppendArrayElement( loaderSupportedExtensionProperties );

                    extensionFound = true;

                    break;
                }
            }

            if ( extensionFound )
            {
                continue;
            }

            for ( const auto& apiLayer : this->apiLayerArray )
            {
                for ( const auto& apiLayerExtensionProperties : apiLayer->extensionPropertiesArray )
                {
                    if ( Text::CompareText( apiLayerExtensionProperties.extensionName, enabledExtensionName, XR_MAX_EXTENSION_NAME_SIZE ) )
                    {
                        this->enabledExtensionsExtensionPropertiesArray.AppendArrayElement( apiLayerExtensionProperties );
                        finalEnabledExtensionNameArray.AppendArrayElement( apiLayerExtensionProperties.extensionName );

                        extensionFound = true;

                        break;
                    }
                }
            }

            if ( !extensionFound )
            {
                Log::Warning( functionNameToReport, FormatString( "Requested extension % was not found.", createInfo->enabledExtensionNames[ a ] ) );

                this->Uninitialize_AssumesLocked( );

                return XR_ERROR_EXTENSION_NOT_PRESENT;
            }
        }

        XrInstanceCreateInfo finalXrInstanceCreateInfo { *createInfo };
        finalXrInstanceCreateInfo.enabledExtensionCount = finalEnabledExtensionNameArray.GetArrayElementCount( );
        finalXrInstanceCreateInfo.enabledExtensionNames = finalEnabledExtensionNameArray.begin( );

        const Array<Holder<ApiLayer>>& apiLayerArray    = this->apiLayerArray;

        if ( apiLayerArray.GetArrayElementCount( ) == 0 )
        {
            result = xrCreateInstance_Terminator( functionNameToReport, &finalXrInstanceCreateInfo, instance );

            if ( XR_FAILED( result ) )
            {
                Log::Error( functionNameToReport, "xrCreateInstance failed." );

                this->Uninitialize_AssumesLocked( );

                return result;
            }

            topmostGetInstanceProcAddr = xrGetInstanceProcAddr_Terminator;
            this->activeXrInstance     = *instance;

            Holder<CoreXrFunctionPointerList> destroyer( Move( this->xrFunctionPointerList ) );

            xrFunctionPointerList = new CoreXrFunctionPointerList( this->activeXrInstance, this->topmostGetInstanceProcAddr );

            result                = CreateDefaultDebugUtilsMessenger( );

            if ( XR_FAILED( result ) )
            {
                this->Uninitialize_AssumesLocked( );
            }

            return result;
        }

        Array<XrApiLayerNextInfo> apiLayerNextInfoArray;
        apiLayerNextInfoArray.SetArrayElementCount( apiLayerArray.GetArrayElementCount( ) );

        uint32 a = apiLayerArray.GetArrayElementCount( );
        do
        {
            a--;

            XrApiLayerNextInfo& apiLayerNextInfo = apiLayerNextInfoArray[ a ];
            apiLayerNextInfo.structType          = XR_LOADER_INTERFACE_STRUCT_API_LAYER_NEXT_INFO;
            apiLayerNextInfo.structVersion       = XR_API_LAYER_INFO_STRUCT_VERSION;
            apiLayerNextInfo.structSize          = sizeof( XrApiLayerNextInfo );

            Text::CopyText( static_cast<const char*>( apiLayerArray[ a ]->name ), apiLayerNextInfo.layerName, XR_MAX_API_LAYER_NAME_SIZE - 1 );

            if ( a != apiLayerArray.GetArrayElementCount( ) - 1 )
            {
                XrApiLayerNextInfo* nextApiLayerNextInfo    = &apiLayerNextInfoArray[ a + 1 ];
                const ApiLayer&     nextApiLayer            = *apiLayerArray[ a + 1 ];

                apiLayerNextInfo.next                       = nextApiLayerNextInfo;
                apiLayerNextInfo.nextGetInstanceProcAddr    = nextApiLayer.xrGetInstanceProcAddr;
                apiLayerNextInfo.nextCreateApiLayerInstance = nextApiLayer.xrcreateApiLayerInstance;
            }
            else
            { // Terminator
                apiLayerNextInfo.next                       = nullptr;
                apiLayerNextInfo.nextGetInstanceProcAddr    = xrGetInstanceProcAddr_Terminator;
                apiLayerNextInfo.nextCreateApiLayerInstance = xrCreateApiLayerInstance_Terminator;
            }

        } while ( a != 0 );

        XrApiLayerCreateInfo apiLayerCreateInfo;
        apiLayerCreateInfo.structType                  = XR_LOADER_INTERFACE_STRUCT_API_LAYER_CREATE_INFO;
        apiLayerCreateInfo.structVersion               = XR_API_LAYER_CREATE_INFO_STRUCT_VERSION;
        apiLayerCreateInfo.structSize                  = sizeof( XrApiLayerCreateInfo );
        apiLayerCreateInfo.loaderInstance              = nullptr;
        apiLayerCreateInfo.settings_file_location[ 0 ] = 0;
        apiLayerCreateInfo.nextInfo                    = apiLayerNextInfoArray.begin( );

        XrInstance tmpInstance                         = XR_NULL_HANDLE;
        result                                         = apiLayerArray[ 0 ]->xrcreateApiLayerInstance( &finalXrInstanceCreateInfo, &apiLayerCreateInfo, &tmpInstance );

        if ( XR_FAILED( result ) )
        {
            Log::Error( functionNameToReport, FormatString( "xrcreateApiLayerInstance failed for trampoline entry layer: %.", apiLayerArray[ 0 ]->name ) );

            this->Uninitialize_AssumesLocked( );

            return result;
        }


        this->activeXrInstance           = tmpInstance;
        *instance                        = tmpInstance;

        this->topmostGetInstanceProcAddr = apiLayerArray[ 0 ]->xrGetInstanceProcAddr;

        Holder<CoreXrFunctionPointerList> destroyer( Move( this->xrFunctionPointerList ) );
        xrFunctionPointerList = new CoreXrFunctionPointerList( this->activeXrInstance, this->topmostGetInstanceProcAddr );

        result                = CreateDefaultDebugUtilsMessenger( );

        if ( XR_FAILED( result ) )
        {
            this->Uninitialize_AssumesLocked( );
        }

        return result;
    }

    XrResult OpenXrLoader::xrDestroyInstance( XrInstance instance )
    {
        static const char* functionNameToReport = "xrDestroyInstance";

        Log::Verbose( functionNameToReport, "OpenXrLoader::xrDestroyInstance." );

        if ( instance == XR_NULL_HANDLE )
        {
            Log::Error( functionNameToReport, "XrInstance cannot be null." );

            return XR_ERROR_HANDLE_INVALID;
        }

        SharedMutexExclusiveLocker exclusiveLoaderMutexLocker( this->mutex );

        if ( xrFunctionPointerList == nullptr )
        {
            Log::Error( functionNameToReport, "XrInstance is no longer valid." );

            return XR_ERROR_HANDLE_INVALID;
        }

        XrResult result = xrFunctionPointerList->DestroyInstance( instance );

        if ( this->activeXrInstance != instance )
        {
            Log::Warning( functionNameToReport, "The provided instance is not active or is unknown; instance was destroyed regardless." );
            return XR_FAILED( result ) ? result : XR_SUCCESS;
        }

        if ( XR_FAILED( result ) )
        {
            Log::Error( functionNameToReport, "Destroy instance failed — uninitializing anyway." );
        }

        Uninitialize_AssumesLocked( );
        return result;
    }


    Holder<CoreXrFunctionPointerList>& OpenXrLoader::GetXrFunctionPointerList( )
    {
        return xrFunctionPointerList;
    }

    bool OpenXrLoader::LoaderSupportsExtension( const char* extensionName )
    {
        for ( const auto& supportedExtensionProperties : this->loaderSupportedExtensionPropertiesArray )
        {
            if ( Text::CompareText( supportedExtensionProperties.extensionName, extensionName ) )
            {
                return true;
            }
        }

        return false;
    }

    bool OpenXrLoader::IsExtensionEnabled( const char* extensionName )
    {
        for ( const auto& enabledExtensionProperties : this->enabledExtensionsExtensionPropertiesArray )
        {
            if ( Text::CompareText( enabledExtensionProperties.extensionName, extensionName ) )
            {
                return true;
            }
        }

        return false;
    }


    XrResult OpenXrLoader::xrEnumerateInstanceExtensionProperties( const char* layerName, uint32_t propertyCapacityInput, uint32_t* propertyCountOutput, XrExtensionProperties* properties )
    {
        static const char* functionNameToReport = "xrEnumerateInstanceExtensionProperties";

        if ( propertyCapacityInput != 0 && properties == nullptr )
        {
            Log::ValidationError( "VUID-xrEnumerateInstanceExtensionProperties-properties-parameter", functionNameToReport,
                                  "If propertyCapacityInput is not 0, properties must be a pointer to an array of propertyCapacityInput XrExtensionProperties structures" );

            return XR_ERROR_VALIDATION_FAILURE;
        }

        if ( propertyCountOutput == nullptr )
        {
            Log::ValidationError( "VUID-xrEnumerateInstanceExtensionProperties-propertyCountOutput-parameter", functionNameToReport, "propertyCountOutput must be a pointer to a uint32_t value." );

            return XR_ERROR_VALIDATION_FAILURE;
        }

        *propertyCountOutput                   = 0;

        auto appendApiLayerExtensionProperties = []( const OpenXrApiLayerManifest* requestedOpenXrApiLayerManifest, Array<XrExtensionProperties>& extensionPropertiesArray )
        {
            for ( const auto& instanceExtension : requestedOpenXrApiLayerManifest->apiLayerDescriptor.instanceExtensionArray )
            {
                uint64 version;
                int32  textLength;
                auto   dataResult = Data::ReadIntegerLiteral( instanceExtension.extensionVersion, &textLength, &version );

                if ( dataResult != kDataOkay )
                {
                    Log::Warning( functionNameToReport, FormatString( "Api layer % contains an incompatible extension version for extension %",
                                                                      requestedOpenXrApiLayerManifest->apiLayerDescriptor.name, instanceExtension.name ) );

                    continue;
                }

                XrExtensionProperties extensionProperty;
                extensionProperty.type             = XR_TYPE_EXTENSION_PROPERTIES;
                extensionProperty.extensionVersion = uint32( version );
                extensionProperty.next             = nullptr;
                Text::CopyText( instanceExtension.name, extensionProperty.extensionName, XR_MAX_EXTENSION_NAME_SIZE );

                extensionPropertiesArray.AppendArrayElement( extensionProperty );
            }
        };


        String<>                     layerNameString        = layerName == 0 ? String<>( "" ) : String<>( layerName, XR_MAX_API_LAYER_NAME_SIZE );
        OpenXrLoader&                openXrRuntimeInterface = TheOpenXrInstanceManager;
        Array<XrExtensionProperties> supportedExtensionProperties;

        {
            SharedMutexSharedLocker xrLoaderSharedMutexSharedLocker( openXrRuntimeInterface.mutex );

            if ( !openXrRuntimeInterface.IsInitialized( ) )
            {
                Log::Error( functionNameToReport, "OpenXR runtime is unavailable." );

                return XR_ERROR_RUNTIME_UNAVAILABLE;
            }

            Array<Holder<OpenXrApiLayerManifest>> implicitApiLayerManifestArray, environmentVariableApiLayerManifestArray, explicitApiLayerManifestArray;

            XrResult result = PopulateOpenXrApiLayerManifestArrays( functionNameToReport, implicitApiLayerManifestArray, environmentVariableApiLayerManifestArray, explicitApiLayerManifestArray );

            // 4.2.1:
            // If the "XR_API_LAYER_PATH" environmental variable is defined, then the desktop loader will not look in the standard locations to find explicit API layers, instead looking only at the
            // paths defined in that environment variable. Implicit API layers will always be discovered using the standard paths.
            // Note: This is unclear to me. Figure 5. Loader API Layer Ordering indicates that we should enable explicit layers that are in the path if they are requested by the application (which is
            // in conflict with the note above).Following the interpretion based on Figure 5.
            // Note: What if the variable is ok, but the layer fails to load when we try to? I think we should fail when we try to do so, so we should be good here.

            if ( layerNameString.GetStringLength( ) != 0 )
            {
                bool apiLayerWasNotFound = true;

                // Check if we find the layer : first in implict, then in the environment variable explicit, and finally in the explict lists
                for ( const OpenXrApiLayerManifest* openXrApiLayerManifest : implicitApiLayerManifestArray )
                {
                    if ( openXrApiLayerManifest->apiLayerDescriptor.name != layerNameString )
                    {
                        continue;
                    }

                    apiLayerWasNotFound = false;

                    appendApiLayerExtensionProperties( openXrApiLayerManifest, supportedExtensionProperties );

                    break;
                }

                if ( apiLayerWasNotFound )
                {
                    for ( const OpenXrApiLayerManifest* openXrApiLayerManifest : environmentVariableApiLayerManifestArray )
                    {
                        if ( openXrApiLayerManifest->apiLayerDescriptor.name != layerNameString )
                        {
                            continue;
                        }

                        apiLayerWasNotFound = false;

                        appendApiLayerExtensionProperties( openXrApiLayerManifest, supportedExtensionProperties );

                        break;
                    }
                }

                if ( apiLayerWasNotFound )
                {
                    for ( const OpenXrApiLayerManifest* openXrApiLayerManifest : explicitApiLayerManifestArray )
                    {
                        if ( openXrApiLayerManifest->apiLayerDescriptor.name != layerNameString )
                        {
                            continue;
                        }

                        apiLayerWasNotFound = false;

                        appendApiLayerExtensionProperties( openXrApiLayerManifest, supportedExtensionProperties );

                        break;
                    }
                }

                if ( apiLayerWasNotFound )
                {
                    Log::Warning( functionNameToReport, FormatString( "Requested Api layer: %, was not found.", layerNameString ) );

                    return XR_ERROR_API_LAYER_NOT_PRESENT;
                }
            }
            else
            {
                for ( const OpenXrApiLayerManifest* openXrApiLayerManifest : implicitApiLayerManifestArray )
                {
                    appendApiLayerExtensionProperties( openXrApiLayerManifest, supportedExtensionProperties );
                }

                for ( const OpenXrApiLayerManifest* openXrApiLayerManifest : environmentVariableApiLayerManifestArray )
                {
                    appendApiLayerExtensionProperties( openXrApiLayerManifest, supportedExtensionProperties );
                }

                for ( const OpenXrApiLayerManifest* openXrApiLayerManifest : explicitApiLayerManifestArray )
                {
                    appendApiLayerExtensionProperties( openXrApiLayerManifest, supportedExtensionProperties );
                }

                for ( const auto& runtimeExtensionProperties : this->openXrRuntime.GetXrExtensionPropertiesArray( ) )
                {
                    bool runtimeOrLoaderExtensionPropertyNotAlreadyEnumerated = true;

                    for ( auto& enumeratedExtensionProperty : supportedExtensionProperties )
                    {
                        if ( Text::CompareText( enumeratedExtensionProperty.extensionName, runtimeExtensionProperties.extensionName, XR_MAX_EXTENSION_NAME_SIZE ) )
                        {
                            runtimeOrLoaderExtensionPropertyNotAlreadyEnumerated = false;
                            enumeratedExtensionProperty.extensionVersion         = Max( enumeratedExtensionProperty.extensionVersion, runtimeExtensionProperties.extensionVersion );

                            break;
                        }
                    }

                    if ( runtimeOrLoaderExtensionPropertyNotAlreadyEnumerated )
                    {
                        supportedExtensionProperties.AppendArrayElement( runtimeExtensionProperties );
                    }
                }

                for ( const auto& loaderExtensionProperties : this->loaderSupportedExtensionPropertiesArray )
                {
                    supportedExtensionProperties.AppendArrayElement( loaderExtensionProperties );
                }
            }
        }

        *propertyCountOutput = supportedExtensionProperties.GetArrayElementCount( );

        if ( propertyCapacityInput == 0 )
        {
            return XR_SUCCESS;
        }

        if ( propertyCapacityInput < supportedExtensionProperties.GetArrayElementCount( ) )
        {
            Log::ValidationError( "VUID-xrEnumerateInstanceExtensionProperties-propertyCountOutput-parameter", functionNameToReport,
                                  "If propertyCapacityInput is not 0, properties must be a pointer to an array of propertyCapacityInput XrExtensionProperties structures." );

            return XR_ERROR_SIZE_INSUFFICIENT;
        }

        uint32 invalidPropertyCount = 0;

        for ( int32 a = 0; a != supportedExtensionProperties.GetArrayElementCount( ); a++ )
        {
            if ( properties[ a ].type != XR_TYPE_EXTENSION_PROPERTIES )
            {
                invalidPropertyCount++;

                continue;
            }

            properties[ a ] = supportedExtensionProperties[ a ];
        }

        if ( invalidPropertyCount != 0 )
        {
            Log::ValidationError(
                "VUID-XrExtensionProperties-type-type", functionNameToReport,
                FormatString( "properties[n]->type must be XR_TYPE_EXTENSION_PROPERTIES. % of % failed.", invalidPropertyCount, supportedExtensionProperties.GetArrayElementCount( ) ) );

            return XR_ERROR_VALIDATION_FAILURE;
        }

        return XR_SUCCESS;
    }


    XrResult OpenXrLoader::xrGetInstanceProcAddr( const char* functionName, PFN_xrVoidFunction* function )
    {
        return this->topmostGetInstanceProcAddr( this->activeXrInstance, functionName, function );
    }

    XrResult OpenXrLoader::xrEnumerateApiLayerProperties( uint32_t propertyCapacityInput, uint32_t* propertyCountOutput, XrApiLayerProperties* apiLayerProperties )
    {
        static const char* functionNameToReport = "xrEnumerateApiLayerProperties";

        if ( propertyCountOutput == nullptr )
        {
            Log::ValidationError( "VUID-xrEnumerateApiLayerProperties-propertyCountOutput-parameter", functionNameToReport, "propertyCountOutput must be a pointer to a uint32_t value." );

            return XR_ERROR_VALIDATION_FAILURE;
        }

        if ( propertyCapacityInput > 0 && apiLayerProperties == nullptr )
        {
            Log::ValidationError( "VUID-xrEnumerateApiLayerProperties-properties-parameter", functionNameToReport,
                                  "If propertyCapacityInput is not 0, properties must be a pointer to an array of propertyCapacityInput XrApiLayerProperties structures." );

            return XR_ERROR_VALIDATION_FAILURE;
        }

        for ( uint32_t a = 0; a != propertyCapacityInput; a++ )
        {
            if ( apiLayerProperties[ a ].type != XR_TYPE_API_LAYER_PROPERTIES )
            {
                Log::ValidationError( "VUID-XrApiLayerProperties-type-type", functionNameToReport, "properties[n]->type must be XR_TYPE_API_LAYER_PROPERTIES." );

                return XR_ERROR_VALIDATION_FAILURE;
            }
        }

        SharedMutexExclusiveLocker            loaderMutexLocker( this->mutex );

        Array<Holder<OpenXrApiLayerManifest>> implicitApiLayerManifestArray;
        Array<Holder<OpenXrApiLayerManifest>> environmentVariableApiLayerManifestArray;
        Array<Holder<OpenXrApiLayerManifest>> explicitApiLayerManifestArray;

        XrResult result = PopulateOpenXrApiLayerManifestArrays( functionNameToReport, implicitApiLayerManifestArray, environmentVariableApiLayerManifestArray, explicitApiLayerManifestArray );

        int32    manifestCount =
            implicitApiLayerManifestArray.GetArrayElementCount( ) + environmentVariableApiLayerManifestArray.GetArrayElementCount( ) + explicitApiLayerManifestArray.GetArrayElementCount( );

        *propertyCountOutput = manifestCount;
        if ( propertyCapacityInput == 0 )
        {
            return XR_SUCCESS;
        }

        if ( manifestCount > propertyCapacityInput )
        {
            Log::ValidationError( "VUID-xrEnumerateApiLayerProperties-properties-parameter", functionNameToReport,
                                  "If propertyCapacityInput is not 0, properties must be a pointer to an array of propertyCapacityInput XrApiLayerProperties structures." );

            return XR_ERROR_VALIDATION_FAILURE;
        }

        auto AppendXrApiLayerProperties = []( const Array<Holder<OpenXrApiLayerManifest>>& manifestArray, XrApiLayerProperties* outputXrApiLayerProperties )
        {
            for ( const auto& apiLayerManifest : manifestArray )
            {
                Text::CopyText( static_cast<const char*>( apiLayerManifest->apiLayerDescriptor.name ), outputXrApiLayerProperties->layerName, XR_MAX_API_LAYER_NAME_SIZE - 1 );
                Text::CopyText( static_cast<const char*>( apiLayerManifest->apiLayerDescriptor.description ), outputXrApiLayerProperties->description, XR_MAX_API_LAYER_DESCRIPTION_SIZE - 1 );

                outputXrApiLayerProperties->layerVersion = Data::StringToInt32( apiLayerManifest->apiLayerDescriptor.implementationVersion );
                outputXrApiLayerProperties->specVersion  = apiLayerManifest->apiLayerDescriptor.xrApiVersion;
                outputXrApiLayerProperties++;
            }

            return outputXrApiLayerProperties;
        };

        auto* apiLayerPropertiesBackPointer = apiLayerProperties;
        apiLayerPropertiesBackPointer       = AppendXrApiLayerProperties( implicitApiLayerManifestArray, apiLayerPropertiesBackPointer );
        apiLayerPropertiesBackPointer       = AppendXrApiLayerProperties( environmentVariableApiLayerManifestArray, apiLayerPropertiesBackPointer );
        AppendXrApiLayerProperties( explicitApiLayerManifestArray, apiLayerPropertiesBackPointer );

        return XR_SUCCESS;
    }

    XrResult OpenXrLoader::xrSetDebugUtilsObjectNameEXT_Terminator( XrInstance instance, const XrDebugUtilsObjectNameInfoEXT* debugUtilsObjectNameInfo )
    {
        static const char* functionNameToReport = "xrSetDebugUtilsObjectNameEXT";

        Log::Verbose( functionNameToReport, "Enter." );

        if ( instance == XR_NULL_HANDLE || instance != activeXrInstance )
        {
            Log::ValidationError( "VUID-xrCreateDebugUtilsMessengerEXT-instance-parameter", functionNameToReport,
                                  "instance must be a valid XrInstance handle." ); // Note: We support only one instance.

            return XR_ERROR_VALIDATION_FAILURE;
        }

        auto     fp     = this->openXrRuntime.GetFunctionPointerList( instance );
        XrResult result = ( fp && fp->SetDebugUtilsObjectNameEXT != nullptr ) ? fp->SetDebugUtilsObjectNameEXT( instance, debugUtilsObjectNameInfo ) : XR_SUCCESS;

        if ( XR_FAILED( result ) )
        {
            Log::Error( functionNameToReport, "SetDebugUtilsObjectName failed." );

            return result;
        }

        OpenXrObjectInfo objectInfo { debugUtilsObjectNameInfo->objectHandle, debugUtilsObjectNameInfo->objectType, debugUtilsObjectNameInfo->objectName };

        if ( objectInfo.name.GetStringLength( ) == 0 )
        {
            TheOpenXrInstanceManager.messageDispatcher.debugUtilsData.RemoveObjectInfo( objectInfo.handle, objectInfo.type );

            return result;
        }

        TheOpenXrInstanceManager.messageDispatcher.debugUtilsData.AddObjectInfo( objectInfo );

        Log::Verbose( functionNameToReport, "Exit." );

        return result;
    }


    XrResult OpenXrLoader::xrSubmitDebugUtilsMessageEXT_Terminator( XrInstance                                  instance,
                                                                    XrDebugUtilsMessageSeverityFlagsEXT         messageSeverity,
                                                                    XrDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                                                    const XrDebugUtilsMessengerCallbackDataEXT* callbackData )
    {
        static const char* functionNameToReport = "xrSubmitDebugUtilsMessageEXT";

        Log::Verbose( functionNameToReport, EnterTerminatorMsg );

        if ( instance == XR_NULL_HANDLE || instance != activeXrInstance )
        {
            Log::ValidationError( "VUID-xrCreateDebugUtilsMessengerEXT-instance-parameter", functionNameToReport,
                                  "instance must be a valid XrInstance handle." ); // Note: We support only one instance.

            return XR_ERROR_VALIDATION_FAILURE;
        }

        auto     fp     = this->openXrRuntime.GetFunctionPointerList( instance );
        XrResult result = XR_SUCCESS;
        if ( fp && fp->SubmitDebugUtilsMessageEXT != nullptr )
        {
            result = fp->SubmitDebugUtilsMessageEXT( instance, messageSeverity, messageTypes, callbackData );
        }
        else
        {
            TheOpenXrInstanceManager.messageDispatcher.SubmitDebugUtilsMessage( messageSeverity, messageTypes, callbackData );
        }

        Log::Verbose( functionNameToReport, ExitTerminatorMsg );

        return result;
    }

    XrResult OpenXrLoader::xrCreateDebugUtilsMessengerEXT_Terminator( XrInstance instance, const XrDebugUtilsMessengerCreateInfoEXT* createInfo, XrDebugUtilsMessengerEXT* messenger )
    {
        static const char* functionNameToReport = "xrCreateDebugUtilsMessengerEXT";

        Log::Verbose( functionNameToReport, EnterTerminatorMsg );

        if ( instance == XR_NULL_HANDLE || instance != activeXrInstance )
        {
            Log::ValidationError( "VUID-xrCreateDebugUtilsMessengerEXT-instance-parameter", functionNameToReport,
                                  "instance must be a valid XrInstance handle." ); // Note: We support only one instance.

            return XR_ERROR_VALIDATION_FAILURE;
        }

        if ( createInfo == nullptr )
        {
            Log::ValidationError( "VUID-xrCreateDebugUtilsMessengerEXT-createInfo-parameter", functionNameToReport,
                                  "createInfo must be a pointer to a valid XrDebugUtilsMessengerCreateInfoEXT structure." );

            return XR_ERROR_VALIDATION_FAILURE;
        }

        if ( messenger == nullptr )
        {
            Log::ValidationError( "VUID-xrCreateDebugUtilsMessengerEXT-messenger-parameter", functionNameToReport, "messenger must be a pointer to an XrDebugUtilsMessengerEXT handle." );

            return XR_ERROR_VALIDATION_FAILURE;
        }

        auto fp = this->openXrRuntime.GetFunctionPointerList( instance );

        if ( !fp || fp->CreateDebugUtilsMessengerEXT == nullptr )
        {
            Log::Info( functionNameToReport, "instance could not be found in the runtimeFunctionPointerListMap, or runtime provided invalid xrCreateDebugUtilsMessengerEXT." );

            DebugUtilsMessenger* debugUtilsLogger = new DebugUtilsMessenger( createInfo->messageSeverities, createInfo->messageTypes, createInfo->userData, createInfo->userCallback );

            TheOpenXrInstanceManager.messageDispatcher.AppendDebugUtilsMessenger( debugUtilsLogger );

            *messenger = reinterpret_cast<XrDebugUtilsMessengerEXT>( debugUtilsLogger );

            Log::Verbose( functionNameToReport, ExitTerminatorMsg );

            return XR_SUCCESS;
        }

        XrResult result = fp->CreateDebugUtilsMessengerEXT( instance, createInfo, messenger );

        if ( XR_FAILED( result ) )
        {
            Log::Error( functionNameToReport, "OpenXR Runtime failed to create DebugUtilsMessenger." );

            return result;
        }

        TheOpenXrInstanceManager.messageDispatcher.AppendDebugUtilsMessenger(
            new DebugUtilsMessenger( createInfo->messageSeverities, createInfo->messageTypes, createInfo->userData, createInfo->userCallback ) );

        Log::Verbose( functionNameToReport, ExitTerminatorMsg );

        return result;
    }

    XrResult OpenXrLoader::xrDestroyDebugUtilsMessengerEXT_Terminator( XrDebugUtilsMessengerEXT messenger )
    {
        static const char* functionNameToReport = "xrDestroyDebugUtilsMessengerEXT";

        Log::Verbose( functionNameToReport, EnterTerminatorMsg );

        auto fp = this->openXrRuntime.GetFunctionPointerList( activeXrInstance );

        if ( !fp || fp->DestroyDebugUtilsMessengerEXT == nullptr )
        {
            Log::Verbose(
                functionNameToReport,
                "the active XrInstance could not be found in the runtimeFunctionPointerListMap, or runtime provided invalid xrDestroyDebugUtilsMessengerEXT. This is generally not an issue." );

            bool existed = TheOpenXrInstanceManager.messageDispatcher.RemoveDebugUtilsMessenger( reinterpret_cast<DebugUtilsMessenger*>( messenger ) );

            if ( !existed )
            {
                Log::ValidationError( "VUID-xrDestroyDebugUtilsMessengerEXT-messenger-parameter", functionNameToReport, "messenger must be a valid XrDebugUtilsMessengerEXT handle." );
            }

            Log::Verbose( functionNameToReport, ExitTerminatorMsg );

            return existed ? XR_SUCCESS : XR_ERROR_HANDLE_INVALID;
        }

        XrResult result = fp->DestroyDebugUtilsMessengerEXT( messenger );

        Log::Verbose( functionNameToReport, ExitTerminatorMsg );

        return result;
    }

    XrResult OpenXrLoader::xrCreateDebugUtilsMessengerEXT_Trampoline( XrInstance instance, const XrDebugUtilsMessengerCreateInfoEXT* createInfo, XrDebugUtilsMessengerEXT* messenger )
    {
        static const char* functionNameToReport = "xrCreateDebugUtilsMessengerEXT";

        Log::Verbose( functionNameToReport, EnterTrampolineMsg );

        if ( instance == XR_NULL_HANDLE )
        {
            Log::ValidationError( "VUID-xrCreateDebugUtilsMessengerEXT-instance-parameter", functionNameToReport,
                                  "instance must be a valid XrInstance handle." ); // Note: We support only one instance.

            return XR_ERROR_VALIDATION_FAILURE;
        }

        if ( this->xrFunctionPointerList == nullptr || this->xrFunctionPointerList->CreateDebugUtilsMessengerEXT == nullptr )
        {
            Log::Error( functionNameToReport, "Internal error: xrFunctionPointerList is null pointer." );

            return XR_ERROR_HANDLE_INVALID;
        }

        XrResult result = this->xrFunctionPointerList->CreateDebugUtilsMessengerEXT( instance, createInfo, messenger );

        Log::Verbose( functionNameToReport, ExitTrampolineMsg );

        return result;
    }


    XrResult OpenXrLoader::xrDestroyDebugUtilsMessengerEXT_Trampoline( XrDebugUtilsMessengerEXT messenger )
    {
        static const char* functionNameToReport = "xrDestroyDebugUtilsMessengerEXT";

        Log::Verbose( functionNameToReport, EnterTrampolineMsg );

        if ( messenger == XR_NULL_HANDLE )
        {
            Log::ValidationError( "VUID-xrDestroyDebugUtilsMessengerEXT-messenger-parameter", functionNameToReport, "messenger must be a valid XrDebugUtilsMessengerEXT handle." );

            return XR_ERROR_HANDLE_INVALID;
        }

        if ( this->xrFunctionPointerList == nullptr || this->xrFunctionPointerList->DestroyDebugUtilsMessengerEXT == nullptr )
        {
            Log::Error( functionNameToReport, "Internal error: xrFunctionPointerList or xrFunctionPointerList->DestroyDebugUtilsMessengerEXT is null pointer." );

            return XR_ERROR_HANDLE_INVALID;
        }

        XrResult result = this->xrFunctionPointerList->DestroyDebugUtilsMessengerEXT( messenger );

        Log::Verbose( functionNameToReport, ExitTrampolineMsg );

        return result;
    }
    XrResult OpenXrLoader::xrSessionInsertDebugUtilsLabelEXT_Trampoline( XrSession session, const XrDebugUtilsLabelEXT* labelInfo )
    {
        static const char* functionNameToReport = "xrSessionInsertDebugUtilsLabelEXT";

        Log::Verbose( functionNameToReport, EnterTrampolineMsg );

        if ( session == XR_NULL_HANDLE )
        {
            Log::ValidationError( "VUID-xrSessionInsertDebugUtilsLabelEXT-session-parameter", functionNameToReport, "session must be a valid XrSession handle." );

            return XR_ERROR_HANDLE_INVALID;
        }

        if ( labelInfo == nullptr )
        {
            Log::ValidationError( "VUID-xrSessionInsertDebugUtilsLabelEXT-labelInfo-parameter", functionNameToReport, "labelInfo must be a pointer to a valid XrDebugUtilsLabelEXT structure.",
                                  CreateObjectInfoArray( session, XR_OBJECT_TYPE_SESSION ) );

            return XR_ERROR_VALIDATION_FAILURE;
        }

        TheOpenXrInstanceManager.messageDispatcher.debugUtilsData.InsertLabel( session, *labelInfo );

        if ( xrFunctionPointerList == nullptr || xrFunctionPointerList->SessionInsertDebugUtilsLabelEXT == nullptr )
        {
            Log::Verbose( functionNameToReport, "SessionInsertDebugUtilsLabelEXT function pointer is unavailable" );

            return XR_SUCCESS;
        }

        XrResult result = xrFunctionPointerList->SessionInsertDebugUtilsLabelEXT( session, labelInfo );

        Log::Verbose( functionNameToReport, ExitTrampolineMsg );

        return result;
    }


    XrResult OpenXrLoader::xrSetDebugUtilsObjectNameEXT_Trampoline( XrInstance instance, const XrDebugUtilsObjectNameInfoEXT* nameInfo )
    {
        static const char* functionNameToReport = "xrSetDebugUtilsObjectNameEXT";

        Log::Verbose( functionNameToReport, EnterTrampolineMsg );

        if ( xrFunctionPointerList == nullptr || xrFunctionPointerList->SetDebugUtilsObjectNameEXT == nullptr )
        {
            Log::Verbose( functionNameToReport, "SetDebugUtilsObjectNameEXT function pointer is unavailable" );

            return XR_SUCCESS;
        }

        XrResult result = xrFunctionPointerList->SetDebugUtilsObjectNameEXT( instance, nameInfo );

        if ( XR_SUCCEEDED( result ) )
        {
            TheOpenXrInstanceManager.messageDispatcher.debugUtilsData.AddObjectInfo( OpenXrObjectInfo { nameInfo->objectHandle, nameInfo->objectType, nameInfo->objectName } );
        }

        Log::Verbose( functionNameToReport, ExitTrampolineMsg );

        return result;
    }


    XrResult OpenXrLoader::xrSubmitDebugUtilsMessageEXT_Trampoline( XrInstance                                  instance,
                                                                    XrDebugUtilsMessageSeverityFlagsEXT         messageSeverity,
                                                                    XrDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                                                    const XrDebugUtilsMessengerCallbackDataEXT* callbackData )
    {
        static const char* functionNameToReport = "xrSubmitDebugUtilsMessageEXT";

        Log::Verbose( functionNameToReport, EnterTrampolineMsg );

        if ( xrFunctionPointerList == nullptr || this->xrFunctionPointerList->SubmitDebugUtilsMessageEXT == nullptr )
        {
            Log::Error( functionNameToReport, "Internal error: xrFunctionPointerList or xrFunctionPointerList->SubmitDebugUtilsMessageEXT is null pointer." );

            return XR_ERROR_HANDLE_INVALID;
        }

        XrResult result = xrFunctionPointerList->SubmitDebugUtilsMessageEXT( instance, messageSeverity, messageTypes, callbackData );

        Log::Verbose( functionNameToReport, ExitTrampolineMsg );

        return result;
    }

    XrResult OpenXrLoader::xrSessionBeginDebugUtilsLabelRegionEXT_Trampoline( XrSession session, const XrDebugUtilsLabelEXT* labelInfo )
    {
        static const char* functionNameToReport = "xrSessionBeginDebugUtilsLabelRegionEXT";

        Log::Verbose( functionNameToReport, EnterTrampolineMsg );

        if ( session == XR_NULL_HANDLE )
        {
            Log::Error( functionNameToReport, "session cannot be XR_NULL_HANDLE." );
            return XR_ERROR_HANDLE_INVALID;
        }

        if ( labelInfo == nullptr )
        {
            Log::Error( functionNameToReport, "labelInfo cannot be a nullptr." );
            return XR_ERROR_HANDLE_INVALID;
        }

        TheOpenXrInstanceManager.messageDispatcher.debugUtilsData.BeginLabelRegion( session, *labelInfo );

        if ( xrFunctionPointerList == nullptr || xrFunctionPointerList->SessionBeginDebugUtilsLabelRegionEXT == nullptr )
        {
            Log::Verbose( functionNameToReport, "SessionBeginDebugUtilsLabelRegionEXT function pointer is unavailable" );

            return XR_SUCCESS;
        }

        XrResult result = xrFunctionPointerList->SessionBeginDebugUtilsLabelRegionEXT( session, labelInfo );

        Log::Verbose( functionNameToReport, ExitTrampolineMsg );

        return result;
    }

    XrResult OpenXrLoader::xrSessionEndDebugUtilsLabelRegionEXT_Trampoline( XrSession session )
    {
        static const char* functionNameToReport = "xrSessionEndDebugUtilsLabelRegionEXT";

        Log::Verbose( functionNameToReport, EnterTrampolineMsg );

        if ( session == XR_NULL_HANDLE )
        {
            Log::Error( functionNameToReport, "session cannot be XR_NULL_HANDLE." );

            return XR_ERROR_HANDLE_INVALID;
        }

        TheOpenXrInstanceManager.messageDispatcher.debugUtilsData.EndLabelRegion( session );

        if ( xrFunctionPointerList == nullptr || xrFunctionPointerList->SessionEndDebugUtilsLabelRegionEXT == nullptr )
        {
            Log::Verbose( functionNameToReport, "SessionEndDebugUtilsLabelRegionEXT function pointer is unavailable" );

            return XR_SUCCESS;
        }

        XrResult result = xrFunctionPointerList->SessionEndDebugUtilsLabelRegionEXT( session );

        Log::Verbose( functionNameToReport, ExitTrampolineMsg );

        return result;
    }


    static XRAPI_ATTR XrResult XRAPI_CALL openXrManager_xrEnumerateApiLayerProperties( uint32_t propertyCapacityInput, uint32_t* propertyCountOutput, XrApiLayerProperties* properties )
    {

        if ( XrResult result = TheOpenXrInstanceManager.EnsureIsInitialized( "xrEnumerateApiLayerProperties" ); XR_FAILED( result ) )
        {
            return result;
        }

        return TheOpenXrInstanceManager.xrEnumerateApiLayerProperties( propertyCapacityInput, propertyCountOutput, properties );
    }

    static XRAPI_ATTR XrResult XRAPI_CALL openXrManager_xrEnumerateInstanceExtensionProperties( const char*            layerName,
                                                                                                uint32_t               propertyCapacityInput,
                                                                                                uint32_t*              propertyCountOutput,
                                                                                                XrExtensionProperties* properties )
    {
        if ( XrResult result = TheOpenXrInstanceManager.EnsureIsInitialized( "xrEnumerateInstanceExtensionProperties" ); XR_FAILED( result ) )
        {
            return result;
        }

        return TheOpenXrInstanceManager.xrEnumerateInstanceExtensionProperties( layerName, propertyCapacityInput, propertyCountOutput, properties );
    }

    XrResult XRAPI_CALL openXrManager_xrCreateInstance( const XrInstanceCreateInfo* createInfo, XrInstance* instance )
    {
        if ( XrResult result = TheOpenXrInstanceManager.EnsureIsInitialized( "xrCreateInstance" ); XR_FAILED( result ) )
        {
            return result;
        }

        return TheOpenXrInstanceManager.xrCreateInstance( createInfo, instance );
    }


    static XRAPI_ATTR XrResult XRAPI_CALL openXrManager_xrDestroyInstance( XrInstance instance )
    {
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            return XR_ERROR_HANDLE_INVALID;
        }

        return TheOpenXrInstanceManager.xrDestroyInstance( instance );
    }

} // namespace VortexXr


extern "C"
{
VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateApiLayerProperties( uint32_t propertyCapacityInput, uint32_t* propertyCountOutput, XrApiLayerProperties* properties )
{
    return VortexXr::openXrManager_xrEnumerateApiLayerProperties( propertyCapacityInput, propertyCountOutput, properties );
}

VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateInstanceExtensionProperties( const char*            layerName,
                                                                                           uint32_t               propertyCapacityInput,
                                                                                           uint32_t*              propertyCountOutput,
                                                                                           XrExtensionProperties* properties )
{
    return VortexXr::openXrManager_xrEnumerateInstanceExtensionProperties( layerName, propertyCapacityInput, propertyCountOutput, properties );
}

VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrCreateInstance( const XrInstanceCreateInfo* createInfo, XrInstance* instance )
{
    return VortexXr::openXrManager_xrCreateInstance( createInfo, instance );
}

VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrDestroyInstance( XrInstance instance )
{
    return VortexXr::openXrManager_xrDestroyInstance( instance );
}
}

namespace VortexXr
{

    static XRAPI_ATTR XrResult XRAPI_CALL xrGetInstanceProcAddr_Dispatch( XrInstance instance, const char* functionName, PFN_xrVoidFunction* function )
    {
        using namespace VortexXr;

        static const char* functionNameToReport = "xrGetInstanceProcAddr";

        if ( functionName == nullptr )
        {
            Log::ValidationError( "VUID-xrGetInstanceProcAddr-name-parameter", functionNameToReport, "name must be a null-terminated UTF-8 string." );

            return XR_ERROR_VALIDATION_FAILURE;
        }

        if ( function == nullptr )
        {
            Log::ValidationError( "VUID-xrGetInstanceProcAddr-function-parameter", functionNameToReport, "function must be a pointer to a PFN_xrVoidFunction value." );

            return XR_ERROR_VALIDATION_FAILURE;
        }

        auto IsFunctionName = [ &functionName ]( const char* comp )
        {
            return VortexXr::CompareFunctionName( functionName, comp );
        };

        auto SetFunctionIfNameIs = [ &function, &IsFunctionName ]( const char* comp, auto pfn )
        {
            if ( IsFunctionName( comp ) )
            {
                *function = reinterpret_cast<PFN_xrVoidFunction>( pfn );

                return true;
            }
            return false;
        };

        // Initialization functions. These do not need an XrInstance
        if ( SetFunctionIfNameIs( "xrEnumerateApiLayerProperties", openXrManager_xrEnumerateApiLayerProperties ) )
        {
            return XR_SUCCESS;
        }

        if ( SetFunctionIfNameIs( "xrEnumerateInstanceExtensionProperties", openXrManager_xrEnumerateInstanceExtensionProperties ) )
        {
            return XR_SUCCESS;
        }

        if ( SetFunctionIfNameIs( "xrCreateInstance", openXrManager_xrCreateInstance ) )
        {
            return XR_SUCCESS;
        }

        if ( IsFunctionName( "xrInitializeLoaderKHR" ) )
        {
            return XR_ERROR_FUNCTION_UNSUPPORTED;
        }

        // Functions that need an active XrInstance

        if ( instance == XR_NULL_HANDLE )
        {
            Log::Error( functionNameToReport, "The provided instance is a null handle." );

            return XR_ERROR_HANDLE_INVALID;
        }

        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( functionNameToReport, "There is no active XrInstance." );

            return XR_ERROR_HANDLE_INVALID;
        }

        if ( TheOpenXrInstanceManager.GetActiveXrInstance( ) != instance )
        {
            Log::ValidationError( "VUID-xrGetInstanceProcAddr-instance-parameter", functionNameToReport, "If instance is not XR_NULL_HANDLE, instance must be a valid XrInstance handle." );

            return XR_ERROR_HANDLE_INVALID;
        }

        if ( SetFunctionIfNameIs( "xrGetInstanceProcAddr", xrGetInstanceProcAddr_Dispatch ) )
        {
            return XR_SUCCESS;
        }

        if ( SetFunctionIfNameIs( "xrDestroyInstance", openXrManager_xrDestroyInstance ) )
        {
            return XR_SUCCESS;
        }

        PFN_xrVoidFunction userProvidedFunctionHandle = *function;
        //
        auto CheckExtDebugUtilsIsEnabled = [ & ]( )
        {
            if ( !TheOpenXrInstanceManager.IsExtensionEnabled( XR_EXT_DEBUG_UTILS_EXTENSION_NAME ) )
            {
                Log::Error( functionNameToReport, String<>( "Pointer to " ) + String<>( functionName ) + " could not be supplied because extension EXT_debug_utils is not enabled." );

                *function = userProvidedFunctionHandle; // Revert to user provided handle to avoid UB in user code.
                return XR_ERROR_FUNCTION_UNSUPPORTED;
            }
            return XR_SUCCESS;
        };


        if ( SetFunctionIfNameIs( "xrSubmitDebugUtilsMessageEXT", xrSubmitDebugUtilsMessageEXT_Trampoline ) )
        {
            return CheckExtDebugUtilsIsEnabled( );
        }

        if ( SetFunctionIfNameIs( "xrSessionInsertDebugUtilsLabelEXT", xrSessionInsertDebugUtilsLabelEXT_Trampoline ) )
        {
            return CheckExtDebugUtilsIsEnabled( );
        }

        if ( SetFunctionIfNameIs( "xrSessionBeginDebugUtilsLabelRegionEXT", xrSessionBeginDebugUtilsLabelRegionEXT_Trampoline ) )
        {
            return CheckExtDebugUtilsIsEnabled( );
        }

        if ( SetFunctionIfNameIs( "xrSessionEndDebugUtilsLabelRegionEXT", xrSessionEndDebugUtilsLabelRegionEXT_Trampoline ) )
        {
            return CheckExtDebugUtilsIsEnabled( );
        }

        if ( SetFunctionIfNameIs( "xrSetDebugUtilsObjectNameEXT", xrSetDebugUtilsObjectNameEXT_Trampoline ) )
        {
            return CheckExtDebugUtilsIsEnabled( );
        }

        if ( SetFunctionIfNameIs( "xrCreateDebugUtilsMessengerEXT", xrCreateDebugUtilsMessengerEXT_Trampoline ) )
        {
            return CheckExtDebugUtilsIsEnabled( );
        }

        if ( SetFunctionIfNameIs( "xrDestroyDebugUtilsMessengerEXT", xrDestroyDebugUtilsMessengerEXT_Trampoline ) )
        {
            return CheckExtDebugUtilsIsEnabled( );
        }


        // Command functions
        return TheOpenXrInstanceManager.xrGetInstanceProcAddr( functionName, function );
    }

} // namespace VortexXr

extern "C"
{

VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrGetInstanceProcAddr( XrInstance instance, const char* name, PFN_xrVoidFunction* function )
{
    return VortexXr::xrGetInstanceProcAddr_Dispatch( instance, name, function );
}

} // extern "C"