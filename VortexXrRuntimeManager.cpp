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

#include "VortexXrRuntimeManager.h"
#include "VortexXrMessageDispatch.h"
#include "VortexXrOpenXrInstanceManager.h"

#include "openxr/openxr_loader_negotiation.h"
#include <openxr/openxr.h>

namespace VortexXr
{

    OpenXrRuntimeManager::~OpenXrRuntimeManager( )
    {
        Unload( );
    }

    XrResult OpenXrRuntimeManager::LoadRuntimeLibrary( const char* functionNameToReport, const String<>& fileName )
    {
        UniqueHolder<DynamicLibrary> temp( new DynamicLibrary( fileName ) );

        if ( !temp->IsInitialized( ) )
        {
            Log::Error( functionNameToReport, "Runtime library failed to load." );
            return XR_ERROR_RUNTIME_UNAVAILABLE;
        }

        {
            SharedMutexExclusiveLocker lock( runtimeStateMutex );
            openXrRuntimeLibrary = Move( temp );
        }

        return XR_SUCCESS;
    }

    XrResult OpenXrRuntimeManager::NegotiateRuntimeInterface( const char* functionNameToReport, PFN_xrGetInstanceProcAddr& outGetInstanceProcAddr )
    {
        PFN_xrNegotiateLoaderRuntimeInterface xrNegotiateLoaderRuntimeInterface = nullptr;

        {
            SharedMutexSharedLocker lock( runtimeStateMutex );

            if ( !openXrRuntimeLibrary || !openXrRuntimeLibrary->IsInitialized( ) )
            {
                Log::Error( functionNameToReport, "OpenXR runtime library is not loaded." );
                return XR_ERROR_RUNTIME_FAILURE;
            }

            xrNegotiateLoaderRuntimeInterface = openXrRuntimeLibrary->GetProcedureAddress<PFN_xrNegotiateLoaderRuntimeInterface>( "xrNegotiateLoaderRuntimeInterface" );
        }

        if ( xrNegotiateLoaderRuntimeInterface == nullptr )
        {
            Log::Error( functionNameToReport, "Failed to resolve xrNegotiateLoaderRuntimeInterface." );
            return XR_ERROR_RUNTIME_FAILURE;
        }

        XrNegotiateLoaderInfo loaderInfo { };
        loaderInfo.structType          = XR_LOADER_INTERFACE_STRUCT_LOADER_INFO;
        loaderInfo.structVersion       = OpenXrLoader::loaderInfoStructVersion;
        loaderInfo.structSize          = sizeof( XrNegotiateLoaderInfo );
        loaderInfo.minInterfaceVersion = OpenXrLoader::minInterfaceVersion;
        loaderInfo.maxInterfaceVersion = OpenXrLoader::maxInterfaceVersion;
        loaderInfo.minApiVersion       = OpenXrLoader::minApiVersion;
        loaderInfo.maxApiVersion       = OpenXrLoader::maxApiVersion;

        XrNegotiateRuntimeRequest runtimeRequest { };
        runtimeRequest.structType    = XR_LOADER_INTERFACE_STRUCT_RUNTIME_REQUEST;
        runtimeRequest.structVersion = OpenXrLoader::runtimeInfoStructVersion;
        runtimeRequest.structSize    = sizeof( XrNegotiateRuntimeRequest );

        XrResult result              = xrNegotiateLoaderRuntimeInterface( &loaderInfo, &runtimeRequest );

        if ( XR_FAILED( result ) )
        {
            Log::Error( functionNameToReport, "xrNegotiateLoaderRuntimeInterface failed." );

            return result;
        }

        if ( runtimeRequest.getInstanceProcAddr == nullptr )
        {
            Log::Error( functionNameToReport, "xrNegotiateLoaderRuntimeInterface returned null xrNegotiateRuntimeRequest.getInstanceProcAddr." );

            return XR_ERROR_RUNTIME_FAILURE;
        }

        if ( runtimeRequest.runtimeInterfaceVersion == 0 )
        {
            Log::Error( functionNameToReport, "xrNegotiateLoaderRuntimeInterface returned invalid xrNegotiateRuntimeRequest.runtimeInterfaceVersion." );

            return XR_ERROR_RUNTIME_FAILURE;
        }

        if ( runtimeRequest.runtimeApiVersion < OpenXrLoader::minApiVersion || runtimeRequest.runtimeApiVersion > OpenXrLoader::maxApiVersion )
        {
            Log::Error( functionNameToReport, "Runtime OpenXR API version is outside the supported range." );

            return XR_ERROR_API_VERSION_UNSUPPORTED;
        }

        outGetInstanceProcAddr = runtimeRequest.getInstanceProcAddr;

        return XR_SUCCESS;
    }

    XrResult OpenXrRuntimeManager::QueryEnumerateFunction( const char* functionNameToReport, PFN_xrGetInstanceProcAddr getInstanceProcAddr, PFN_xrEnumerateInstanceExtensionProperties& outFn )
    {
        XrResult result = getInstanceProcAddr( XR_NULL_HANDLE, "xrEnumerateInstanceExtensionProperties", reinterpret_cast<PFN_xrVoidFunction*>( &outFn ) );
        if ( XR_FAILED( result ) )
        {
            Log::Error( functionNameToReport, "Failed to resolve xrEnumerateInstanceExtensionProperties" );

            return result;
        }

        if ( outFn == nullptr )
        {
            Log::Error( functionNameToReport, "Runtime returned invalid xrEnumerateInstanceExtensionProperties function" );

            return XR_ERROR_RUNTIME_FAILURE;
        }

        return XR_SUCCESS;
    }

    XrResult OpenXrRuntimeManager::LoadExtensionProperties( const char* functionNameToReport, PFN_xrEnumerateInstanceExtensionProperties xrEnumerateInstanceExtensionProperties )
    {
        uint32_t propertyCount = 0;

        XrResult result        = xrEnumerateInstanceExtensionProperties( nullptr, 0, &propertyCount, nullptr );
        if ( XR_FAILED( result ) )
        {
            Log::Error( functionNameToReport, "Failed to query runtime instance extension properties." );

            return result;
        }

        const uint32_t               maxRetries = 10;
        uint32_t                     retryCount = 0;

        Array<XrExtensionProperties> tempExtensionPropertiesArray;

        while ( retryCount < maxRetries )
        {
            ++retryCount;

            uint32_t written = 0;

            {
                tempExtensionPropertiesArray.SetArrayElementCount( propertyCount, { XR_TYPE_EXTENSION_PROPERTIES, nullptr } );

                result = xrEnumerateInstanceExtensionProperties( nullptr, propertyCount, &written, tempExtensionPropertiesArray.begin( ) );
            }

            if ( written > propertyCount )
            {
                Log::Error( functionNameToReport, "Runtime returned more extension properties than allocated." );
                return XR_ERROR_RUNTIME_FAILURE;
            }

            propertyCount = written;

            if ( result == XR_SUCCESS )
            {
                SharedMutexExclusiveLocker lock( xrInstance_extensionPropertiesArrayMutex );

                this->extensionPropertiesArray.PurgeArray( );
                this->extensionPropertiesArray.SetArrayElementCount( propertyCount, { XR_TYPE_EXTENSION_PROPERTIES, nullptr } );

                for ( uint32 i = 0; i < propertyCount; i++ )
                {
                    this->extensionPropertiesArray[ i ] = tempExtensionPropertiesArray[ i ];
                }

                return XR_SUCCESS;
            }

            if ( result != XR_ERROR_SIZE_INSUFFICIENT )
            {
                Log::Error( functionNameToReport, "Failed to enumerate runtime instance extension properties." );

                return result;
            }

            result = xrEnumerateInstanceExtensionProperties( nullptr, 0, &propertyCount, nullptr );

            if ( XR_FAILED( result ) )
            {
                Log::Error( functionNameToReport, "Failed to query runtime instance extension properties." );
                return result;
            }
        }

        return XR_ERROR_RUNTIME_FAILURE;
    }


    XrResult OpenXrRuntimeManager::Load( const char* functionNameToReport, const String<>& fileName )
    {
        {
            SharedMutexSharedLocker lock( runtimeStateMutex );
            if ( openXrRuntimeLibrary != nullptr )
            {
                Log::Warning( functionNameToReport, "OpenXR runtime load called while another library is still loaded." );
            }
        }

        Reset( );

        XrResult result = LoadRuntimeLibrary( functionNameToReport, fileName );
        if ( XR_FAILED( result ) )
        {
            return result;
        }

        PFN_xrGetInstanceProcAddr tempGetInstanceProcAddr = nullptr;
        result                                            = NegotiateRuntimeInterface( functionNameToReport, tempGetInstanceProcAddr );
        if ( XR_FAILED( result ) )
        {
            Reset( );
            return result;
        }

        PFN_xrEnumerateInstanceExtensionProperties xrEnumerateInstanceExtensionProperties = nullptr;
        result = QueryEnumerateFunction( functionNameToReport, tempGetInstanceProcAddr, xrEnumerateInstanceExtensionProperties );
        if ( XR_FAILED( result ) )
        {
            Reset( );
            return result;
        }

        result = LoadExtensionProperties( functionNameToReport, xrEnumerateInstanceExtensionProperties );
        if ( XR_FAILED( result ) )
        {
            Reset( );
            return result;
        }

        {
            SharedMutexExclusiveLocker lock( runtimeStateMutex );
            getInstanceProcAddr = tempGetInstanceProcAddr;
        }

        return XR_SUCCESS;
    }


    void OpenXrRuntimeManager::Unload( )
    {
        Reset( );
    }

    void OpenXrRuntimeManager::Reset( )
    {
        {
            SharedMutexExclusiveLocker lock( xrInstance_functionPointerList_MapMutex );
            xrInstance_functionPointerList_Map.PurgeMap( );
        }

        {
            SharedMutexExclusiveLocker lock( xrInstance_extensionPropertiesArrayMutex );
            extensionPropertiesArray.PurgeArray( );
        }

        {
            SharedMutexExclusiveLocker lock( runtimeStateMutex );
            getInstanceProcAddr = nullptr;
            openXrRuntimeLibrary.reset( );
        }
    }

    bool OpenXrRuntimeManager::IsRuntimeValid_AssumesLocked( ) const
    {
        return openXrRuntimeLibrary && openXrRuntimeLibrary->IsInitialized( ) && getInstanceProcAddr;
    }

    bool OpenXrRuntimeManager::IsRuntimeValid( ) const
    {
        SharedMutexSharedLocker lock( runtimeStateMutex );

        return IsRuntimeValid_AssumesLocked( );
    }

    PFN_xrGetInstanceProcAddr OpenXrRuntimeManager::GetXrGetInstanceProcAddr( ) const
    {
        SharedMutexSharedLocker lock( runtimeStateMutex );
        return IsRuntimeValid_AssumesLocked( ) ? getInstanceProcAddr : nullptr;
    }

    Array<XrExtensionProperties> OpenXrRuntimeManager::GetXrExtensionPropertiesArray( ) const
    {
        SharedMutexSharedLocker lock( xrInstance_extensionPropertiesArrayMutex );
        return extensionPropertiesArray;
    }

    XrResult OpenXrRuntimeManager::CreateXrInstance( const char* functionNameToReport, const XrInstanceCreateInfo* createInfo, XrInstance* xrInstance )
    {
        if ( !xrInstance )
        {
            Log::ValidationError( "VUID-XrInstance-xrInstance-nullptr", functionNameToReport, "xrInstance must be a valid pointer (non-null)." );
            return XR_ERROR_VALIDATION_FAILURE;
        }

        if ( !createInfo || createInfo->type != XR_TYPE_INSTANCE_CREATE_INFO )
        {
            Log::ValidationError( "VUID-XrInstanceCreateInfo-type-type", functionNameToReport, "createInfo->type must be XR_TYPE_INSTANCE_CREATE_INFO." );
            return XR_ERROR_VALIDATION_FAILURE;
        }

        if ( createInfo->createFlags != 0 )
        {
            Log::ValidationError( "VUID-XrInstanceCreateInfo-createFlags-zerobitmask", functionNameToReport, "createInfo->createFlags must be 0." );
            return XR_ERROR_VALIDATION_FAILURE;
        }

        if ( createInfo->enabledExtensionCount != 0 && createInfo->enabledExtensionNames == nullptr )
        {
            Log::ValidationError( "VUID-XrInstanceCreateInfo-enabledExtensionNames-parameter", functionNameToReport, "If enabledExtensionCount is not 0, enabledExtensionNames must be valid." );
            return XR_ERROR_VALIDATION_FAILURE;
        }

        if ( createInfo->applicationInfo.applicationName[ 0 ] == '\0' ||
             GetTextLength( createInfo->applicationInfo.applicationName, XR_MAX_APPLICATION_NAME_SIZE + 1 ) == XR_MAX_APPLICATION_NAME_SIZE + 1 )
        {
            Log::ValidationError( "VUID-XrApplicationInfo-applicationName-parameter", functionNameToReport, "applicationName must be non-empty and within size limits." );
            return XR_ERROR_NAME_INVALID;
        }

        if ( GetTextLength( createInfo->applicationInfo.engineName, XR_MAX_ENGINE_NAME_SIZE + 1 ) == XR_MAX_ENGINE_NAME_SIZE + 1 )
        {
            Log::ValidationError( "VUID-XrApplicationInfo-engineName-parameter", functionNameToReport, "engineName must be within size limits." );
            return XR_ERROR_NAME_INVALID;
        }

        PFN_xrGetInstanceProcAddr localGetInstanceProcAddr = nullptr;
        {
            SharedMutexSharedLocker lock( runtimeStateMutex );

            if ( !IsRuntimeValid_AssumesLocked( ) )
            {
                Log::Error( functionNameToReport, "OpenXR runtime is not initialized." );
                return XR_ERROR_RUNTIME_FAILURE;
            }

            localGetInstanceProcAddr = getInstanceProcAddr;
        }

        PFN_xrCreateInstance runtimeXrCreateInstance = nullptr;
        XrResult             result                  = localGetInstanceProcAddr( XR_NULL_HANDLE, "xrCreateInstance", reinterpret_cast<PFN_xrVoidFunction*>( &runtimeXrCreateInstance ) );
        if ( XR_FAILED( result ) || runtimeXrCreateInstance == nullptr )
        {
            Log::Error( functionNameToReport, "Runtime failed to provide a valid xrCreateInstance function." );
            return XR_ERROR_RUNTIME_FAILURE;
        }

        result = runtimeXrCreateInstance( createInfo, xrInstance );

        if ( XR_FAILED( result ) )
        {
            Log::Error( functionNameToReport, FormatString( "runtimeXrCreateInstance failed with error code: %", result ) );

            return result;
        }

        {
            SharedMutexExclusiveLocker lock( xrInstance_functionPointerList_MapMutex );
            auto                       newElement = new FunctionPointerListMapElement( *xrInstance, CoreXrFunctionPointerList( *xrInstance, localGetInstanceProcAddr ) );
            auto*                      old        = xrInstance_functionPointerList_Map.InsertReplaceMapElement( newElement );
            delete old;
        }

        return XR_SUCCESS;
    }

    XrResult OpenXrRuntimeManager::DestroyXrInstance( const char* functionNameToReport, XrInstance xrInstance )
    {
        if ( xrInstance == XR_NULL_HANDLE )
        {
            return XR_SUCCESS;
        }

        PFN_xrGetInstanceProcAddr localGetInstanceProcAddr = nullptr;

        {
            SharedMutexSharedLocker lock( runtimeStateMutex );

            if ( !IsRuntimeValid_AssumesLocked( ) )
            {
                Log::Warning( functionNameToReport, "DestroyXrInstance called while runtime is not initialized." );
                return XR_ERROR_RUNTIME_FAILURE;
            }

            localGetInstanceProcAddr = getInstanceProcAddr;
        }

        PFN_xrDestroyInstance runtimeXrDestroyInstance = nullptr;

        XrResult              result                   = localGetInstanceProcAddr( xrInstance, "xrDestroyInstance", reinterpret_cast<PFN_xrVoidFunction*>( &runtimeXrDestroyInstance ) );

        if ( XR_FAILED( result ) || runtimeXrDestroyInstance == nullptr )
        {
            Log::Warning( functionNameToReport, "OpenXR runtime did not provide a valid xrDestroyInstance function. Returning XR_SUCCESS regardless." );
            return XR_SUCCESS;
        }

        result = runtimeXrDestroyInstance( xrInstance );

        {
            SharedMutexExclusiveLocker lock( xrInstance_functionPointerList_MapMutex );

            auto                       element = xrInstance_functionPointerList_Map.FindMapElement( xrInstance );
            if ( element )
            {
                xrInstance_functionPointerList_Map.RemoveMapElement( element );
                delete element;
            }
        }

        if ( XR_FAILED( result ) )
        {
            Log::Warning( functionNameToReport, "xrDestroyInstance failed." );
        }

        return result;
    }

    Optional<CoreXrFunctionPointerList> OpenXrRuntimeManager::GetFunctionPointerList( XrInstance instance ) const
    {
        SharedMutexSharedLocker        lock( xrInstance_functionPointerList_MapMutex );

        FunctionPointerListMapElement* element = xrInstance_functionPointerList_Map.FindMapElement( instance );
        if ( element )
        {
            return MakeOptional( element->xrFunctionPointerList );
        }

        return Optional<CoreXrFunctionPointerList>( );
    }

} // namespace VortexXr