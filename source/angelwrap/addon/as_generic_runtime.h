#ifndef AS_GENERIC_RUNTIME_H
#define AS_GENERIC_RUNTIME_H

#include <angelscript.h>

#if defined( __EMSCRIPTEN__ ) || defined( __WASM__ )

#include <cstring>
#include <new>
#include <type_traits>

namespace ASGen
{
	static const int MAX_SLOTS = 1536;

	template<typename T>
	struct Proxy {
		T value;
		Proxy( T v ) : value( v ) {}
		static T cast( void *ptr ) {
			return reinterpret_cast<Proxy<T> *>( &ptr )->value;
		}
	};

	// GetAddressOfArg() points at the stack slot. References/handles store a pointer there.
	template<typename T>
	struct ArgGet {
		static T get( void *p ) { return *reinterpret_cast<T *>( p ); }
	};
	template<typename T>
	struct ArgGet<T *> {
		static T *get( void *p ) { return *reinterpret_cast<T **>( p ); }
	};
	template<typename T>
	struct ArgGet<const T *> {
		static const T *get( void *p ) { return *reinterpret_cast<const T **>( p ); }
	};
	template<typename T>
	struct ArgGet<T &> {
		static T &get( void *p ) { return **reinterpret_cast<T **>( p ); }
	};
	template<typename T>
	struct ArgGet<const T &> {
		static const T &get( void *p ) { return **reinterpret_cast<T **>( p ); }
	};

	// GetObject() already returns the object pointer.
	template<typename T>
	struct ObjGet {
		static T get( void *obj ) { return *reinterpret_cast<T *>( obj ); }
	};
	template<typename T>
	struct ObjGet<T *> {
		static T *get( void *obj ) { return static_cast<T *>( obj ); }
	};
	template<typename T>
	struct ObjGet<const T *> {
		static const T *get( void *obj ) { return static_cast<const T *>( obj ); }
	};
	template<typename T>
	struct ObjGet<T &> {
		static T &get( void *obj ) { return *static_cast<T *>( obj ); }
	};
	template<typename T>
	struct ObjGet<const T &> {
		static const T &get( void *obj ) { return *static_cast<const T *>( obj ); }
	};

	template<int... I>
	struct seq {};

	template<int N, int... I>
	struct gens : gens<N - 1, N - 1, I...> {};

	template<int... I>
	struct gens<0, I...> { typedef seq<I...> type; };

	struct Slot {
		void ( *invoke )( asIScriptGeneric *gen, const Slot *s );
		char fn[32];
	};

	extern Slot slots[MAX_SLOTS];
	extern int nslots;

	void InvokeSlot( int i, asIScriptGeneric *g );

	template<typename F>
	F loadFn( const Slot *s )
	{
		F f;
		std::memcpy( &f, s->fn, sizeof( F ) );
		return f;
	}

	template<typename R>
	struct RetHelper {
		template<typename F>
		static void apply( asIScriptGeneric *gen, F fn ) {
			new ( gen->GetAddressOfReturnLocation() ) Proxy<R>( fn() );
		}
	};

	template<>
	struct RetHelper<void> {
		template<typename F>
		static void apply( asIScriptGeneric *gen, F fn ) {
			(void)gen;
			fn();
		}
	};

	inline int allocSlot( const void *fn, unsigned fnSize, void ( *invoke )( asIScriptGeneric *, const Slot * ) )
	{
		if( nslots >= MAX_SLOTS || fnSize > sizeof( Slot::fn ) )
			return -1;
		int i = nslots++;
		std::memset( slots[i].fn, 0, sizeof( slots[i].fn ) );
		std::memcpy( slots[i].fn, fn, fnSize );
		slots[i].invoke = invoke;
		return i;
	}
}

extern void ( * const ASGen_Thunks[] )( asIScriptGeneric * );

namespace ASGen
{
	inline asSFuncPtr thunkPtr( int i )
	{
		return asFUNCTION( ASGen_Thunks[i] );
	}

	template<typename R, typename... Args, int... I>
	R invokeCdeclCall( asIScriptGeneric *gen, R ( *fp )( Args... ), seq<I...> )
	{
		return fp( ArgGet<Args>::get( gen->GetAddressOfArg( I ) )... );
	}

	template<typename R, typename... Args>
	void invokeCdecl( asIScriptGeneric *gen, const Slot *s )
	{
		auto fp = loadFn<R ( * )( Args... )>( s );
		RetHelper<R>::apply( gen, [gen, fp]() -> R {
			return invokeCdeclCall( gen, fp, typename gens<sizeof...( Args )>::type() );
		} );
	}

	template<typename T, typename R, typename... Args, int... I>
	R invokeMethodCall( asIScriptGeneric *gen, R ( T::*fp )( Args... ), seq<I...> )
	{
		return ( static_cast<T *>( gen->GetObject() )->*fp )( ArgGet<Args>::get( gen->GetAddressOfArg( I ) )... );
	}

	template<typename T, typename R, typename... Args>
	void invokeMethod( asIScriptGeneric *gen, const Slot *s )
	{
		auto fp = loadFn<R ( T::* )( Args... )>( s );
		RetHelper<R>::apply( gen, [gen, fp]() -> R {
			return invokeMethodCall<T, R, Args...>( gen, fp, typename gens<sizeof...( Args )>::type() );
		} );
	}

	template<typename T, typename R, typename... Args, int... I>
	R invokeMethodConstCall( asIScriptGeneric *gen, R ( T::*fp )( Args... ) const, seq<I...> )
	{
		return ( static_cast<const T *>( gen->GetObject() )->*fp )( ArgGet<Args>::get( gen->GetAddressOfArg( I ) )... );
	}

	template<typename T, typename R, typename... Args>
	void invokeMethodConst( asIScriptGeneric *gen, const Slot *s )
	{
		auto fp = loadFn<R ( T::* )( Args... ) const>( s );
		RetHelper<R>::apply( gen, [gen, fp]() -> R {
			return invokeMethodConstCall<T, R, Args...>( gen, fp, typename gens<sizeof...( Args )>::type() );
		} );
	}

	template<typename R, typename Obj, typename... Args, int... I>
	R invokeObjFirstCall( asIScriptGeneric *gen, R ( *fp )( Obj, Args... ), seq<I...> )
	{
		return fp( ObjGet<Obj>::get( gen->GetObject() ), ArgGet<Args>::get( gen->GetAddressOfArg( I ) )... );
	}

	template<typename R, typename Obj, typename... Args>
	void invokeObjFirst( asIScriptGeneric *gen, const Slot *s )
	{
		auto fp = loadFn<R ( * )( Obj, Args... )>( s );
		RetHelper<R>::apply( gen, [gen, fp]() -> R {
			return invokeObjFirstCall( gen, fp, typename gens<sizeof...( Args )>::type() );
		} );
	}

	template<typename T, int I, int Last>
	struct ObjLastGet {
		static T get( asIScriptGeneric *gen ) {
			return ArgGet<T>::get( gen->GetAddressOfArg( I ) );
		}
	};

	template<typename T, int I>
	struct ObjLastGet<T, I, I> {
		static T get( asIScriptGeneric *gen ) {
			return ObjGet<T>::get( gen->GetObject() );
		}
	};

	template<typename R, typename... Args, int... I>
	R invokeObjLastCall( asIScriptGeneric *gen, R ( *fp )( Args... ), seq<I...> )
	{
		return fp( ObjLastGet<Args, I, (int)sizeof...( Args ) - 1>::get( gen )... );
	}

	template<typename R, typename... Args>
	void invokeObjLast( asIScriptGeneric *gen, const Slot *s )
	{
		auto fp = loadFn<R ( * )( Args... )>( s );
		RetHelper<R>::apply( gen, [gen, fp]() -> R {
			return invokeObjLastCall( gen, fp, typename gens<sizeof...( Args )>::type() );
		} );
	}

	inline void DummyGeneric( asIScriptGeneric * )
	{
	}

	inline asSFuncPtr wrapOrDummy( int i )
	{
		if( i < 0 )
			return asFUNCTION( DummyGeneric );
		return thunkPtr( i );
	}

	template<typename R, typename... Args>
	asSFuncPtr Wrap( R ( *fp )( Args... ) )
	{
		return wrapOrDummy( allocSlot( &fp, sizeof( fp ), &invokeCdecl<R, Args...> ) );
	}

	template<typename T, typename R, typename... Args>
	asSFuncPtr Wrap( R ( T::*fp )( Args... ) )
	{
		return wrapOrDummy( allocSlot( &fp, sizeof( fp ), &invokeMethod<T, R, Args...> ) );
	}

	template<typename T, typename R, typename... Args>
	asSFuncPtr Wrap( R ( T::*fp )( Args... ) const )
	{
		return wrapOrDummy( allocSlot( &fp, sizeof( fp ), &invokeMethodConst<T, R, Args...> ) );
	}

	template<typename R, typename Obj, typename... Args>
	asSFuncPtr WrapObjFirst( R ( *fp )( Obj, Args... ) )
	{
		return wrapOrDummy( allocSlot( &fp, sizeof( fp ), &invokeObjFirst<R, Obj, Args...> ) );
	}

	template<typename R>
	asSFuncPtr WrapObjFirst( R ( *fp )() )
	{
		return Wrap( fp );
	}

	template<typename R, typename... Args>
	asSFuncPtr WrapObjLast( R ( *fp )( Args... ) )
	{
		return wrapOrDummy( allocSlot( &fp, sizeof( fp ), &invokeObjLast<R, Args...> ) );
	}
}

#endif // __EMSCRIPTEN__ || __WASM__

#if defined( __EMSCRIPTEN__ ) || defined( __WASM__ )
#define AS_FN( fn ) ( ::ASGen::Wrap( fn ) )
#define AS_OL( fn ) ( ::ASGen::WrapObjLast( fn ) )
#define AS_OF( fn ) ( ::ASGen::WrapObjFirst( fn ) )
#define AS_CC asCALL_GENERIC
#define AS_CC_OL asCALL_GENERIC
#define AS_CC_OF asCALL_GENERIC
#define AS_CC_THIS asCALL_GENERIC
#else
#define AS_FN( fn ) asFUNCTION( fn )
#define AS_OL( fn ) asFUNCTION( fn )
#define AS_OF( fn ) asFUNCTION( fn )
#define AS_CC asCALL_CDECL
#define AS_CC_OL asCALL_CDECL_OBJLAST
#define AS_CC_OF asCALL_CDECL_OBJFIRST
#define AS_CC_THIS asCALL_THISCALL
#endif

#endif
