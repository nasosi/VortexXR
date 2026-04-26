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

#include <dlfcn.h>
#include <pthread.h>

#define VORTEXXR_LINUX
#define VORTEXXR_FOLDER_SEPARATOR '/'
#define VORTEXXR_RECQUIRED_LOCK_HELD
#define VORTEXXR_RWLOCK_TYPE pthread_rwlock_t
#define VORTEXXR_EXPORT_API           __attribute__( ( visibility( "default" ) ) )

namespace VortexXr
{
    using LibraryHandle = void*;

    template <class PFN_Type> PFN_Type OsGetProcedureAddress( LibraryHandle libraryHandle, const char* procedureName )
    {
        return reinterpret_cast<PFN_Type>( dlsym( libraryHandle, procedureName ) );
    }

} // namespace VortexXr