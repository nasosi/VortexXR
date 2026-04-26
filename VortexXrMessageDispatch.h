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

#pragma once

#include "VortexXrUtility.h"

#include <TSMap.h>
#include <TSString.h>
#include <TSTools.h>

#include <openxr/openxr.h>

namespace VortexXr
{

    using namespace Terathon;


    struct OpenXrObjectInfo
    {
            uint64_t     handle;
            XrObjectType type;
            String<>     name;
    };

    struct DebugUtilsMessengerCallbackDataOwner
    {
            DebugUtilsMessengerCallbackDataOwner( );

            String<>                             messageId;
            String<>                             functionName;
            String<>                             message;

            Array<String<>>                      ownedObjectNames;
            Array<String<>>                      ownedLabelNames;

            Array<XrDebugUtilsObjectNameInfoEXT> xrDebugUtilsObjectNameInfoArray;
            Array<XrDebugUtilsLabelEXT>          xrDebugUtilsLabelArray;

            XrDebugUtilsMessengerCallbackDataEXT xrDebugUtilsMessengerCallbackData;
    };


    namespace Detail
    {

        template <class T, class... Args> void CreateObjectInfoArrayImpl( Array<OpenXrObjectInfo>& objectInfoArray, const T& handle, const XrObjectType& type, Args... args )
        {
            objectInfoArray.AppendArrayElement( OpenXrObjectInfo { reinterpret_cast<uint64_t>( handle ), type } );

            if constexpr ( sizeof...( args ) != 0 )
            {
                Detail::CreateObjectInfoArrayImpl( objectInfoArray, args... );
            }
        }

    } // namespace Detail

    template <class T, class... Args> Array<OpenXrObjectInfo> CreateObjectInfoArray( const T& handle, const XrObjectType& type, Args... args )
    {
        Array<OpenXrObjectInfo> objectInfoArray;

        objectInfoArray.AppendArrayElement( OpenXrObjectInfo { reinterpret_cast<uint64_t>( handle ), type } );

        if constexpr ( sizeof...( args ) != 0 )
        {
            Detail::CreateObjectInfoArrayImpl( objectInfoArray, args... );
        }

        return objectInfoArray;
    }

    struct XrSessionLabel
    {

        public:

            enum class Type : uint8
            {
                Region,
                Individual
            };


        private:

            String<>             name;
            XrDebugUtilsLabelEXT debugUtilsLabel;
            Type                 type;

            XrSessionLabel( ) = delete;


        public:

            XrSessionLabel( const XrDebugUtilsLabelEXT& debugUtilsLabel, Type type );
            XrSessionLabel( const XrSessionLabel& other );

            const Type&                 GetType( ) const;

            const XrDebugUtilsLabelEXT& GetDebugUtilsLabel( ) const;
    };


    class SessionLabelArrayMapElement : public MapElement<SessionLabelArrayMapElement>
    {
        private:

            XrSession key;


        public:

            using KeyType = XrSession;

            Array<XrSessionLabel> sessionLabelArray;

            SessionLabelArrayMapElement( XrSession key );

            const KeyType& GetKey( ) const
            {
                return key;
            }
    };


    struct Log
    {
        public:

            static bool Verbose( const char*                    functionName,
                                 const char*                    message,
                                 const Array<OpenXrObjectInfo>& objectInfoArray = Array<OpenXrObjectInfo>( ),
                                 unsigned int                   callingLine     = __builtin_LINE( ),
                                 const char*                    callingFileName = __builtin_FILE( ) );

            static bool Info( const char*                    functionName,
                              const char*                    message,
                              const Array<OpenXrObjectInfo>& objectInfoArray = Array<OpenXrObjectInfo>( ),
                              unsigned int                   callingLine     = __builtin_LINE( ),
                              const char*                    callingFileName = __builtin_FILE( ) );

            static bool Warning( const char*                    functionName,
                                 const char*                    message,
                                 const Array<OpenXrObjectInfo>& objectInfoArray = Array<OpenXrObjectInfo>( ),
                                 unsigned int                   callingLine     = __builtin_LINE( ),
                                 const char*                    callingFileName = __builtin_FILE( ) );

            static bool Error( const char*                    functionName,
                               const char*                    message,
                               const Array<OpenXrObjectInfo>& objectInfoArray = Array<OpenXrObjectInfo>( ),
                               unsigned int                   callingLine     = __builtin_LINE( ),
                               const char*                    callingFileName = __builtin_FILE( ) );

            static bool ValidationWarning( const char*                    vuid,
                                           const char*                    functionName,
                                           const char*                    message,
                                           const Array<OpenXrObjectInfo>& objectInfoArray = Array<OpenXrObjectInfo>( ),
                                           unsigned int                   callingLine     = __builtin_LINE( ),
                                           const char*                    callingFileName = __builtin_FILE( ) );

            static bool ValidationError( const char*                    vuid,
                                         const char*                    functionName,
                                         const char*                    message,
                                         const Array<OpenXrObjectInfo>& objectInfoArray = Array<OpenXrObjectInfo>( ),
                                         unsigned int                   callingLine     = __builtin_LINE( ),
                                         const char*                    callingFileName = __builtin_FILE( ) );
    };


    class Logger
    {
        private:

            XrDebugUtilsMessageSeverityFlagsEXT messageSeverityFlags;
            XrDebugUtilsMessageTypeFlagsEXT     messageTypeFlags;

        protected:

            String<> FormatXrMessage( XrDebugUtilsMessageSeverityFlagsEXT         messageSeverityFlags,
                                      XrDebugUtilsMessageTypeFlagsEXT             messageTypeFlags,
                                      const XrDebugUtilsMessengerCallbackDataEXT* DebugUtilsMessengerCallbackData );

            bool     Intercepts( XrDebugUtilsMessageSeverityFlagsEXT messageSeverityFlags, XrDebugUtilsMessageTypeFlagsEXT messageTypeFlags );

        public:

            Logger( XrDebugUtilsMessageSeverityFlagsEXT messageSeverityFlags, XrDebugUtilsMessageTypeFlagsEXT messageTypeFlags );

            virtual bool Log( XrDebugUtilsMessageSeverityFlagsEXT         messageSeverityFlags,
                              XrDebugUtilsMessageTypeFlagsEXT             messageTypeFlags,
                              const XrDebugUtilsMessengerCallbackDataEXT* debugUtilsMessengerCallbackDataEXT ) = 0;
    };


    class ConsoleLogger : public Logger
    {
        public:

            using Logger::Logger;

            bool Log( XrDebugUtilsMessageSeverityFlagsEXT         messageSeverityFlags,
                      XrDebugUtilsMessageTypeFlagsEXT             messageTypeFlags,
                      const XrDebugUtilsMessengerCallbackDataEXT* debugUtilsMessengerCallbackDataEXT ) override;
    };


    class DebugUtilsMessenger : public Logger
    {
        private:

            PFN_xrDebugUtilsMessengerCallbackEXT userCallback;
            void*                                userData = nullptr;

            using Logger::Logger;

        public:

            DebugUtilsMessenger( XrDebugUtilsMessageSeverityFlagsEXT  messageSeverityFlags,
                                 XrDebugUtilsMessageTypeFlagsEXT      messageTypeFlags,
                                 void*                                userData,
                                 PFN_xrDebugUtilsMessengerCallbackEXT userCallback );


            bool Log( XrDebugUtilsMessageSeverityFlagsEXT         messageSeverityFlags,
                      XrDebugUtilsMessageTypeFlagsEXT             messageTypeFlags,
                      const XrDebugUtilsMessengerCallbackDataEXT* debugUtilsMessengerCallbackDataEXT ) override;

            bool ProcessDebugUtilsMessage( XrDebugUtilsMessageSeverityFlagsEXT         messageSeverityFlags,
                                           XrDebugUtilsMessageTypeFlagsEXT             messageTypeFlags,
                                           const XrDebugUtilsMessengerCallbackDataEXT* debugUtilsMessengerCallbackDataEXT );
    };


    class DebugUtilsData
    {
        private:

            Array<OpenXrObjectInfo>          objectInfoArray;
            Map<SessionLabelArrayMapElement> sessionLabelArrayMap;

            static void                      RemoveLastElementIfIndividual( Array<XrSessionLabel>* sessionLabelArray );

            int32                            FindObject( uint64_t handle, XrObjectType type );

        public:

            void                                 Uninitialize( );

            void                                 AddObjectInfo( OpenXrObjectInfo objectInfo );
            void                                 RemoveObjectInfo( uint64 handle, XrObjectType type );

            void                                 InsertLabel( XrSession session, const XrDebugUtilsLabelEXT& debugUtilsLabel );

            void                                 BeginLabelRegion( XrSession session, const XrDebugUtilsLabelEXT& debugUtilsLabel );
            void                                 EndLabelRegion( XrSession session );

            void                                 RemoveSessionLabelsAndSessionObjectInfo( XrSession session );

            void                                 PopulateDebugUtilsObjectNamesAndLabelArray( const Array<OpenXrObjectInfo>&        queryObjectInfoArray,
                                                                                             Array<XrDebugUtilsObjectNameInfoEXT>& debugUtilsObjectNameInfoArray,
                                                                                             Array<XrDebugUtilsLabelEXT>&          labelArray );

            Array<XrSessionLabel>*               PrepareSessionLabelArray( XrSession session );

            DebugUtilsMessengerCallbackDataOwner LiftCallbackData( const XrDebugUtilsMessengerCallbackDataEXT* callbackData );
    };


    class MessageDispatcher
    {
        private:

            static const XrDebugUtilsMessageSeverityFlagsEXT allMessageSeverities = XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                                                                    XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

            static const XrDebugUtilsMessageTypeFlagsEXT allMessageTypes = 0xFFFFFFFFUL;


            SharedMutex                                  mutex;
            Array<Holder<Logger>>                        loggerArray;
            Array<DebugUtilsMessenger*>                  debugUtilsMessengerArray;

            void                                         SetupCommonLoggers( );

        public:

            DebugUtilsData debugUtilsData;

            MessageDispatcher( );
            ~MessageDispatcher( );

            void Uninitialize( );

            void AppendLogger( Logger* logger );

            void AppendDebugUtilsMessenger( DebugUtilsMessenger* debugUtilsMessenger );
            bool RemoveDebugUtilsMessenger( DebugUtilsMessenger* debugUtilsMessenger );


            bool SubmitMessage( XrDebugUtilsMessageSeverityFlagsEXT messageSeverity,
                                XrDebugUtilsMessageTypeFlagsEXT     messageType,
                                const char*                         messageId,
                                const char*                         functionName,
                                const char*                         message,
                                const Array<OpenXrObjectInfo>&      objects,
                                unsigned int                        callingLine,
                                const char*                         callingFileName );


            bool SubmitDebugUtilsMessage( XrDebugUtilsMessageSeverityFlagsEXT         messageSeverityFlags,
                                          XrDebugUtilsMessageTypeFlagsEXT             messageTypeFlags,
                                          const XrDebugUtilsMessengerCallbackDataEXT* debugUtilsMessengerCallbackDataEXT );
    };

} // namespace VortexXr