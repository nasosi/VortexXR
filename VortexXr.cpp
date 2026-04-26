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

#include "VortexXr.h"
#include "VortexXrMessageDispatch.h"
#include "VortexXrUtility.h"

#include "C4Json.h"

#include "TSData.h"
#include "TSMap.h"


#define VortexXr_VersionMAJOR 0
#define VortexXr_VersionMINOR 5
#define VortexXr_VersionPATCH 1

#define STRN( s )            #s
#define STR( s )             STRN( s )
#define VortexXr_Version      STR( VortexXr_VersionMAJOR ) "." STR( VortexXr_VersionMINOR ) "." STR( VortexXr_VersionPATCH )
#define VortexXrWITHVERSION  "VortexXr v" VortexXr_Version



using namespace Terathon;

namespace VortexXr
{
    const char* GetVersion( )
    {
        return VortexXr_Version;
    }

    const char* GetVortexXrWithVersion( )
    {
        return VortexXrWITHVERSION;
    }


    static const char NoActiveInstanceMsg[] = "There is no active XrInstance.";


} // namespace VortexXr


using namespace VortexXr;


extern "C"
{

    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrGetInstanceProperties( XrInstance instance, XrInstanceProperties* instanceProperties )
    {
         if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrGetInstanceProperties", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }

        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->GetInstanceProperties( instance, instanceProperties );
    }

    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrPollEvent( XrInstance instance, XrEventDataBuffer* eventData )
    {
         if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrPollEvent", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }

        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->PollEvent( instance, eventData );
    }

    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrResultToString( XrInstance instance, XrResult value, char buffer[ XR_MAX_RESULT_STRING_SIZE ] )
    {
         if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            return XR_ERROR_HANDLE_INVALID;
        }

        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->ResultToString( instance, value, buffer );
    }

    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrStructureTypeToString( XrInstance instance, XrStructureType value, char buffer[ XR_MAX_STRUCTURE_NAME_SIZE ] )
    {
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrStructureTypeToString", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }

        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->StructureTypeToString( instance, value, buffer );
    }

    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrGetSystem( XrInstance instance, const XrSystemGetInfo* getInfo, XrSystemId* systemId )
    {
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrGetSystem", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }

        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->GetSystem( instance, getInfo, systemId );
    }

    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrGetSystemProperties( XrInstance instance, XrSystemId systemId, XrSystemProperties* properties )
    {
         if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrGetSystemProperties", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->GetSystemProperties( instance, systemId, properties );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateEnvironmentBlendModes( XrInstance              instance,
                                                                                         XrSystemId              systemId,
                                                                                         XrViewConfigurationType viewConfigurationType,
                                                                                         uint32_t                environmentBlendModeCapacityInput,
                                                                                         uint32_t*               environmentBlendModeCountOutput,
                                                                                         XrEnvironmentBlendMode* environmentBlendModes )
    {
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrEnumerateEnvironmentBlendModes", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->EnumerateEnvironmentBlendModes( instance, systemId, viewConfigurationType, environmentBlendModeCapacityInput, environmentBlendModeCountOutput,
                                                                                          environmentBlendModes );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrCreateSession( XrInstance instance, const XrSessionCreateInfo* createInfo, XrSession* session )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrCreateSession", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->CreateSession( instance, createInfo, session );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrDestroySession( XrSession session )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrDestroySession", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }

        XrResult result = TheOpenXrInstanceManager.GetXrFunctionPointerList( )->DestroySession( session );

        if ( XR_FAILED( result ) )
        {
            return result;
        }

        TheOpenXrInstanceManager.messageDispatcher.debugUtilsData.RemoveSessionLabelsAndSessionObjectInfo( session );

        return result;
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateReferenceSpaces( XrSession session, uint32_t spaceCapacityInput, uint32_t* spaceCountOutput, XrReferenceSpaceType* spaces )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrEnumerateReferenceSpaces", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->EnumerateReferenceSpaces( session, spaceCapacityInput, spaceCountOutput, spaces );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrCreateReferenceSpace( XrSession session, const XrReferenceSpaceCreateInfo* createInfo, XrSpace* space )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrCreateReferenceSpace", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->CreateReferenceSpace( session, createInfo, space );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrGetReferenceSpaceBoundsRect( XrSession session, XrReferenceSpaceType referenceSpaceType, XrExtent2Df* bounds )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrGetReferenceSpaceBoundsRect", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->GetReferenceSpaceBoundsRect( session, referenceSpaceType, bounds );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrCreateActionSpace( XrSession session, const XrActionSpaceCreateInfo* createInfo, XrSpace* space )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrCreateActionSpace", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->CreateActionSpace( session, createInfo, space );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrLocateSpace( XrSpace space, XrSpace baseSpace, XrTime time, XrSpaceLocation* location )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrLocateSpace", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->LocateSpace( space, baseSpace, time, location );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrDestroySpace( XrSpace space )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrDestroySpace", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->DestroySpace( space );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateViewConfigurations( XrInstance               instance,
                                                                                      XrSystemId               systemId,
                                                                                      uint32_t                 viewConfigurationTypeCapacityInput,
                                                                                      uint32_t*                viewConfigurationTypeCountOutput,
                                                                                      XrViewConfigurationType* viewConfigurationTypes )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrEnumerateViewConfigurations", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->EnumerateViewConfigurations( instance, systemId, viewConfigurationTypeCapacityInput, viewConfigurationTypeCountOutput,
                                                                                       viewConfigurationTypes );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrGetViewConfigurationProperties( XrInstance                     instance,
                                                                                         XrSystemId                     systemId,
                                                                                         XrViewConfigurationType        viewConfigurationType,
                                                                                         XrViewConfigurationProperties* configurationProperties )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrGetViewConfigurationProperties", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->GetViewConfigurationProperties( instance, systemId, viewConfigurationType, configurationProperties );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateViewConfigurationViews( XrInstance               instance,
                                                                                          XrSystemId               systemId,
                                                                                          XrViewConfigurationType  viewConfigurationType,
                                                                                          uint32_t                 viewCapacityInput,
                                                                                          uint32_t*                viewCountOutput,
                                                                                          XrViewConfigurationView* views )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrEnumerateViewConfigurationViews", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->EnumerateViewConfigurationViews( instance, systemId, viewConfigurationType, viewCapacityInput, viewCountOutput, views );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateSwapchainFormats( XrSession session, uint32_t formatCapacityInput, uint32_t* formatCountOutput, int64_t* formats )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrEnumerateSwapchainFormats", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->EnumerateSwapchainFormats( session, formatCapacityInput, formatCountOutput, formats );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrCreateSwapchain( XrSession session, const XrSwapchainCreateInfo* createInfo, XrSwapchain* swapchain )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrCreateSwapchain", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->CreateSwapchain( session, createInfo, swapchain );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrDestroySwapchain( XrSwapchain swapchain )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrDestroySwapchain", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->DestroySwapchain( swapchain );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrEnumerateSwapchainImages( XrSwapchain swapchain, uint32_t imageCapacityInput, uint32_t* imageCountOutput, XrSwapchainImageBaseHeader* images )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrEnumerateSwapchainImages", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->EnumerateSwapchainImages( swapchain, imageCapacityInput, imageCountOutput, images );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrAcquireSwapchainImage( XrSwapchain swapchain, const XrSwapchainImageAcquireInfo* acquireInfo, uint32_t* index )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrAcquireSwapchainImage", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->AcquireSwapchainImage( swapchain, acquireInfo, index );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrWaitSwapchainImage( XrSwapchain swapchain, const XrSwapchainImageWaitInfo* waitInfo )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrWaitSwapchainImage", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->WaitSwapchainImage( swapchain, waitInfo );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrReleaseSwapchainImage( XrSwapchain swapchain, const XrSwapchainImageReleaseInfo* releaseInfo )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrReleaseSwapchainImage", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->ReleaseSwapchainImage( swapchain, releaseInfo );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrBeginSession( XrSession session, const XrSessionBeginInfo* beginInfo )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrBeginSession", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->BeginSession( session, beginInfo );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrEndSession( XrSession session )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrEndSession", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->EndSession( session );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrRequestExitSession( XrSession session )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrRequestExitSession", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->RequestExitSession( session );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrWaitFrame( XrSession session, const XrFrameWaitInfo* frameWaitInfo, XrFrameState* frameState )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrWaitFrame", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->WaitFrame( session, frameWaitInfo, frameState );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrBeginFrame( XrSession session, const XrFrameBeginInfo* frameBeginInfo )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrBeginFrame", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->BeginFrame( session, frameBeginInfo );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrEndFrame( XrSession session, const XrFrameEndInfo* frameEndInfo )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrEndFrame", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->EndFrame( session, frameEndInfo );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL
        xrLocateViews( XrSession session, const XrViewLocateInfo* viewLocateInfo, XrViewState* viewState, uint32_t viewCapacityInput, uint32_t* viewCountOutput, XrView* views )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrLocateViews", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->LocateViews( session, viewLocateInfo, viewState, viewCapacityInput, viewCountOutput, views );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrStringToPath( XrInstance instance, const char* pathString, XrPath* path )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrStringToPath", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->StringToPath( instance, pathString, path );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrPathToString( XrInstance instance, XrPath path, uint32_t bufferCapacityInput, uint32_t* bufferCountOutput, char* buffer )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrPathToString", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->PathToString( instance, path, bufferCapacityInput, bufferCountOutput, buffer );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrCreateActionSet( XrInstance instance, const XrActionSetCreateInfo* createInfo, XrActionSet* actionSet )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrCreateActionSet", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->CreateActionSet( instance, createInfo, actionSet );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrDestroyActionSet( XrActionSet actionSet )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrDestroyActionSet", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->DestroyActionSet( actionSet );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrCreateAction( XrActionSet actionSet, const XrActionCreateInfo* createInfo, XrAction* action )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrCreateAction", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->CreateAction( actionSet, createInfo, action );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrDestroyAction( XrAction action )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrDestroyAction", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->DestroyAction( action );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrSuggestInteractionProfileBindings( XrInstance instance, const XrInteractionProfileSuggestedBinding* suggestedBindings )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrSuggestInteractionProfileBindings", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->SuggestInteractionProfileBindings( instance, suggestedBindings );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrAttachSessionActionSets( XrSession session, const XrSessionActionSetsAttachInfo* attachInfo )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrAttachSessionActionSets", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->AttachSessionActionSets( session, attachInfo );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrGetCurrentInteractionProfile( XrSession session, XrPath topLevelUserPath, XrInteractionProfileState* interactionProfile )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrGetCurrentInteractionProfile", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->GetCurrentInteractionProfile( session, topLevelUserPath, interactionProfile );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrGetActionStateBoolean( XrSession session, const XrActionStateGetInfo* getInfo, XrActionStateBoolean* state )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrGetActionStateBoolean", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->GetActionStateBoolean( session, getInfo, state );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrGetActionStateFloat( XrSession session, const XrActionStateGetInfo* getInfo, XrActionStateFloat* state )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrGetActionStateFloat", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->GetActionStateFloat( session, getInfo, state );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrGetActionStateVector2f( XrSession session, const XrActionStateGetInfo* getInfo, XrActionStateVector2f* state )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrGetActionStateVector2f", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->GetActionStateVector2f( session, getInfo, state );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrGetActionStatePose( XrSession session, const XrActionStateGetInfo* getInfo, XrActionStatePose* state )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrGetActionStatePose", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->GetActionStatePose( session, getInfo, state );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrSyncActions( XrSession session, const XrActionsSyncInfo* syncInfo )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrSyncActions", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->SyncActions( session, syncInfo );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL
        xrEnumerateBoundSourcesForAction( XrSession session, const XrBoundSourcesForActionEnumerateInfo* enumerateInfo, uint32_t sourceCapacityInput, uint32_t* sourceCountOutput, XrPath* sources )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrEnumerateBoundSourcesForAction", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->EnumerateBoundSourcesForAction( session, enumerateInfo, sourceCapacityInput, sourceCountOutput, sources );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL
        xrGetInputSourceLocalizedName( XrSession session, const XrInputSourceLocalizedNameGetInfo* getInfo, uint32_t bufferCapacityInput, uint32_t* bufferCountOutput, char* buffer )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrGetInputSourceLocalizedName", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->GetInputSourceLocalizedName( session, getInfo, bufferCapacityInput, bufferCountOutput, buffer );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrApplyHapticFeedback( XrSession session, const XrHapticActionInfo* hapticActionInfo, const XrHapticBaseHeader* hapticFeedback )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrApplyHapticFeedback", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->ApplyHapticFeedback( session, hapticActionInfo, hapticFeedback );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrStopHapticFeedback( XrSession session, const XrHapticActionInfo* hapticActionInfo )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrStopHapticFeedback", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->StopHapticFeedback( session, hapticActionInfo );
    }


    VORTEXXR_EXPORT_API XRAPI_ATTR XrResult XRAPI_CALL xrLocateSpaces( XrSession session, const XrSpacesLocateInfo* locateInfo, XrSpaceLocations* spaceLocations )
    {
        
        if ( !TheOpenXrInstanceManager.IsXrInstanceActive( ) )
        {
            Log::Error( "xrLocateSpaces", NoActiveInstanceMsg );

            return XR_ERROR_HANDLE_INVALID;
        }
        return TheOpenXrInstanceManager.GetXrFunctionPointerList( )->LocateSpaces( session, locateInfo, spaceLocations );
    }
}

