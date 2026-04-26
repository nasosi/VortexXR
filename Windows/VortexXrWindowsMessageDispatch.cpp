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

#include "VortexXrWindowsDebuggerLogger.h"
#include <VortexXrMessageDispatch.h>
#include <VortexXrUtility.h>

namespace VortexXr
{


    MessageDispatcher::MessageDispatcher( )
    {

#ifdef VORTEXXR_ENABLE_MSVC_WINDOWS_DEBUGGER_LOGGER

        AppendLogger( new DebuggerLogger( allMessageSeverities, allMessageTypes ) );

#endif

        SetupCommonLoggers( );
    }

} // namespace VortexXr