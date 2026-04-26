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

#include "VortexXrFunctionPointerList.h"
#include "VortexXrUtility.h"

namespace VortexXr
{

    class DynamicLibrary;

    class OpenXrRuntimeManager
    {
        private:

            UniqueHolder<DynamicLibrary>       openXrRuntimeLibrary;
            PFN_xrGetInstanceProcAddr          getInstanceProcAddr = nullptr;
            mutable SharedMutex                runtimeStateMutex; // for: openXrRuntimeLibrary, getInstanceProcAddr

            Array<XrExtensionProperties>       extensionPropertiesArray;
            mutable SharedMutex                xrInstance_extensionPropertiesArrayMutex;

            Map<FunctionPointerListMapElement> xrInstance_functionPointerList_Map;
            mutable SharedMutex                xrInstance_functionPointerList_MapMutex;


            OpenXrRuntimeManager( const OpenXrRuntimeManager& )            = delete;
            OpenXrRuntimeManager& operator=( const OpenXrRuntimeManager& ) = delete;

            OpenXrRuntimeManager( OpenXrRuntimeManager&& )                 = delete;
            OpenXrRuntimeManager& operator=( OpenXrRuntimeManager&& )      = delete;

            void                  Reset( );

            bool                  IsRuntimeValid_AssumesLocked( ) const;

            XrResult              LoadRuntimeLibrary( const char* functionNameToReport, const String<>& fileName );

            XrResult              NegotiateRuntimeInterface( const char* functionNameToReport, PFN_xrGetInstanceProcAddr& outGetInstanceProcAddr );

            XrResult              QueryEnumerateFunction( const char* functionNameToReport, PFN_xrGetInstanceProcAddr getInstanceProcAddr, PFN_xrEnumerateInstanceExtensionProperties& outFn );

            XrResult              LoadExtensionProperties( const char* functionNameToReport, PFN_xrEnumerateInstanceExtensionProperties xrEnumerate );

        public:

            OpenXrRuntimeManager( ) = default;

            ~OpenXrRuntimeManager( );

            XrResult                     Load( const char* functionNameToReport, const String<>& libraryPath ); // Should not be called concurrently with itself.
            void                         Unload( );

            bool                         IsRuntimeValid( ) const;

            PFN_xrGetInstanceProcAddr    GetXrGetInstanceProcAddr( ) const;

            Array<XrExtensionProperties> GetXrExtensionPropertiesArray( ) const;

            XrResult                     CreateXrInstance( const char* functionNameToReport, const XrInstanceCreateInfo* createInfo, XrInstance* xrInstance );
            XrResult                     DestroyXrInstance( const char* functionNameToReport, XrInstance xrInstance );

            // Returns a copy of the function pointer list for the given instance, or an empty Optional if not found.
            Optional<CoreXrFunctionPointerList> GetFunctionPointerList( XrInstance instance ) const;
    };

} // namespace VortexXr