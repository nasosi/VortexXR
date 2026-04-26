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

#include "windows.h"

#define VORTEXXR_WINDOWS             1
#define VORTEXXR_FOLDER_SEPARATOR    '\\'
#define VORTEXXR_RECQUIRED_LOCK_HELD _Requires_lock_held_( sRWLock )
#define VORTEXXR_RWLOCK_TYPE         SRWLOCK

#ifdef VORTEXXR_EXPORT 

#    define VORTEXXR_EXPORT_API __declspec( dllexport )

#else

#    ifdef VORTEXXR_SHARED 

#        define VORTEXXR_EXPORT_API __declspec( dllimport )

#    else // Static build

#        define VORTEXXR_EXPORT_API

#    endif

#endif

namespace VortexXr
{

    using LibraryHandle = HMODULE;

    template <class PFN_Type> PFN_Type OsGetProcedureAddress( LibraryHandle libraryHandle, const char* procedureName )
    {
        return reinterpret_cast<PFN_Type>( GetProcAddress( libraryHandle, procedureName ) );
    }

} // namespace VortexXr