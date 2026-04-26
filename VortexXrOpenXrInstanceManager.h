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
#include "VortexXrMessageDispatch.h"
#include "VortexXrPlatform.h"
#include "VortexXrRuntimeManager.h"
#include "VortexXrUtility.h"

#include "TSArray.h"
#include "TSMap.h"
#include "TSTools.h"

#include "openxr/openxr.h"
#include "openxr/openxr_loader_negotiation.h"

namespace VortexXr
{

    class OpenXrApiLayerManifest;
    class ApiLayer;


    class OpenXrLoader
    {
        private:

            SharedMutex                       mutex;

            XrInstance                        activeXrInstance           = nullptr;
            PFN_xrGetInstanceProcAddr         topmostGetInstanceProcAddr = nullptr;
            XrDebugUtilsMessengerEXT          defaultDebugUtilsMessenger = nullptr;

            Array<Holder<ApiLayer>>           apiLayerArray;

            OpenXrRuntimeManager              openXrRuntime;

            Holder<CoreXrFunctionPointerList> xrFunctionPointerList;
            Array<XrExtensionProperties>      loaderSupportedExtensionPropertiesArray;
            Array<XrExtensionProperties>      enabledExtensionsExtensionPropertiesArray;

            XrResult                          Initialize( const char* functionNameToReport );
            void                              Uninitialize_AssumesLocked( );
            void                              Uninitialize( );

            bool                              LoadAndAppendApiLayerLibraries( const char* callingXrFunctionName, const Array<Holder<OpenXrApiLayerManifest>>& apiLayerManifestArray );


        public:

            static constexpr uint32    loaderInfoStructVersion  = XR_LOADER_INFO_STRUCT_VERSION;
            static constexpr uint32    runtimeInfoStructVersion = XR_RUNTIME_INFO_STRUCT_VERSION;
            static constexpr uint32    minInterfaceVersion      = 1;
            static constexpr uint32    maxInterfaceVersion      = XR_CURRENT_LOADER_RUNTIME_VERSION;
            static constexpr uint16    apiMajorVersion          = XR_VERSION_MAJOR( XR_CURRENT_API_VERSION );
            static constexpr uint16    apiMinorVersion          = XR_VERSION_MINOR( XR_CURRENT_API_VERSION );
            static constexpr XrVersion apiVersion               = XR_CURRENT_API_VERSION;
            static constexpr XrVersion apiSemanticVersion       = XR_MAKE_VERSION( apiMajorVersion, apiMinorVersion, 0 );
            static constexpr XrVersion minApiVersion            = XR_MAKE_VERSION( 1, 0, 0 );
            static constexpr XrVersion maxApiVersion            = XR_MAKE_VERSION( apiMajorVersion, 0x400 - 1, 0x1000 - 1 );

            MessageDispatcher messageDispatcher;

        public:

            OpenXrLoader( );
            ~OpenXrLoader( );

            bool                               IsInitialized( );
            XrResult                           EnsureIsInitialized( const char* functionNameToReport );
            bool                               IsXrInstanceActive( );
            const XrInstance&                  GetActiveXrInstance( ) const;
            OpenXrRuntimeManager&              GetOpenXrRuntimeManager( );
            Holder<CoreXrFunctionPointerList>& GetXrFunctionPointerList( );
            bool                               LoaderSupportsExtension( const char* extensionName );
            bool                               IsExtensionEnabled( const char* extensionName );

            //
            XrResult xrCreateInstance( const XrInstanceCreateInfo* createInfo, XrInstance* xrInstance );
            XrResult xrDestroyInstance( XrInstance instance );
            XrResult xrEnumerateInstanceExtensionProperties( const char* layerName, uint32_t propertyCapacityInput, uint32_t* propertyCountOutput, XrExtensionProperties* properties );
            XrResult xrGetInstanceProcAddr( const char* functionName, PFN_xrVoidFunction* function );
            XrResult xrEnumerateApiLayerProperties( uint32_t propertyCapacityInput, uint32_t* propertyCountOutput, XrApiLayerProperties* properties );


            XrResult xrSetDebugUtilsObjectNameEXT_Terminator( XrInstance instance, const XrDebugUtilsObjectNameInfoEXT* nameInfo );
            XrResult xrSubmitDebugUtilsMessageEXT_Terminator( XrInstance                                  instance,
                                                              XrDebugUtilsMessageSeverityFlagsEXT         messageSeverity,
                                                              XrDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                                              const XrDebugUtilsMessengerCallbackDataEXT* callbackData );


            XrResult xrCreateDebugUtilsMessengerEXT_Terminator( XrInstance instance, const XrDebugUtilsMessengerCreateInfoEXT* createInfo, XrDebugUtilsMessengerEXT* messenger );
            XrResult xrDestroyDebugUtilsMessengerEXT_Terminator( XrDebugUtilsMessengerEXT messenger );


            XrResult xrCreateDebugUtilsMessengerEXT_Trampoline( XrInstance instance, const XrDebugUtilsMessengerCreateInfoEXT* createInfo, XrDebugUtilsMessengerEXT* messenger );
            XrResult xrDestroyDebugUtilsMessengerEXT_Trampoline( XrDebugUtilsMessengerEXT messenger );
            XrResult xrSessionInsertDebugUtilsLabelEXT_Trampoline( XrSession session, const XrDebugUtilsLabelEXT* labelInfo );
            XrResult xrSetDebugUtilsObjectNameEXT_Trampoline( XrInstance instance, const XrDebugUtilsObjectNameInfoEXT* objectNameInfo );
            XrResult xrSubmitDebugUtilsMessageEXT_Trampoline( XrInstance                                  instance,
                                                              XrDebugUtilsMessageSeverityFlagsEXT         messageSeverity,
                                                              XrDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                                              const XrDebugUtilsMessengerCallbackDataEXT* callbackData );
            XrResult xrSessionBeginDebugUtilsLabelRegionEXT_Trampoline( XrSession session, const XrDebugUtilsLabelEXT* labelInfo );
            XrResult xrSessionEndDebugUtilsLabelRegionEXT_Trampoline( XrSession session );
    };

    inline OpenXrLoader TheOpenXrInstanceManager;

} // namespace VortexXr