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

#include "VortexXrPlatform.h"

#include "TSArray.h"
#include "TSString.h"
#include "TSTools.h"

#include "openxr/openxr.h"


#define XR_MAX_FUNCTION_NAME_SIZE                   XR_MAX_EXTENSION_NAME_SIZE
#define XR_RUNTIME_JSON_ENVIRONMENT_VARIABLE_NAME   "XR_RUNTIME_JSON"
#define XR_API_LAYER_PATH_ENVIRONMENT_VARIABLE_NAME "XR_API_LAYER_PATH"


namespace VortexXr
{

    using namespace Terathon;


    // # \class	UniqueHolder	A helper class that uniquely owns a pointer to an object.
    //
    // # The $UniqueHolder$ class template provides unique ownership of a pointer to an object.
    // # When a $UniqueHolder$ object is destroyed (usually by going out of scope), the object
    // # that was passed into the $ptr$ parameter when the $UniqueHolder$ object was constructed
    // # is automatically deleted with the $delete$ operator.
    //
    // # A $UniqueHolder$ object behaves like a pointer to an object of the type given by the
    // # $type$ template parameter. A $UniqueHolder$ object can be passed as a function parameter
    // # wherever a pointer to $type$ is expected, and the $->$ operator can be used to access
    // # members of the object that the $UniqueHolder$ object owns.
    //
    // # A $UniqueHolder$ cannot be copied. Ownership can only be transferred using move semantics.
    // # This ensures that at most one $UniqueHolder$ instance owns a given object at any time.
    //
    // # \def	template <class type> class UniqueHolder
    //
    // # \tparam	type	The type of object owned by the $UniqueHolder$ object.
    //
    // # \ctor	explicit UniqueHolder(type *ptr);
    //
    // # \param	ptr		A pointer to the object that is owned by the $UniqueHolder$ object.
    //
    // # \also	$@UniqueHolder::UniqueHolder@$
    // # \also	$@UniqueHolder::operator =@$
    // # \also	$@UniqueHolder::reset@$
    // # \also	$@UniqueHolder::release@$


    // # \function	UniqueHolder::UniqueHolder	Move constructor.
    //
    // # \proto	UniqueHolder(UniqueHolder&& other);
    //
    // # \param	other		The $UniqueHolder$ whose ownership will be transferred.
    //
    // # \desc
    // # Move constructor. Transfers ownership of the object from $other$ to the newly
    // # constructed $UniqueHolder$ instance.
    // #
    // # After construction, the new $UniqueHolder$ owns the object that was previously
    // # owned by $other$, and $other$ is left empty (set to $nullptr$).
    //
    // # \also	$@UniqueHolder::operator =@$
    // # \also	$@UniqueHolder::UniqueHolder@$


    // # \function	UniqueHolder::operator =	Move assignment operator.
    //
    // # \proto	UniqueHolder& operator =(UniqueHolder&& other);
    //
    // # \param	other		The $UniqueHolder$ whose ownership will be transferred.
    //
    // # \desc
    // # Move assignment operator. Deletes any currently owned object, then transfers
    // # ownership from $other$ to this $UniqueHolder$.
    // #
    // # After assignment, this $UniqueHolder$ owns the object previously owned by $other$,
    // # and $other$ is left empty (set to $nullptr$).
    //
    // # \also	$@UniqueHolder::UniqueHolder@$
    // # \also	$@UniqueHolder::reset@$


    // # \function	UniqueHolder::reset		Replaces the currently owned object.
    //
    // # \proto	void reset(type *ptr = nullptr);
    //
    // # \param	ptr		A pointer to the object that should replace the currently
    // #				owned object. If $nullptr$, the $UniqueHolder$ becomes empty.
    //
    // # \desc
    // # Replaces the currently owned object with a new pointer. If the $UniqueHolder$
    // # already owns an object, it is deleted first using the $delete$ operator.
    // #
    // # After the call, the $UniqueHolder$ takes ownership of the object passed in
    // # through the $ptr$ parameter. If $ptr$ is $nullptr$, the $UniqueHolder$ will
    // # no longer own any object.
    // #
    // # This operation is safe even if $ptr$ is the same as the currently owned pointer.
    //
    // # \also	$@UniqueHolder::release@$
    // # \also	$@UniqueHolder::operator =@$

    // # \function	UniqueHolder::release		Releases ownership of the object.
    //
    // # \proto	type* release(void);
    //
    // # \desc
    // # Releases ownership of the currently owned object and returns its raw pointer.
    // # After this call, the $UniqueHolder$ no longer owns any object and will not delete
    // # the previously owned pointer.
    // #
    // # This function is typically used when transferring ownership to another system
    // # or object that takes responsibility for deleting the pointer.
    //
    // # \also	$@UniqueHolder::reset@$
    // # \also	$@UniqueHolder::UniqueHolder@$

    template <class type> class UniqueHolder
    {
        private:

            type* pointer;

            UniqueHolder( const UniqueHolder& )            = delete;
            UniqueHolder& operator=( const UniqueHolder& ) = delete;

        public:

            UniqueHolder( ) : pointer( nullptr )
            {
            }

            explicit UniqueHolder( type* ptr ) : pointer( ptr )
            {
            }

            UniqueHolder( UniqueHolder&& other ) noexcept : pointer( other.pointer )
            {
                other.pointer = nullptr;
            }

            UniqueHolder& operator=( UniqueHolder&& other ) noexcept
            {
                if ( this != &other )
                {
                    delete pointer;
                    pointer       = other.pointer;
                    other.pointer = nullptr;
                }
                return *this;
            }

            ~UniqueHolder( )
            {
                delete pointer;
            }

            operator type*( ) const
            {
                return pointer;
            }

            type* operator->( ) const
            {
                return pointer;
            }

            type& operator*( ) const
            {
                return *pointer;
            }

            type* get( ) const
            {
                return pointer;
            }

            type* release( )
            {
                type* tmp = pointer;
                pointer   = nullptr;
                return tmp;
            }

            void reset( type* ptr = nullptr )
            {
                if ( pointer != ptr )
                {
                    delete pointer;
                    pointer = ptr;
                }
            }
    };

    template <typename T> struct Identity
    {
            using Type = T;
    };


    template <typename T> T&& Forward( typename Identity<T>::Type&& arg )
    {
        return static_cast<typename Identity<T>::Type&&>( arg );
    }


    template <class T> struct RemoveReference
    {
            using Type = T;
    };

    template <class T> struct RemoveReference<T&>
    {
            using Type = T;
    };

    template <class T> struct RemoveReference<T&&>
    {
            using Type = T;
    };

    template <class T> using RemoveReferenceType = typename RemoveReference<T>::Type;


    template <typename T> constexpr RemoveReferenceType<T>&& Move( T&& t )
    {
        return static_cast<RemoveReferenceType<T>&&>( t );
    }


    template <typename T> class Optional
    {
            alignas( T ) unsigned char storage[ sizeof( T ) ];
            bool hasValue = false;

        public:

            Optional( ) = default;

            Optional( const T& value ) : hasValue( true )
            {
                new ( storage ) T( value );
            }

            Optional( const Optional& other ) : hasValue( other.hasValue )
            {
                if ( hasValue )
                {
                    new ( storage ) T( *other );
                }
            }

            Optional& operator=( const Optional& other )
            {
                if ( this != &other )
                {
                    Reset( );
                    hasValue = other.hasValue;
                    if ( hasValue )
                    {
                        new ( storage ) T( *other );
                    }
                }
                return *this;
            }

            ~Optional( )
            {
                Reset( );
            }

            void Reset( )
            {
                if ( hasValue )
                {
                    reinterpret_cast<T*>( storage )->~T( );
                    hasValue = false;
                }
            }

            bool HasValue( ) const
            {
                return hasValue;
            }

            explicit operator bool( ) const
            {
                return hasValue;
            }

            T& operator*( )
            {
                return *reinterpret_cast<T*>( storage );
            }
            const T& operator*( ) const
            {
                return *reinterpret_cast<const T*>( storage );
            }

            T* operator->( )
            {
                return reinterpret_cast<T*>( storage );
            }
            const T* operator->( ) const
            {
                return reinterpret_cast<const T*>( storage );
            }
    };

    struct NullOptT
    {
    };

    inline constexpr NullOptT         NullOpt { };

    template <typename T> Optional<T> MakeOptional( const T& value )
    {
        return Optional<T>( value );
    }


    class SharedMutex
    {
        private:

            VORTEXXR_RWLOCK_TYPE sRWLock;

        public:

            SharedMutex( const SharedMutex& )            = delete;
            SharedMutex& operator=( const SharedMutex& ) = delete;

        public:

            SharedMutex( );
            ~SharedMutex( );

            void LockExclusive( );
            bool TryLockExclusive( );

            // clang-format off

            VORTEXXR_RECQUIRED_LOCK_HELD
            void UnlockExclusive( );

            // clang-format on

            void LockShared( );
            bool TryLockShared( );
            void UnlockShared( );
    };


    class SharedMutexExclusiveLocker
    {
        private:

            SharedMutex& mutex;

        public:

            SharedMutexExclusiveLocker( SharedMutex& mutex );
            ~SharedMutexExclusiveLocker( );
    };

    class SharedMutexSharedLocker
    {
        private:

            SharedMutex& mutex;

        public:

            SharedMutexSharedLocker( SharedMutex& mutex );
            ~SharedMutexSharedLocker( );
    };


    int32    GetTextLength( const char* c, int32 maxLength );
    XrResult SplitString( const String<>& string, const char separator, Array<String<>>& outStringArray );
    String<> GetParentPath( const String<>& string );

    XrResult GetEnvironmentVariableValue( const String<>& inVariableName, String<>& outVariableValue );

    XrResult GetActiveRuntimeJsonPathArray( Array<String<>>& outActiveRuntimeJsonPathArray );

    XrResult GetApiLayerPathArray( const char*      functionNameToReport,
                                   Array<String<>>& implicitApiLayerManifestFileArray,
                                   Array<String<>>& explicitApiLayerManifestFileArray,
                                   Array<String<>>& environmentVariableApiLayerManifestFileArray );

    bool     CompareFunctionName( const char* name1, const char* name2 );

    namespace Detail
    {

        void Puts( const char* c );
        void Puts( int64 i );
        void PutsChar( const char c );

        void PutsError( const char* c );
        void PutsError( int64 i );
        void PutsErrorChar( const char c );

        void Printf( const char* format );
        void PrintfError( const char* format );

        void FormatString( String<>& out, const char* c );
        void FormatString( String<>& out, int64 i );

        //
        template <typename T, typename... Targs> void FormatString( String<>& out, const char* format, T value, Targs... Fargs )
        {
            for ( ; *format != 0; format++ )
            {
                if ( *format == '%' )
                {
                    out += value;
                    FormatString( out, format + 1, Fargs... );

                    return;
                }


                out += String<>( &*format, 1 );
            }
        }
    } // namespace Detail


    template <typename... Args> String<> FormatString( const char* format, Args... args )
    {
        String<> str;
        Detail::FormatString( str, format, args... );

        return str;
    }


    class DynamicLibrary
    {
        private:

            LibraryHandle libraryHandle = nullptr;

        public:

            DynamicLibrary( const String<>& libraryPath );
            ~DynamicLibrary( );

            template <class PFN_Type> PFN_Type GetProcedureAddress( const char* procedureName )
            {
                if ( libraryHandle == NULL )
                {
                    return nullptr;
                }

                return OsGetProcedureAddress<PFN_Type>( libraryHandle, procedureName );
            }

            bool IsInitialized( ) const;
    };

} // namespace VortexXr