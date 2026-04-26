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

#include "VortexXrWindowsUtility.h"
#include "VortexXrMessageDispatch.h"
#include "VortexXrWindows.h"

#include <TSArray.h>
#include <TSText.h>


// #ifndef TERATHON_NO_SYSTEM

#include <windows.h>

// #else

// extern "C"
//{

//    DWORD GetEnvironmentVariableW( const WCHAR* lpName, WCHAR* lpBuffer, DWORD nSize );
//}

// #endif


#define OPENXR_REGISTRY_PATH                    "SOFTWARE\\Khronos\\OpenXR\\"
#define OPENXR_IMPLICIT_API_LAYER_REGISTRY_PATH "\\ApiLayers\\Implicit"
#define OPENXR_EXPLICIT_API_LAYER_REGISTRY_PATH "\\ApiLayers\\Explicit"


using namespace Terathon;

namespace VortexXr
{

    constexpr bool Is32BitBinaryTargetOn64BitOs( )
    {
#if defined( _WIN64 ) && ( !defined( _M_X64 ) && !defined( _M_ARM64 ) )

        return true;

#endif

        return false;
    }


    namespace Detail
    {
        void Puts( const char* c )
        {
            HANDLE stdOut = GetStdHandle( STD_OUTPUT_HANDLE );

            if ( stdOut == INVALID_HANDLE_VALUE )
            {
                return;
            }

            DWORD textLength = Text::GetTextLength( c );

            WriteConsole( stdOut, c, textLength, nullptr, NULL );
        }

        void PutsChar( const char c )
        {
            HANDLE stdOut = GetStdHandle( STD_OUTPUT_HANDLE );

            if ( stdOut == INVALID_HANDLE_VALUE )
            {
                return;
            }

            WriteConsole( stdOut, &c, 1, nullptr, NULL );
        }


        void PutsError( const char* c )
        {
            HANDLE stdErr = GetStdHandle( STD_ERROR_HANDLE );

            if ( stdErr == INVALID_HANDLE_VALUE )
            {
                return;
            }

            DWORD textLength = Text::GetTextLength( c );

            WriteConsole( stdErr, c, textLength, nullptr, NULL );
        }

        void PutsErrorChar( const char c )
        {
            HANDLE stdErr = GetStdHandle( STD_ERROR_HANDLE );

            if ( stdErr == INVALID_HANDLE_VALUE )
            {
                return;
            }

            WriteConsole( stdErr, &c, 1, nullptr, NULL );
        }


        void PutsDebugger( const char* c )
        {
            OutputDebugStringA( c );
        }

        void PutsDebuggerChar( const char c )
        {
            char s[] = " \0";

            s[ 0 ]   = c;

            PutsDebugger( s );
        }

        void PutsDebugger( int64 i )
        {
            String<31> s = Text::Integer64ToString( i );
            PutsDebugger( static_cast<const char*>( s ) );
        }

    } // namespace Detail


    WCharBuffer::WCharBuffer( uint32 maxLength )
    {
        wchar_t* c = buffer = new wchar_t[ maxLength + 1 ];
        wchar_t* end        = buffer + maxLength;

        while ( c != end )
        {
            *( c++ ) = 0;
        }

        *end = 0;
    }

    WCharBuffer::WCharBuffer( const wchar_t* str, uint32 maxLength )
    {
        wchar_t* c = buffer = new wchar_t[ maxLength + 1 ];
        wchar_t* end        = buffer + maxLength;

        while ( *str != L'\0' && c != end )
        {
            *( c++ ) = *( str++ );
        }

        *c   = 0;
        *end = 0;
    }

    WCharBuffer::WCharBuffer( const String<>& str )
    {
        int32 length = Text::GetWideTextCharCount( str );
        buffer       = new wchar_t[ length + 1 ];

        Text::ConvertStringToWideText( str, reinterpret_cast<uint16*>( buffer ), length );
    }

    WCharBuffer::~WCharBuffer( )
    {
        delete[] buffer;
    }

    WCharBuffer::operator wchar_t*( )
    {
        return buffer;
    }

    WCharBuffer::operator const wchar_t*( ) const
    {
        return buffer;
    }

    WCharBuffer::operator String<>( )
    {
        int32    charArrayLength = Text::GetUnicodeStringLength( reinterpret_cast<uint16*>( buffer ) );

        String<> string;
        string.SetStringLength( charArrayLength );

        Text::ConvertWideTextToString( reinterpret_cast<uint16*>( buffer ), string, charArrayLength );

        return string;
    }

    XrResult GetEnvironmentVariableValue( const String<>& variableName, String<>& outVariableValue )
    {
        const WCharBuffer variableWideName( variableName );
        DWORD             characterCount = GetEnvironmentVariableW( variableWideName, nullptr, 0 );

        if ( characterCount <= 1 )
        {
            return XR_ERROR_NAME_INVALID;
        }

        WCharBuffer variableValue( characterCount );
        DWORD       finalCharacterCount = GetEnvironmentVariableW( variableWideName, variableValue, characterCount );

        if ( finalCharacterCount + 1 != characterCount )
        {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        outVariableValue = variableValue;

        return XR_SUCCESS;
    }


    String<> GetRegistryValue( HKEY rootRegistryKeyHandle, const String<>& subkeyName, const String<>& registryValueName )
    {
        WCharBuffer subkeyWideName( subkeyName );
        HKEY        registryKeyHandle;

        LSTATUS     result = RegOpenKeyExW( rootRegistryKeyHandle, subkeyWideName, 0, KEY_QUERY_VALUE, &registryKeyHandle );

        if ( result != ERROR_SUCCESS )
        {
            return String<>( );
        }

        const DWORD maxLength   = 2048;
        DWORD       sizeInBytes = maxLength * sizeof( wchar_t );
        WCharBuffer registryValueWideName( registryValueName );
        WCharBuffer registryValue( maxLength );

        result = RegGetValueW( registryKeyHandle, nullptr, registryValueWideName, RRF_ZEROONFAILURE | RRF_RT_REG_SZ | REG_EXPAND_SZ, nullptr, static_cast<wchar_t*>( registryValue ), &sizeInBytes );

        RegCloseKey( registryKeyHandle );

        return registryValue;
    }


    XrResult AppendRegistryNamesWithDataValueZero( HKEY rootRegistryKeyHandle, const String<>& subkeyName, Array<String<>>& nameArrayOutput )
    {
        WCharBuffer subkeyWideName( subkeyName );
        HKEY        registryKeyHandle;

        LSTATUS     result = RegOpenKeyExW( rootRegistryKeyHandle, subkeyWideName, 0, KEY_QUERY_VALUE, &registryKeyHandle );

        if ( result != ERROR_SUCCESS )
        {
            return XR_ERROR_RUNTIME_FAILURE;
        }

        DWORD       data;
        DWORD       dataSize      = sizeof( DWORD );
        DWORD       index         = 0;
        const DWORD maxStringSize = 2047;
        WCharBuffer entryName( maxStringSize );

        result = ERROR_SUCCESS;
        while ( result == ERROR_SUCCESS )
        {
            DWORD stringSize = maxStringSize;
            result           = RegEnumValueW( registryKeyHandle, index++, entryName, &stringSize, NULL, NULL, reinterpret_cast<LPBYTE>( &data ), &dataSize );

            if ( result == ERROR_SUCCESS && dataSize == sizeof( DWORD ) && data == 0 )
            {
                nameArrayOutput.AppendArrayElement( entryName );
            }
        }

        return XR_SUCCESS;
    }


    DWORD GetCurrentProcessIntegrityLevel( )
    {
        HANDLE tokenHandle = NULL;

        if ( !OpenProcessToken( GetCurrentProcess( ), TOKEN_QUERY_SOURCE | TOKEN_QUERY, &tokenHandle ) )
        {
            return SECURITY_MANDATORY_UNTRUSTED_RID;
        }

        if ( tokenHandle == NULL )
        {
            return SECURITY_MANDATORY_UNTRUSTED_RID;
        }

        uint8 tokenInformation[ SECURITY_MAX_SID_SIZE + sizeof( DWORD ) ] { };
        DWORD returnLength;

        if ( GetTokenInformation( tokenHandle, TokenIntegrityLevel, tokenInformation, sizeof( tokenInformation ), &returnLength ) == 0 )
        {
            CloseHandle( tokenHandle );

            return SECURITY_MANDATORY_UNTRUSTED_RID;
        }

        TOKEN_MANDATORY_LABEL* mandatoryLabel = reinterpret_cast<TOKEN_MANDATORY_LABEL*>( tokenInformation );

        if ( mandatoryLabel->Label.Sid == 0 )
        {
            CloseHandle( tokenHandle );

            return SECURITY_MANDATORY_UNTRUSTED_RID;
        }

        DWORD integrityLevel = *GetSidSubAuthority( mandatoryLabel->Label.Sid, *GetSidSubAuthorityCount( mandatoryLabel->Label.Sid ) - 1 );

        CloseHandle( tokenHandle );

        return integrityLevel;
    }


    XrResult AppendFindFiles( const String<>& directory, const String<>& pattern, Array<String<>>& fileArrayOutput )
    {
        WCharBuffer      wideDirectorySearchName( directory + "\\" + pattern );
        WIN32_FIND_DATAW findFileData;
        HANDLE           findFileHandle = FindFirstFileW( wideDirectorySearchName, &findFileData );

        if ( findFileHandle == INVALID_HANDLE_VALUE )
        {
            return XR_ERROR_FILE_ACCESS_ERROR;
        }

        do
        {
            if ( findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
                continue;
            }

            fileArrayOutput.AppendArrayElement( String<>( WCharBuffer( findFileData.cFileName, MAX_PATH ) ) );

        } while ( FindNextFileW( findFileHandle, &findFileData ) );

        FindClose( findFileHandle );

        return XR_SUCCESS;
    }


    XrResult GetActiveRuntimeJsonPathArray( Array<String<>>& outActiveRuntimeJsonPathArray )
    {
        if ( GetCurrentProcessIntegrityLevel( ) > SECURITY_MANDATORY_MEDIUM_RID )
        {
            return XR_ERROR_VALIDATION_FAILURE;
        }

        String<> jsonPathList;
        XrResult xrResult = GetEnvironmentVariableValue( XR_RUNTIME_JSON_ENVIRONMENT_VARIABLE_NAME, jsonPathList );

        // From the 1.1.40 standard: If the "XR_RUNTIME_JSON" variable is defined, then the loader will not look in the standard location for the active runtime. Instead, the loader will only utilize
        // the filename defined in the environment variable.
        if ( XR_FAILED( xrResult ) || jsonPathList.GetStringLength( ) == 0 )
        {
            String<> subkeyName = Is32BitBinaryTargetOn64BitOs( ) ? String<>( "WOW6432\\" ) + String<>( OPENXR_REGISTRY_PATH ) + XR_VERSION_MAJOR( XR_CURRENT_API_VERSION )
                                                                  : String<>( OPENXR_REGISTRY_PATH ) + XR_VERSION_MAJOR( XR_CURRENT_API_VERSION );

            jsonPathList        = GetRegistryValue( HKEY_LOCAL_MACHINE, subkeyName, "ActiveRuntime" );

            if ( GetCurrentProcessIntegrityLevel( ) <= SECURITY_MANDATORY_MEDIUM_RID )
            {
                jsonPathList += String<>( ";" ) + GetRegistryValue( HKEY_CURRENT_USER, subkeyName, "ActiveRuntime" );
            }
        }

        return SplitString( jsonPathList, ';', outActiveRuntimeJsonPathArray );
    }


    XrResult GetApiLayerPathArray( const char*      functionNameToReport,
                                   Array<String<>>& implicitApiLayerManifestFileArray,
                                   Array<String<>>& explicitApiLayerManifestFileArray,
                                   Array<String<>>& environmentVariableApiLayerManifestFileArray )
    {
        implicitApiLayerManifestFileArray.ClearArray( );
        explicitApiLayerManifestFileArray.ClearArray( );

        // Implicit layers
        String<> subkeyName = Is32BitBinaryTargetOn64BitOs( )
                                  ? String<>( "WOW6432\\" ) + String<>( OPENXR_REGISTRY_PATH ) + XR_VERSION_MAJOR( XR_CURRENT_API_VERSION ) + OPENXR_IMPLICIT_API_LAYER_REGISTRY_PATH
                                  : String<>( OPENXR_REGISTRY_PATH ) + XR_VERSION_MAJOR( XR_CURRENT_API_VERSION ) + OPENXR_IMPLICIT_API_LAYER_REGISTRY_PATH;

        auto     result     = AppendRegistryNamesWithDataValueZero( HKEY_LOCAL_MACHINE, subkeyName, implicitApiLayerManifestFileArray );

        if ( GetCurrentProcessIntegrityLevel( ) <= SECURITY_MANDATORY_MEDIUM_RID )
        {
            result = AppendRegistryNamesWithDataValueZero( HKEY_CURRENT_USER, subkeyName, implicitApiLayerManifestFileArray );
        }

        // Explicit layers.
        const String<> environmentVariableName( XR_API_LAYER_PATH_ENVIRONMENT_VARIABLE_NAME );

        if ( GetCurrentProcessIntegrityLevel( ) <= SECURITY_MANDATORY_MEDIUM_RID ) // Do not allow high integrity level to access the environment variable
        {
            String<>        environmentVariablePath;
            XrResult        xrResult = GetEnvironmentVariableValue( environmentVariableName, environmentVariablePath );

            Array<String<>> pathArray;
            xrResult = SplitString( environmentVariablePath, ';', pathArray );

            if ( XR_UNQUALIFIED_SUCCESS( xrResult ) )
            {
                for ( const auto& path : pathArray )
                {
                    AppendFindFiles( path, "*.json", environmentVariableApiLayerManifestFileArray );
                }
            };
        }
        else
        {
            Log::Warning( functionNameToReport, "Process integrity level too high to use environment variable." );
        }

        subkeyName = Is32BitBinaryTargetOn64BitOs( ) ? String<>( "WOW6432\\" ) + String<>( OPENXR_REGISTRY_PATH ) + XR_VERSION_MAJOR( XR_CURRENT_API_VERSION ) + OPENXR_EXPLICIT_API_LAYER_REGISTRY_PATH
                                                     : String<>( OPENXR_REGISTRY_PATH ) + XR_VERSION_MAJOR( XR_CURRENT_API_VERSION ) + OPENXR_EXPLICIT_API_LAYER_REGISTRY_PATH;

        result     = AppendRegistryNamesWithDataValueZero( HKEY_LOCAL_MACHINE, subkeyName, explicitApiLayerManifestFileArray );

        if ( GetCurrentProcessIntegrityLevel( ) <= SECURITY_MANDATORY_MEDIUM_RID )
        {
            result = AppendRegistryNamesWithDataValueZero( HKEY_CURRENT_USER, subkeyName, explicitApiLayerManifestFileArray );
        }

        for ( auto f : explicitApiLayerManifestFileArray )
        {
            Log::Info( functionNameToReport, FormatString( "Registry defined explicit Layer: %s", f ) );
        }

        return XR_SUCCESS;
    }

    SharedMutex::SharedMutex( )
    {
        InitializeSRWLock( &sRWLock );
    }

    SharedMutex::~SharedMutex( )
    {
    }

    void SharedMutex::LockExclusive( )
    {
        TryAcquireSRWLockExclusive( &sRWLock );
    }

    bool SharedMutex::TryLockExclusive( )
    {
        return TryAcquireSRWLockExclusive( &sRWLock );
    }

    _Requires_lock_held_( sRWLock ) void SharedMutex::UnlockExclusive( )
    {
        ReleaseSRWLockExclusive( &sRWLock );
    }

    void SharedMutex::LockShared( )
    {
        TryAcquireSRWLockShared( &sRWLock );
    }

    bool SharedMutex::TryLockShared( )
    {
        return TryAcquireSRWLockShared( &sRWLock );
    }

    void SharedMutex::UnlockShared( )
    {
        ReleaseSRWLockShared( &sRWLock );
    }


    DynamicLibrary::DynamicLibrary( const Terathon::String<>& libraryPath )
    {
        WCharBuffer libraryWidePath( libraryPath );
        libraryHandle = LoadLibraryW( libraryWidePath );

        if ( libraryHandle == nullptr )
        {
            libraryHandle = LoadLibraryExW( libraryWidePath, NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR );
        }
    }

    DynamicLibrary::~DynamicLibrary( )
    {
        if ( libraryHandle != nullptr )
        {
            FreeLibrary( libraryHandle );

            libraryHandle = nullptr;
        }
    }

} // namespace VortexXr