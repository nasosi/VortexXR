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

#include "VortexXrUtility.h"

#include <cstdlib>
#include <TSArray.h>
#include <TSText.h>

#include <dirent.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>

#if defined( __x86_64__ ) && defined( __ILP32__ )
#    define ARCHITECTURE_AND_ABI "x32"
#elif defined( __x86_64__ ) || defined( _M_X64 )
#    define ARCHITECTURE_AND_ABI "x86_64"
#elif defined( _X86_ ) || defined( __i386__ ) || defined( _M_IX86 )
#    define ARCHITECTURE_AND_ABI "i686"
#elif ( defined( __LP64__ ) && defined( __aarch64__ ) ) || defined( _M_ARM64 )
#    define ARCHITECTURE_AND_ABI "aarch64"
#elif defined( __ANDROID__ ) || defined( _M_ARM ) || ( defined( __ARM_ARCH ) && __ARM_ARCH >= 7 && defined( __ARM_PCS_VFP ) )
#    define ARCHITECTURE_AND_ABI "armv7a-vfp"
#elif ( defined( __ARM_ARCH ) && __ARM_ARCH > 5 ) || defined( __ARM_ARCH_5TE__ )
#    define ARCHITECTURE_AND_ABI "armv5te"
#elif defined( __mips64 )
#    define ARCHITECTURE_AND_ABI "mips64"
#elif defined( __mips )
#    define ARCHITECTURE_AND_ABI "mips"
#elif defined( __powerpc64__ ) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#    define ARCHITECTURE_AND_ABI "ppc64"
#elif defined( __powerpc64__ ) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#    define ARCHITECTURE_AND_ABI "ppc64el"
#elif defined( __zarch__ ) || defined( __s390x__ )
#    define ARCHITECTURE_AND_ABI "s390x"
#elif defined( __hppa__ )
#    define ARCHITECTURE_AND_ABI "hppa"
#elif defined( __alpha__ )
#    define ARCHITECTURE_AND_ABI "alpha"
#elif defined( _M_IA64 ) || defined( __ia64__ )
#    define ARCHITECTURE_AND_ABI "ia64"
#elif defined( __m68k__ )
#    define ARCHITECTURE_AND_ABI "m68k"
#elif defined( __riscv_xlen ) && ( __riscv_xlen == 64 )
#    define ARCHITECTURE_AND_ABI "riscv64"
#elif defined( __sparc__ )
#    define ARCHITECTURE_AND_ABI "sparc64"
#else
#    error "VortexXr: Unsupported architecture"
#endif

namespace VortexXr
{
    using namespace Terathon;


    XrResult GetEnvironmentVariableValue( const String<>& variableName, String<>& outValue )
    {
        const char* name = variableName;

#if defined( __GLIBC__ )
        const char* value = secure_getenv( name );
#else
        const char* value = getenv( name );
#endif

        if ( !value || value[ 0 ] == '\0' )
        {
            return XR_ERROR_NAME_INVALID;
        }

        outValue = String<>( value );
        return XR_SUCCESS;
    }


    XrResult AppendFindFiles( const String<>& directory, Array<String<>>& outFiles )
    {
        DIR* dirHandle = opendir( directory );
        if ( !dirHandle )
        {
            return XR_ERROR_FILE_ACCESS_ERROR;
        }

        struct dirent* dir;
        while ( ( dir = readdir( dirHandle ) ) != nullptr )
        {
            if ( dir->d_type == DT_REG || dir->d_type == DT_LNK )
            {
                String<> fileName( dir->d_name );

                if ( fileName.GetStringLength( ) >= 5 && Text::CompareText( static_cast<char*>( fileName ) + fileName.GetStringLength( ) - 5, ".json" ) )
                {
                    outFiles.AppendArrayElement( directory + "/" + fileName );
                }
            }
        }

        closedir( dirHandle );
        return XR_SUCCESS;
    }


    XrResult AppendFindOpenXr1Jsons( const String<>& dirList, Array<String<>>& outFiles )
    {
        Array<String<>> dirs;
        SplitString( dirList, ':', dirs );

        for ( String<> dir : dirs )
        {
            dir += "/openxr/1";
            AppendFindFiles( dir, outFiles );
        }

        return XR_SUCCESS;
    }


    XrResult GetActiveRuntimeJsonPathArray( Array<String<>>& outArray )
    {
        String<> runtimePath;

        if ( XR_SUCCEEDED( GetEnvironmentVariableValue( XR_RUNTIME_JSON_ENVIRONMENT_VARIABLE_NAME, runtimePath ) ) )
        {
            outArray.AppendArrayElement( runtimePath );
            return XR_SUCCESS;
        }

        String<> paths;
        String<> tmp;

        paths  = XR_FAILED( GetEnvironmentVariableValue( "XDG_CONFIG_DIRS", tmp ) ) ? String<>( "/etc/xdg" ) : tmp;

        paths += ":/etc";

        paths += XR_FAILED( GetEnvironmentVariableValue( "XDG_DATA_DIRS", tmp ) ) ? String<>( ":/usr/local/share:/usr/share" ) : String<>( ":" ) + tmp;

        if ( XR_SUCCEEDED( GetEnvironmentVariableValue( "XDG_DATA_HOME", tmp ) ) )
        {
            paths += ":" + tmp;
        }
        else if ( XR_SUCCEEDED( GetEnvironmentVariableValue( "HOME", tmp ) ) )
        {
            paths += ":" + tmp + "/.local/share";
        }

        AppendFindOpenXr1Jsons( paths, outArray );
        return XR_SUCCESS;
    }


    XrResult GetApiLayerPathArray( const char*, Array<String<>>& implicitArray, Array<String<>>& explicitArray, Array<String<>>& envArray )
    {
        String<> pathList;

        if ( XR_SUCCEEDED( GetEnvironmentVariableValue( "XR_API_LAYER_PATH", pathList ) ) )
        {
            AppendFindOpenXr1Jsons( pathList, envArray );
        }
        else
        {
            String<> tmp;

            pathList  = XR_FAILED( GetEnvironmentVariableValue( "XDG_CONFIG_DIRS", tmp ) ) ? String<>( "/etc/xdg" ) : tmp;

            pathList += ":/etc";

            pathList += XR_FAILED( GetEnvironmentVariableValue( "XDG_DATA_DIRS", tmp ) ) ? String<>( ":/usr/local/share:/usr/share" ) : String<>( ":" ) + tmp;

            AppendFindOpenXr1Jsons( pathList, explicitArray );
        }

        {
            String<> tmp;
            String<> implicitPaths;

            implicitPaths = XR_FAILED( GetEnvironmentVariableValue( "XDG_DATA_DIRS", tmp ) ) ? String<>( "/usr/local/share:/usr/share" ) : tmp;

            if ( XR_SUCCEEDED( GetEnvironmentVariableValue( "XDG_DATA_HOME", tmp ) ) )
            {
                implicitPaths += ":" + tmp;
            }
            else if ( XR_SUCCEEDED( GetEnvironmentVariableValue( "HOME", tmp ) ) )
            {
                implicitPaths += ":" + tmp + "/.local/share";
            }

            AppendFindOpenXr1Jsons( implicitPaths, implicitArray );
        }

        return XR_SUCCESS;
    }

    SharedMutex::SharedMutex( )
    {
        pthread_rwlock_init( &sRWLock, nullptr );
    }

    SharedMutex::~SharedMutex( )
    {
        pthread_rwlock_destroy( &sRWLock );
    }

    void SharedMutex::LockExclusive( )
    {
        pthread_rwlock_wrlock( &sRWLock );
    }
    bool SharedMutex::TryLockExclusive( )
    {
        return pthread_rwlock_trywrlock( &sRWLock ) == 0;
    }
    void SharedMutex::UnlockExclusive( )
    {
        pthread_rwlock_unlock( &sRWLock );
    }

    void SharedMutex::LockShared( )
    {
        pthread_rwlock_rdlock( &sRWLock );
    }
    bool SharedMutex::TryLockShared( )
    {
        return pthread_rwlock_tryrdlock( &sRWLock ) == 0;
    }
    void SharedMutex::UnlockShared( )
    {
        pthread_rwlock_unlock( &sRWLock );
    }


    DynamicLibrary::DynamicLibrary( const String<>& path )
    {
        libraryHandle = dlopen( path, RTLD_LAZY );
    }

    DynamicLibrary::~DynamicLibrary( )
    {
        if ( libraryHandle )
        {
            dlclose( libraryHandle );
            libraryHandle = nullptr;
        }
    }

} // namespace VortexXr