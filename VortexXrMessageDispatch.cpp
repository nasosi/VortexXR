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

#include "VortexXrMessageDispatch.h"
#include "VortexXr.h"
#include "VortexXrUtility.h"

#include <TSString.h>

namespace VortexXr
{
    using namespace Terathon;

    String<> Logger::FormatXrMessage( XrDebugUtilsMessageSeverityFlagsEXT         messageSeverityFlags,
                                      XrDebugUtilsMessageTypeFlagsEXT             messageTypeFlags,
                                      const XrDebugUtilsMessengerCallbackDataEXT* debugUtilsMessengerCallbackData )
    {
        // clang-format off
        String<> formattedMessage = ( messageSeverityFlags < XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    ) ? "Verbose [": 
                                    ( messageSeverityFlags < XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ) ? "Info    [": 
                                    ( messageSeverityFlags < XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT   ) ? "Warning [": 
                                                                                                                 "Error   [";
        // clang-format on

        auto AppendTypeToFormattedMessage = [ & ]( XrDebugUtilsMessageTypeFlagsEXT type, const char* string )
        {
            if ( ( messageTypeFlags & type ) == type )
            {
                const char lastChar  = formattedMessage[ formattedMessage.GetStringLength( ) - 1 ];
                formattedMessage    += lastChar == '[' ? "" : ", ";
                formattedMessage    += string;

                return true;
            }

            return false;
        };

        auto S = []( const char* s )
        {
            return String<>( s );
        };

        bool knownMessageType  = AppendTypeToFormattedMessage( XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT, "GENERAL    " );
        knownMessageType      |= AppendTypeToFormattedMessage( XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT, "VALIDATION " );
        knownMessageType      |= AppendTypeToFormattedMessage( XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT, "PERFORMANCE" );
        knownMessageType      |= AppendTypeToFormattedMessage( XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT, "CONFORMANCE" );

        if ( !knownMessageType )
        {
            formattedMessage += "UNKNOWN";
        }

        const String<> sep = S( " | " );

        formattedMessage +=
            sep + S( debugUtilsMessengerCallbackData->functionName ) + sep + S( debugUtilsMessengerCallbackData->messageId ) + S( "]: " ) + S( debugUtilsMessengerCallbackData->message ) + S( "\n" );

        for ( uint32_t a = 0; a < debugUtilsMessengerCallbackData->objectCount; a++ )
        {
            // clang-format off

            XrDebugUtilsObjectNameInfoEXT& objectNameInfo = debugUtilsMessengerCallbackData->objects[ a ];

            formattedMessage += S( "    Object[" ) + 
                                Text::Integer64ToString( a ) +  "] = "  + 
                                Text::Integer64ToHexString16( objectNameInfo.objectHandle ) +  " (" + 
                                objectNameInfo.objectName + S( ")\n" );

            // clang-format on
        }

        for ( uint32_t a = 0; a < debugUtilsMessengerCallbackData->sessionLabelCount; a++ )
        {
            formattedMessage += S( "    SessionLabel[" ) + Text::Integer64ToString( a ) + "] = " + S( debugUtilsMessengerCallbackData->sessionLabels[ a ].labelName ) + "\n";
        }

        return formattedMessage;
    }

    Logger::Logger( XrDebugUtilsMessageSeverityFlagsEXT severityFlags, XrDebugUtilsMessageTypeFlagsEXT typeFlags ) : messageSeverityFlags( severityFlags ), messageTypeFlags( typeFlags )
    {
    }

    bool Logger::Intercepts( XrDebugUtilsMessageSeverityFlagsEXT severityFlags, XrDebugUtilsMessageTypeFlagsEXT typeFlags )
    {
        return ( ( this->messageSeverityFlags & severityFlags ) == severityFlags ) && ( ( this->messageTypeFlags & typeFlags ) == typeFlags );
    }

    bool ConsoleLogger::Log( XrDebugUtilsMessageSeverityFlagsEXT         messageSeverityFlags,
                             XrDebugUtilsMessageTypeFlagsEXT             messageTypeFlags,
                             const XrDebugUtilsMessengerCallbackDataEXT* debugUtilsMessengerCallbackDataEXT )
    {
        if ( this->Intercepts( messageSeverityFlags, messageTypeFlags ) )
        {
            const String<> message = this->FormatXrMessage( messageSeverityFlags, messageTypeFlags, debugUtilsMessengerCallbackDataEXT );

            if ( ( messageSeverityFlags & XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ) == XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
            {
                Detail::PrintfError( static_cast<const char*>( message ) );
            }
            else
            {
                Detail::Printf( static_cast<const char*>( message ) );
            }
        }

        return false; // If true this would signal exiting the application after logging.
    }

    DebugUtilsMessengerCallbackDataOwner::DebugUtilsMessengerCallbackDataOwner( )
    {
        xrDebugUtilsMessengerCallbackData.type              = XR_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT;
        xrDebugUtilsMessengerCallbackData.next              = nullptr;
        xrDebugUtilsMessengerCallbackData.messageId         = nullptr;
        xrDebugUtilsMessengerCallbackData.functionName      = nullptr;
        xrDebugUtilsMessengerCallbackData.message           = nullptr;
        xrDebugUtilsMessengerCallbackData.objectCount       = 0;
        xrDebugUtilsMessengerCallbackData.objects           = nullptr;
        xrDebugUtilsMessengerCallbackData.sessionLabelCount = 0;
        xrDebugUtilsMessengerCallbackData.sessionLabels     = nullptr;
    }

    XrSessionLabel::XrSessionLabel( const XrDebugUtilsLabelEXT& utilsLabel, Type t ) : debugUtilsLabel( utilsLabel ), name( utilsLabel.labelName ), type( t )
    {
        debugUtilsLabel.next      = nullptr;
        debugUtilsLabel.labelName = static_cast<const char*>( name );
    }

    XrSessionLabel::XrSessionLabel( const XrSessionLabel& other ) : debugUtilsLabel( other.debugUtilsLabel ), name( other.debugUtilsLabel.labelName ), type( other.type )
    {
        debugUtilsLabel.next      = nullptr;
        debugUtilsLabel.labelName = static_cast<const char*>( name );
    }

    const XrSessionLabel::Type& XrSessionLabel::GetType( ) const
    {
        return type;
    }

    const XrDebugUtilsLabelEXT& XrSessionLabel::GetDebugUtilsLabel( ) const
    {
        return debugUtilsLabel;
    }


    DebugUtilsMessenger::DebugUtilsMessenger( XrDebugUtilsMessageSeverityFlagsEXT  messageSeverityFlags,
                                              XrDebugUtilsMessageTypeFlagsEXT      messageTypeFlags,
                                              void*                                uData,
                                              PFN_xrDebugUtilsMessengerCallbackEXT callback ) :
        Logger( messageSeverityFlags, messageTypeFlags ),
        userCallback( callback ),
        userData( uData )
    {
    }

    bool DebugUtilsMessenger::Log( XrDebugUtilsMessageSeverityFlagsEXT         messageSeverityFlags,
                                   XrDebugUtilsMessageTypeFlagsEXT             messageTypeFlags,
                                   const XrDebugUtilsMessengerCallbackDataEXT* debugUtilsMessengerCallbackDataEXT )
    {
        if ( !this->Intercepts( messageSeverityFlags, messageTypeFlags ) )
        {
            return false;
        }

        return this->userCallback( messageSeverityFlags, messageTypeFlags, debugUtilsMessengerCallbackDataEXT, this->userData );
    }

    bool DebugUtilsMessenger::ProcessDebugUtilsMessage( XrDebugUtilsMessageSeverityFlagsEXT         messageSeverityFlags,
                                                        XrDebugUtilsMessageTypeFlagsEXT             messageTypeFlags,
                                                        const XrDebugUtilsMessengerCallbackDataEXT* debugUtilsMessengerCallbackDataEXT )
    {
        return this->Log( messageSeverityFlags, messageTypeFlags, debugUtilsMessengerCallbackDataEXT );
    }


    void MessageDispatcher::SetupCommonLoggers( )
    {
        String<> xrLoaderDebugEnvVarValue;

        XrResult getEnvVarResult = GetEnvironmentVariableValue( "XR_LOADER_DEBUG", xrLoaderDebugEnvVarValue );

        if ( xrLoaderDebugEnvVarValue == "none" )
        {
            return;
        }

        if ( xrLoaderDebugEnvVarValue.GetStringLength( ) == 0 )
        {
            AppendLogger( new ConsoleLogger( XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, allMessageTypes ) );

            if ( getEnvVarResult == XR_ERROR_VALIDATION_FAILURE )
            {
                this->SubmitMessage( XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT, GetVortexXrWithVersion( ), "MessageDispatcher::MessageDispatcher( )",
                                     "Could not read XR_LOADER_DEBUG environment variable.", Array<OpenXrObjectInfo>( ), __builtin_LINE( ), __builtin_FILE( ) );
            }

            return;
        }

        // clang-format off
        
        XrDebugUtilsMessageSeverityFlagsEXT severity =
              xrLoaderDebugEnvVarValue   == "error"                                        ? XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
            : xrLoaderDebugEnvVarValue   == "warn"                                         ? XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            : xrLoaderDebugEnvVarValue   == "info"                                         ? XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
            : xrLoaderDebugEnvVarValue   == "all" || xrLoaderDebugEnvVarValue == "verbose" ? allMessageSeverities
            : 0;

        // clang-format on

        if ( severity != 0 )
        {
            AppendLogger( new ConsoleLogger( severity, allMessageTypes ) );

            return;
        }

        this->SubmitMessage( XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT, GetVortexXrWithVersion( ), "MessageDispatcher::MessageDispatcher( )",
                             "XR_LOADER_DEBUG environment variable contains invalid value.", Array<OpenXrObjectInfo>( ), __builtin_LINE( ), __builtin_FILE( ) );
    }

    MessageDispatcher::~MessageDispatcher( )
    {
        Uninitialize( );
    }

    void MessageDispatcher::Uninitialize( )
    {
        SharedMutexExclusiveLocker exclusiveLocker( mutex );

        this->debugUtilsData.Uninitialize( );

        for ( auto debugUtilsMessenger : debugUtilsMessengerArray )
        {
            delete debugUtilsMessenger;
        }

        debugUtilsMessengerArray.PurgeArray( );
    }

    void MessageDispatcher::AppendLogger( Logger* logger )
    {
        SharedMutexExclusiveLocker exclusiveLocker( mutex );

        loggerArray.AppendArrayElement( logger );
    }

    void MessageDispatcher::AppendDebugUtilsMessenger( DebugUtilsMessenger* logger )
    {
        SharedMutexExclusiveLocker exclusiveLocker( mutex );

        debugUtilsMessengerArray.AppendArrayElement( logger );
    }

    bool MessageDispatcher::RemoveDebugUtilsMessenger( DebugUtilsMessenger* logger )
    {
        SharedMutexExclusiveLocker exclusiveLocker( mutex );

        int32                      index = debugUtilsMessengerArray.FindArrayElementIndex( logger );
        if ( index >= 0 )
        {
            debugUtilsMessengerArray.RemoveArrayElement( index );

            delete logger;

            return true;
        }

        return false;
    }

    bool MessageDispatcher::SubmitMessage( XrDebugUtilsMessageSeverityFlagsEXT messageSeverityFlags,
                                           XrDebugUtilsMessageTypeFlagsEXT     messageTypeFlags,
                                           const char*                         messageId,
                                           const char*                         functionName,
                                           const char*                         message,
                                           const Array<OpenXrObjectInfo>&      objectInfoArray,
                                           unsigned int                        callingLine,
                                           const char*                         callingFileName )
    {
        SharedMutexSharedLocker sharedLocker( this->mutex );

        if ( loggerArray.GetArrayElementCount( ) == 0 && debugUtilsMessengerArray.GetArrayElementCount( ) == 0 )
        {
            return false;
        }

        Array<XrDebugUtilsObjectNameInfoEXT> debugUtilsObjectNameInfoArray;
        Array<XrDebugUtilsLabelEXT>          labelArray;

        debugUtilsData.PopulateDebugUtilsObjectNamesAndLabelArray( objectInfoArray, debugUtilsObjectNameInfoArray, labelArray );

        String<> extendedMessage = String<>( message ) + String<>( " File: " ) + String<>( callingFileName ) + String<>( ", line: " ) + Text::Integer64ToString( callingLine );

        XrDebugUtilsMessengerCallbackDataEXT debugUtilsMessengerCallbackData;
        debugUtilsMessengerCallbackData.type              = XR_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT;
        debugUtilsMessengerCallbackData.next              = nullptr;
        debugUtilsMessengerCallbackData.messageId         = messageId;
        debugUtilsMessengerCallbackData.functionName      = functionName;
        debugUtilsMessengerCallbackData.message           = static_cast<char*>( extendedMessage );
        debugUtilsMessengerCallbackData.objectCount       = debugUtilsObjectNameInfoArray.GetArrayElementCount( );
        debugUtilsMessengerCallbackData.objects           = debugUtilsObjectNameInfoArray.GetArrayElementCount( ) == 0 ? nullptr : debugUtilsObjectNameInfoArray.begin( );
        debugUtilsMessengerCallbackData.sessionLabelCount = labelArray.GetArrayElementCount( );
        debugUtilsMessengerCallbackData.sessionLabels     = labelArray.GetArrayElementCount( ) == 0 ? nullptr : labelArray.begin( );

        bool requestExit                                  = false;

        for ( const auto& messenger : debugUtilsMessengerArray )
        {
            requestExit |= messenger->Log( messageSeverityFlags, messageTypeFlags, &debugUtilsMessengerCallbackData );
        }

        for ( const auto& logger : loggerArray )
        {
            requestExit |= logger->Log( messageSeverityFlags, messageTypeFlags, &debugUtilsMessengerCallbackData );
        }

        return requestExit;
    }

    bool MessageDispatcher::SubmitDebugUtilsMessage( XrDebugUtilsMessageSeverityFlagsEXT         messageSeverityFlags,
                                                     XrDebugUtilsMessageTypeFlagsEXT             messageTypeFlags,
                                                     const XrDebugUtilsMessengerCallbackDataEXT* debugUtilsMessengerCallbackData )
    {

        SharedMutexSharedLocker sharedLocker( mutex );

        auto                    liftedCallbackData = this->debugUtilsData.LiftCallbackData( debugUtilsMessengerCallbackData );
        bool                    requestExit        = false;

        for ( const auto& debugUtilsMessenger : debugUtilsMessengerArray )
        {
            requestExit |= debugUtilsMessenger->ProcessDebugUtilsMessage( messageSeverityFlags, messageTypeFlags, &liftedCallbackData.xrDebugUtilsMessengerCallbackData );
        }

        return requestExit;
    }

    bool Log::Verbose( const char* functionName, const char* message, const Array<OpenXrObjectInfo>& objectInfoArray, unsigned int callingLine, const char* callingFileName )
    {
        return TheOpenXrInstanceManager.messageDispatcher.SubmitMessage( XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT, XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT, GetVortexXrWithVersion( ),
                                                                         functionName, message, objectInfoArray, callingLine, callingFileName );
    }

    bool Log::Info( const char* functionName, const char* message, const Array<OpenXrObjectInfo>& objectInfoArray, unsigned int callingLine, const char* callingFileName )
    {
        return TheOpenXrInstanceManager.messageDispatcher.SubmitMessage( XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT, XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT, GetVortexXrWithVersion( ),
                                                                         functionName, message, objectInfoArray, callingLine, callingFileName );
    }

    bool Log::Warning( const char* functionName, const char* message, const Array<OpenXrObjectInfo>& objectInfoArray, unsigned int callingLine, const char* callingFileName )
    {
        return TheOpenXrInstanceManager.messageDispatcher.SubmitMessage( XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT, GetVortexXrWithVersion( ),
                                                                         functionName, message, objectInfoArray, callingLine, callingFileName );
    }

    bool Log::Error( const char* functionName, const char* message, const Array<OpenXrObjectInfo>& objectInfoArray, unsigned int callingLine, const char* callingFileName )
    {
        return TheOpenXrInstanceManager.messageDispatcher.SubmitMessage( XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT, GetVortexXrWithVersion( ),
                                                                         functionName, message, objectInfoArray, callingLine, callingFileName );
    }

    bool
        Log::ValidationWarning( const char* vuid, const char* functionName, const char* message, const Array<OpenXrObjectInfo>& objectInfoArray, unsigned int callingLine, const char* callingFileName )
    {
        return TheOpenXrInstanceManager.messageDispatcher.SubmitMessage( XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT, vuid, functionName, message,
                                                                         objectInfoArray, callingLine, callingFileName );
    }

    bool Log::ValidationError( const char* vuid, const char* functionName, const char* message, const Array<OpenXrObjectInfo>& objectInfoArray, unsigned int callingLine, const char* callingFileName )
    {
        return TheOpenXrInstanceManager.messageDispatcher.SubmitMessage( XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT, vuid, functionName, message,
                                                                         objectInfoArray, callingLine, callingFileName );
    }

    void DebugUtilsData::Uninitialize( )
    {
        objectInfoArray.PurgeArray( );
        sessionLabelArrayMap.PurgeMap( );
    }

    void DebugUtilsData::RemoveLastElementIfIndividual( Array<XrSessionLabel>* sessionLabelArray )
    {
        int32 lastElementIndex = sessionLabelArray->GetArrayElementCount( ) - 1;

        if ( lastElementIndex >= 0 )
        {
            if ( ( *sessionLabelArray )[ lastElementIndex ].GetType( ) == XrSessionLabel::Type::Individual )
            {
                sessionLabelArray->RemoveLastArrayElement( );
            }
        }
    }

    void DebugUtilsData::AddObjectInfo( OpenXrObjectInfo objectInfo )
    {
        bool objectInfoAlreadyExists = false;

        for ( auto& currentInfo : objectInfoArray )
        {
            if ( currentInfo.handle == objectInfo.handle && currentInfo.type == objectInfo.type )
            {
                currentInfo.name        = objectInfo.name;
                objectInfoAlreadyExists = true;
            }
        }

        if ( objectInfoAlreadyExists )
        {
            return;
        }

        objectInfoArray.AppendArrayElement( objectInfo );
    }

    void DebugUtilsData::RemoveObjectInfo( uint64 handle, XrObjectType type )
    {
        int32 a = 0;
        while ( a < objectInfoArray.GetArrayElementCount( ) )
        {
            if ( objectInfoArray[ a ].handle == handle && objectInfoArray[ a ].type == type )
            {
                objectInfoArray.RemoveArrayElement( a );
                continue;
            }
            a++;
        }
    }

    void DebugUtilsData::PopulateDebugUtilsObjectNamesAndLabelArray( const Array<OpenXrObjectInfo>&        queryObjectInfoArray,
                                                                     Array<XrDebugUtilsObjectNameInfoEXT>& debugUtilsObjectNameInfoArray,
                                                                     Array<XrDebugUtilsLabelEXT>&          labelArray )
    {
        debugUtilsObjectNameInfoArray.PurgeArray( );
        labelArray.PurgeArray( );

        for ( auto& queryObjectInfo : queryObjectInfoArray )
        {
            XrDebugUtilsObjectNameInfoEXT debugUtilsObjectNameInfo;
            debugUtilsObjectNameInfo.type         = XR_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            debugUtilsObjectNameInfo.objectHandle = queryObjectInfo.handle;
            debugUtilsObjectNameInfo.objectName   = queryObjectInfo.name;

            for ( const auto& thisObjectInfo : this->objectInfoArray )
            {
                if ( ( thisObjectInfo.handle == queryObjectInfo.handle ) && ( thisObjectInfo.type == queryObjectInfo.type ) )
                {
                    debugUtilsObjectNameInfo.objectName = thisObjectInfo.name;
                    break;
                }
            }

            debugUtilsObjectNameInfoArray.AppendArrayElement( debugUtilsObjectNameInfo );

            if ( queryObjectInfo.type == XR_OBJECT_TYPE_SESSION )
            {
                SessionLabelArrayMapElement* element = sessionLabelArrayMap.FindMapElement( reinterpret_cast<XrSession&>( queryObjectInfo.handle ) );

                if ( !element )
                {
                    continue;
                }

                for ( int32 a = element->sessionLabelArray.GetArrayElementCount( ) - 1; a >= 0; a-- )
                {
                    labelArray.AppendArrayElement( element->sessionLabelArray[ a ].GetDebugUtilsLabel( ) );
                }
            }
        }
    }

    DebugUtilsMessengerCallbackDataOwner DebugUtilsData::LiftCallbackData( const XrDebugUtilsMessengerCallbackDataEXT* callbackData )
    {
        // Checks if the callbackData contains objects we are already tracking. If not, just return the
        // callbackData. If it does, then return both our tracked objects and the objects in the callbackData.
        // The encapsulator fully owns all string and array data.

        DebugUtilsMessengerCallbackDataOwner encapsulator;

        auto&                                out = encapsulator.xrDebugUtilsMessengerCallbackData;

        out.type                                 = XR_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT;
        out.next                                 = nullptr;
        out.messageId                            = nullptr;
        out.functionName                         = nullptr;
        out.message                              = nullptr;
        out.objectCount                          = 0;
        out.objects                              = nullptr;
        out.sessionLabelCount                    = 0;
        out.sessionLabels                        = nullptr;

        encapsulator.messageId                   = callbackData->messageId ? callbackData->messageId : "";
        encapsulator.functionName                = callbackData->functionName ? callbackData->functionName : "";
        encapsulator.message                     = callbackData->message ? callbackData->message : "";

        out.messageId                            = static_cast<const char*>( encapsulator.messageId );
        out.functionName                         = static_cast<const char*>( encapsulator.functionName );
        out.message                              = static_cast<const char*>( encapsulator.message );

        auto& newObjectArray                     = encapsulator.xrDebugUtilsObjectNameInfoArray;
        auto& ownedNames                         = encapsulator.ownedObjectNames;

        for ( uint32_t a = 0; a < callbackData->objectCount; a++ )
        {
            const auto&                   src  = callbackData->objects[ a ];

            XrDebugUtilsObjectNameInfoEXT dst  = src;

            const char*                   name = src.objectName ? src.objectName : "";
            ownedNames.AppendArrayElement( name );
            dst.objectName = static_cast<const char*>( ownedNames[ ownedNames.GetArrayElementCount( ) - 1 ] );

            newObjectArray.AppendArrayElement( dst );
        }

        if ( newObjectArray.GetArrayElementCount( ) > 0 )
        {
            out.objectCount = newObjectArray.GetArrayElementCount( );
            out.objects     = newObjectArray.begin( );
        }

        if ( this->objectInfoArray.GetArrayElementCount( ) == 0 || callbackData->objectCount == 0 )
        {
            return encapsulator;
        }

        auto& newLabelArray       = encapsulator.xrDebugUtilsLabelArray;
        auto& ownedLabelNames     = encapsulator.ownedLabelNames;

        bool  atLeastOneNameFound = false;

        for ( uint32_t a = 0; a < callbackData->objectCount; a++ )
        {
            const auto& queryObjectInfo  = callbackData->objects[ a ];

            int32       id               = this->FindObject( queryObjectInfo.objectHandle, queryObjectInfo.objectType );
            atLeastOneNameFound         |= ( id >= 0 );

            if ( queryObjectInfo.objectType == XR_OBJECT_TYPE_SESSION )
            {
                SessionLabelArrayMapElement* element = this->sessionLabelArrayMap.FindMapElement( reinterpret_cast<XrSession>( queryObjectInfo.objectHandle ) );

                if ( !element )
                {
                    continue;
                }

                for ( int32 b = element->sessionLabelArray.GetArrayElementCount( ) - 1; b >= 0; b-- )
                {
                    const auto&          srcLabel  = element->sessionLabelArray[ b ].GetDebugUtilsLabel( );

                    XrDebugUtilsLabelEXT dstLabel  = srcLabel;

                    const char*          labelName = srcLabel.labelName ? srcLabel.labelName : "";
                    ownedLabelNames.AppendArrayElement( labelName );
                    dstLabel.labelName = static_cast<const char*>( ownedLabelNames[ ownedLabelNames.GetArrayElementCount( ) - 1 ] );

                    newLabelArray.AppendArrayElement( dstLabel );
                }
            }
        }

        if ( !atLeastOneNameFound && newLabelArray.GetArrayElementCount( ) == 0 )
        {
            return encapsulator;
        }

        for ( auto& newObjectInfo : newObjectArray )
        {
            for ( const auto& thisObjectInfo : this->objectInfoArray )
            {
                if ( ( thisObjectInfo.handle == newObjectInfo.objectHandle ) && ( thisObjectInfo.type == newObjectInfo.objectType ) )
                {
                    encapsulator.ownedObjectNames.AppendArrayElement( thisObjectInfo.name );
                    newObjectInfo.objectName = static_cast<const char*>( encapsulator.ownedObjectNames[ encapsulator.ownedObjectNames.GetArrayElementCount( ) - 1 ] );
                    break;
                }
            }
        }

        out.objectCount       = newObjectArray.GetArrayElementCount( );
        out.objects           = newObjectArray.GetArrayElementCount( ) > 0 ? newObjectArray.begin( ) : nullptr;

        out.sessionLabelCount = newLabelArray.GetArrayElementCount( );
        out.sessionLabels     = newLabelArray.GetArrayElementCount( ) > 0 ? newLabelArray.begin( ) : nullptr;

        return encapsulator;
    }

    int32 DebugUtilsData::FindObject( uint64_t handle, XrObjectType type )
    {

        for ( int32 a = 0; a != this->objectInfoArray.GetArrayElementCount( ); a++ )
        {
            const auto& thisObjectInfo = this->objectInfoArray[ a ];

            if ( ( thisObjectInfo.handle == handle ) && ( thisObjectInfo.type == type ) )
            {
                return a;
            }
        }
        return -1;
    }


    SessionLabelArrayMapElement::SessionLabelArrayMapElement( XrSession session ) : key( session )
    {
    }


    Array<XrSessionLabel>* DebugUtilsData::PrepareSessionLabelArray( XrSession session )
    {
        Array<XrSessionLabel>*       sessionLabelArray = nullptr;
        SessionLabelArrayMapElement* element           = this->sessionLabelArrayMap.FindMapElement( session );

        if ( element == nullptr )
        {
            auto sessionLabelArrayMapElement = new SessionLabelArrayMapElement( session );
            sessionLabelArray                = &sessionLabelArrayMapElement->sessionLabelArray;

            sessionLabelArrayMap.InsertMapElement( sessionLabelArrayMapElement );
        }
        else
        {
            sessionLabelArray = &element->sessionLabelArray;
            RemoveLastElementIfIndividual( sessionLabelArray );
        }

        return sessionLabelArray;
    }

    void DebugUtilsData::InsertLabel( XrSession session, const XrDebugUtilsLabelEXT& debugUtilsLabel )
    {
        Array<XrSessionLabel>* sessionLabelArray = PrepareSessionLabelArray( session );

        sessionLabelArray->AppendArrayElement( XrSessionLabel( debugUtilsLabel, XrSessionLabel::Type::Individual ) );
    }

    void DebugUtilsData::BeginLabelRegion( XrSession session, const XrDebugUtilsLabelEXT& debugUtilsLabel )
    {
        Array<XrSessionLabel>* sessionLabelArray = PrepareSessionLabelArray( session );

        sessionLabelArray->AppendArrayElement( XrSessionLabel( debugUtilsLabel, XrSessionLabel::Type::Region ) );
    }

    void DebugUtilsData::EndLabelRegion( XrSession session )
    {
        SessionLabelArrayMapElement* element = this->sessionLabelArrayMap.FindMapElement( session );

        if ( element == nullptr )
        {
            return;
        }

        Array<XrSessionLabel>* sessionLabelArray = &element->sessionLabelArray;

        // An individual label will be removed regardless (only the last element can be individual)
        RemoveLastElementIfIndividual( sessionLabelArray );

        sessionLabelArray->RemoveLastArrayElement( );
    }

    void DebugUtilsData::RemoveSessionLabelsAndSessionObjectInfo( XrSession session )
    {
        for ( SessionLabelArrayMapElement* element = this->sessionLabelArrayMap.FindMapElement( session ); element != nullptr; element = this->sessionLabelArrayMap.FindMapElement( session ) )
        {
            this->sessionLabelArrayMap.RemoveMapElement( element );
        }

        int32 a = 0;

        while ( a < this->objectInfoArray.GetArrayElementCount( ) )
        {
            auto& objectInfo = objectInfoArray[ a ];

            if ( objectInfo.type == XR_OBJECT_TYPE_SESSION && objectInfo.handle == reinterpret_cast<uint64_t>( session ) )
            {
                this->objectInfoArray.RemoveArrayElement( a );

                continue;
            }

            a++;
        }
    }

} // namespace VortexXr
