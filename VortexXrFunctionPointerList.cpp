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

#include "VortexXrFunctionPointerList.h"

namespace VortexXr
{

    CoreXrFunctionPointerList::CoreXrFunctionPointerList( XrInstance xrInstance, PFN_xrGetInstanceProcAddr getInstanceProcAddr ) : GetInstanceProcAddr( getInstanceProcAddr )
    {
        // OpenXR 1.0
        SetInstanceProcAddr( xrInstance, "xrAcquireSwapchainImage", AcquireSwapchainImage );
        SetInstanceProcAddr( xrInstance, "xrApplyHapticFeedback", ApplyHapticFeedback );
        SetInstanceProcAddr( xrInstance, "xrAttachSessionActionSets", AttachSessionActionSets );
        SetInstanceProcAddr( xrInstance, "xrBeginFrame", BeginFrame );
        SetInstanceProcAddr( xrInstance, "xrBeginSession", BeginSession );
        SetInstanceProcAddr( xrInstance, "xrCreateAction", CreateAction );
        SetInstanceProcAddr( xrInstance, "xrCreateActionSet", CreateActionSet );
        SetInstanceProcAddr( xrInstance, "xrCreateActionSpace", CreateActionSpace );
        SetInstanceProcAddr( xrInstance, "xrCreateInstance", CreateInstance );
        SetInstanceProcAddr( xrInstance, "xrCreateReferenceSpace", CreateReferenceSpace );
        SetInstanceProcAddr( xrInstance, "xrCreateSession", CreateSession );
        SetInstanceProcAddr( xrInstance, "xrCreateSwapchain", CreateSwapchain );
        SetInstanceProcAddr( xrInstance, "xrDestroyAction", DestroyAction );
        SetInstanceProcAddr( xrInstance, "xrDestroyActionSet", DestroyActionSet );
        SetInstanceProcAddr( xrInstance, "xrDestroyInstance", DestroyInstance );
        SetInstanceProcAddr( xrInstance, "xrDestroySession", DestroySession );
        SetInstanceProcAddr( xrInstance, "xrDestroySpace", DestroySpace );
        SetInstanceProcAddr( xrInstance, "xrDestroySwapchain", DestroySwapchain );
        SetInstanceProcAddr( xrInstance, "xrEndFrame", EndFrame );
        SetInstanceProcAddr( xrInstance, "xrEndSession", EndSession );
        SetInstanceProcAddr( xrInstance, "xrEnumerateBoundSourcesForAction", EnumerateBoundSourcesForAction );
        SetInstanceProcAddr( xrInstance, "xrEnumerateEnvironmentBlendModes", EnumerateEnvironmentBlendModes );
        SetInstanceProcAddr( xrInstance, "xrEnumerateReferenceSpaces", EnumerateReferenceSpaces );
        SetInstanceProcAddr( xrInstance, "xrEnumerateSwapchainFormats", EnumerateSwapchainFormats );
        SetInstanceProcAddr( xrInstance, "xrEnumerateSwapchainImages", EnumerateSwapchainImages );
        SetInstanceProcAddr( xrInstance, "xrEnumerateViewConfigurations", EnumerateViewConfigurations );
        SetInstanceProcAddr( xrInstance, "xrEnumerateViewConfigurationViews", EnumerateViewConfigurationViews );
        SetInstanceProcAddr( xrInstance, "xrGetActionStateBoolean", GetActionStateBoolean );
        SetInstanceProcAddr( xrInstance, "xrGetActionStateFloat", GetActionStateFloat );
        SetInstanceProcAddr( xrInstance, "xrGetActionStatePose", GetActionStatePose );
        SetInstanceProcAddr( xrInstance, "xrGetActionStateVector2f", GetActionStateVector2f );
        SetInstanceProcAddr( xrInstance, "xrGetCurrentInteractionProfile", GetCurrentInteractionProfile );
        SetInstanceProcAddr( xrInstance, "xrGetInputSourceLocalizedName", GetInputSourceLocalizedName );
        SetInstanceProcAddr( xrInstance, "xrGetInstanceProperties", GetInstanceProperties );
        SetInstanceProcAddr( xrInstance, "xrGetReferenceSpaceBoundsRect", GetReferenceSpaceBoundsRect );
        SetInstanceProcAddr( xrInstance, "xrGetSystem", GetSystem );
        SetInstanceProcAddr( xrInstance, "xrGetSystemProperties", GetSystemProperties );
        SetInstanceProcAddr( xrInstance, "xrGetViewConfigurationProperties", GetViewConfigurationProperties );
        SetInstanceProcAddr( xrInstance, "xrLocateSpace", LocateSpace );
        SetInstanceProcAddr( xrInstance, "xrLocateViews", LocateViews );
        SetInstanceProcAddr( xrInstance, "xrPathToString", PathToString );
        SetInstanceProcAddr( xrInstance, "xrPollEvent", PollEvent );
        SetInstanceProcAddr( xrInstance, "xrReleaseSwapchainImage", ReleaseSwapchainImage );
        SetInstanceProcAddr( xrInstance, "xrRequestExitSession", RequestExitSession );
        SetInstanceProcAddr( xrInstance, "xrResultToString", ResultToString );
        SetInstanceProcAddr( xrInstance, "xrStopHapticFeedback", StopHapticFeedback );
        SetInstanceProcAddr( xrInstance, "xrStringToPath", StringToPath );
        SetInstanceProcAddr( xrInstance, "xrStructureTypeToString", StructureTypeToString );
        SetInstanceProcAddr( xrInstance, "xrSuggestInteractionProfileBindings", SuggestInteractionProfileBindings );
        SetInstanceProcAddr( xrInstance, "xrSyncActions", SyncActions );
        SetInstanceProcAddr( xrInstance, "xrWaitFrame", WaitFrame );
        SetInstanceProcAddr( xrInstance, "xrWaitSwapchainImage", WaitSwapchainImage );

        // Debug Utils 1.0
        SetInstanceProcAddr( xrInstance, "xrCreateDebugUtilsMessengerEXT", CreateDebugUtilsMessengerEXT );
        SetInstanceProcAddr( xrInstance, "xrDestroyDebugUtilsMessengerEXT", DestroyDebugUtilsMessengerEXT );
        SetInstanceProcAddr( xrInstance, "xrSessionBeginDebugUtilsLabelRegionEXT", SessionBeginDebugUtilsLabelRegionEXT );
        SetInstanceProcAddr( xrInstance, "xrSessionEndDebugUtilsLabelRegionEXT", SessionEndDebugUtilsLabelRegionEXT );
        SetInstanceProcAddr( xrInstance, "xrSessionInsertDebugUtilsLabelEXT", SessionInsertDebugUtilsLabelEXT );
        SetInstanceProcAddr( xrInstance, "XrSetDebugUtilsObjectNameEXT", SetDebugUtilsObjectNameEXT );
        SetInstanceProcAddr( xrInstance, "xrSubmitDebugUtilsMessageEXT", SubmitDebugUtilsMessageEXT );

        // OpenXR 1.1
        SetInstanceProcAddr( xrInstance, "xrLocateSpaces", LocateSpaces );

        // FUTURE: ADDITIONAL EXTENSION COMMANDS
    }

    FunctionPointerListMapElement::FunctionPointerListMapElement( KeyType instance, CoreXrFunctionPointerList functionPointers ) : xrInstance( instance ), xrFunctionPointerList( functionPointers )
    {
    }

} // namespace VortexXr