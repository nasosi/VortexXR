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

#include "openxr/openxr.h"


using namespace Terathon;


namespace VortexXr
{

    SharedMutexExclusiveLocker::SharedMutexExclusiveLocker( SharedMutex& mutex ) : mutex( mutex )
    {
        mutex.LockExclusive( );
    }

    SharedMutexExclusiveLocker::~SharedMutexExclusiveLocker( )
    {
        mutex.UnlockExclusive( );
    }


    SharedMutexSharedLocker::SharedMutexSharedLocker( SharedMutex& mutex ) : mutex( mutex )
    {
        mutex.LockShared( );
    }

    SharedMutexSharedLocker::~SharedMutexSharedLocker( )
    {
        mutex.UnlockShared( );
    }

    int32 GetTextLength( const char* c, int32 maxLength )
    {
        const char* start = c;
        while ( *c != 0 && int32( c - start ) < maxLength )
        {
            c++;
        }

        return ( int32( c - start ) );
    }

    XrResult SplitString( const String<>& string, const char separator, Array<String<>>& outStringArray )
    {
        outStringArray.ClearArray( );

        const char* c                 = string;
        int32       separatorLocation = Text::FindChar( c, separator );

        while ( separatorLocation != -1 )
        {
            String<> path = String<>( c, separatorLocation );

            if ( path.GetStringLength( ) != 0 )
            {
                outStringArray.AppendArrayElement( path );
            }

            c                 += separatorLocation + 1;
            separatorLocation  = Text::FindChar( c, separator );
        }

        String<> path( c );

        if ( path.GetStringLength( ) != 0 )
        {
            outStringArray.AppendArrayElement( path );
        }

        return XR_SUCCESS;
    }

    Terathon::String<> GetParentPath( const Terathon::String<>& filePath )
    {
        const char* c      = filePath;

        int32       last   = -1;
        int32       offset = 0;

        int32       pos    = Text::FindChar( c, VORTEXXR_FOLDER_SEPARATOR );

        while ( pos != -1 )
        {
            last    = offset + pos;
            offset += pos + 1;
            c      += pos + 1;
            pos     = Text::FindChar( c, VORTEXXR_FOLDER_SEPARATOR );
        }

        if ( last == -1 )
        {
            return String<>( );
        }

        return String<>( filePath, last + 1 );
    }


    namespace Detail
    {
        void Puts( int64 i )
        {
            String<31> s = Text::Integer64ToString( i );
            Puts( static_cast<const char*>( s ) );
        }

        void PutsError( int64 i )
        {
            String<31> s = Text::Integer64ToString( i );
            PutsError( static_cast<const char*>( s ) );
        }

        void Printf( const char* format )
        {
            Puts( format );
        }

        void PrintfError( const char* format )
        {
            PutsError( format );
        }

        void FormatString( String<>& out, const char* c )
        {
            out += c;
        }

        void FormatString( String<>& out, int64 i )
        {
            out += Text::Integer64ToString( i );
        }

    } // namespace Detail


    bool DynamicLibrary::IsInitialized( ) const
    {
        return libraryHandle != nullptr;
    }

    bool CompareFunctionName( const char* name1, const char* name2 )
    {
        return Text::CompareText( name1, name2, XR_MAX_FUNCTION_NAME_SIZE );
    }

} // namespace VortexXr
