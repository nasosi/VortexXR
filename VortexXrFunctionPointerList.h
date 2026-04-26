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

#include <TSMap.h>

#include <openxr/openxr.h>


namespace VortexXr
{

    struct CoreXrFunctionPointerList
    {
        private:

            template <class PFN> XrResult SetInstanceProcAddr( XrInstance xrInstance, const char* functionName, PFN& functionPointer )
            {
                return GetInstanceProcAddr( xrInstance, functionName, reinterpret_cast<PFN_xrVoidFunction*>( &functionPointer ) );
            }

            CoreXrFunctionPointerList( ) = delete;

        public:

            // OpenXr 1.0
            PFN_xrAcquireSwapchainImage                AcquireSwapchainImage                = nullptr;
            PFN_xrApplyHapticFeedback                  ApplyHapticFeedback                  = nullptr;
            PFN_xrAttachSessionActionSets              AttachSessionActionSets              = nullptr;
            PFN_xrBeginFrame                           BeginFrame                           = nullptr;
            PFN_xrBeginSession                         BeginSession                         = nullptr;
            PFN_xrCreateAction                         CreateAction                         = nullptr;
            PFN_xrCreateActionSet                      CreateActionSet                      = nullptr;
            PFN_xrCreateActionSpace                    CreateActionSpace                    = nullptr;
            PFN_xrCreateInstance                       CreateInstance                       = nullptr;
            PFN_xrCreateReferenceSpace                 CreateReferenceSpace                 = nullptr;
            PFN_xrCreateSession                        CreateSession                        = nullptr;
            PFN_xrCreateSwapchain                      CreateSwapchain                      = nullptr;
            PFN_xrDestroyAction                        DestroyAction                        = nullptr;
            PFN_xrDestroyActionSet                     DestroyActionSet                     = nullptr;
            PFN_xrDestroyInstance                      DestroyInstance                      = nullptr;
            PFN_xrDestroySession                       DestroySession                       = nullptr;
            PFN_xrDestroySpace                         DestroySpace                         = nullptr;
            PFN_xrDestroySwapchain                     DestroySwapchain                     = nullptr;
            PFN_xrEndFrame                             EndFrame                             = nullptr;
            PFN_xrEndSession                           EndSession                           = nullptr;
            PFN_xrEnumerateApiLayerProperties          EnumerateApiLayerProperties          = nullptr;
            PFN_xrEnumerateBoundSourcesForAction       EnumerateBoundSourcesForAction       = nullptr;
            PFN_xrEnumerateEnvironmentBlendModes       EnumerateEnvironmentBlendModes       = nullptr;
            PFN_xrEnumerateInstanceExtensionProperties EnumerateInstanceExtensionProperties = nullptr;
            PFN_xrEnumerateReferenceSpaces             EnumerateReferenceSpaces             = nullptr;
            PFN_xrEnumerateSwapchainFormats            EnumerateSwapchainFormats            = nullptr;
            PFN_xrEnumerateSwapchainImages             EnumerateSwapchainImages             = nullptr;
            PFN_xrEnumerateViewConfigurations          EnumerateViewConfigurations          = nullptr;
            PFN_xrEnumerateViewConfigurationViews      EnumerateViewConfigurationViews      = nullptr;
            PFN_xrGetActionStateBoolean                GetActionStateBoolean                = nullptr;
            PFN_xrGetActionStateFloat                  GetActionStateFloat                  = nullptr;
            PFN_xrGetActionStatePose                   GetActionStatePose                   = nullptr;
            PFN_xrGetActionStateVector2f               GetActionStateVector2f               = nullptr;
            PFN_xrGetCurrentInteractionProfile         GetCurrentInteractionProfile         = nullptr;
            PFN_xrGetInputSourceLocalizedName          GetInputSourceLocalizedName          = nullptr;
            PFN_xrGetInstanceProcAddr                  GetInstanceProcAddr                  = nullptr;
            PFN_xrGetInstanceProperties                GetInstanceProperties                = nullptr;
            PFN_xrGetReferenceSpaceBoundsRect          GetReferenceSpaceBoundsRect          = nullptr;
            PFN_xrGetSystem                            GetSystem                            = nullptr;
            PFN_xrGetSystemProperties                  GetSystemProperties                  = nullptr;
            PFN_xrGetViewConfigurationProperties       GetViewConfigurationProperties       = nullptr;
            PFN_xrLocateSpace                          LocateSpace                          = nullptr;
            PFN_xrLocateViews                          LocateViews                          = nullptr;
            PFN_xrPathToString                         PathToString                         = nullptr;
            PFN_xrPollEvent                            PollEvent                            = nullptr;
            PFN_xrReleaseSwapchainImage                ReleaseSwapchainImage                = nullptr;
            PFN_xrRequestExitSession                   RequestExitSession                   = nullptr;
            PFN_xrResultToString                       ResultToString                       = nullptr;
            PFN_xrStopHapticFeedback                   StopHapticFeedback                   = nullptr;
            PFN_xrStringToPath                         StringToPath                         = nullptr;
            PFN_xrStructureTypeToString                StructureTypeToString                = nullptr;
            PFN_xrSuggestInteractionProfileBindings    SuggestInteractionProfileBindings    = nullptr;
            PFN_xrSyncActions                          SyncActions                          = nullptr;
            PFN_xrWaitFrame                            WaitFrame                            = nullptr;
            PFN_xrWaitSwapchainImage                   WaitSwapchainImage                   = nullptr;


            // XR_EXT_debug_utils Extension 1.0
            PFN_xrCreateDebugUtilsMessengerEXT         CreateDebugUtilsMessengerEXT         = nullptr;
            PFN_xrDestroyDebugUtilsMessengerEXT        DestroyDebugUtilsMessengerEXT        = nullptr;
            PFN_xrSessionBeginDebugUtilsLabelRegionEXT SessionBeginDebugUtilsLabelRegionEXT = nullptr;
            PFN_xrSessionEndDebugUtilsLabelRegionEXT   SessionEndDebugUtilsLabelRegionEXT   = nullptr;
            PFN_xrSessionInsertDebugUtilsLabelEXT      SessionInsertDebugUtilsLabelEXT      = nullptr;
            PFN_xrSetDebugUtilsObjectNameEXT           SetDebugUtilsObjectNameEXT           = nullptr;
            PFN_xrSubmitDebugUtilsMessageEXT           SubmitDebugUtilsMessageEXT           = nullptr;


            // OpenXR 1.1
            PFN_xrLocateSpaces                          LocateSpaces                        = nullptr;


            CoreXrFunctionPointerList( XrInstance xrInstance, PFN_xrGetInstanceProcAddr getInstanceProcAddr );
    };

    struct FunctionPointerListMapElement : public Terathon::MapElement<FunctionPointerListMapElement>
    {

            XrInstance                xrInstance;
            CoreXrFunctionPointerList xrFunctionPointerList;

            using KeyType = XrInstance;

            FunctionPointerListMapElement( KeyType xrInstance, CoreXrFunctionPointerList functionPointers );

            KeyType GetKey( ) const
            {
                return xrInstance;
            }
    };


} // namespace VortexXr
