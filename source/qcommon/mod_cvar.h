#ifndef __MODULE_CVAR_H
#define __MODULE_CVAR_H


#include "../gameshared/q_cvar.h"

#define DECLARE_TYPEDEF_METHOD( ret, name, ... ) \
	typedef ret(*name##Fn )( __VA_ARGS__ ); \
	ret name (__VA_ARGS__);

DECLARE_TYPEDEF_METHOD( cvar_t *, Cvar_Get, const char *var_name, const char *value, cvar_flag_t flags );
DECLARE_TYPEDEF_METHOD( cvar_t *, Cvar_Set, const char *var_name, const char *value );
DECLARE_TYPEDEF_METHOD( cvar_t *, Cvar_ForceSet, const char *var_name, const char *value );
DECLARE_TYPEDEF_METHOD( cvar_t *, Cvar_FullSet, const char *var_name, const char *value, cvar_flag_t flags, bool overwrite_flags );
DECLARE_TYPEDEF_METHOD( void, Cvar_SetValue, const char *var_name, float value );
DECLARE_TYPEDEF_METHOD( float, Cvar_Value, const char *var_name );
DECLARE_TYPEDEF_METHOD( const char *, Cvar_String, const char *var_name );
DECLARE_TYPEDEF_METHOD( int, Cvar_Integer, const char *var_name );
DECLARE_TYPEDEF_METHOD( cvar_t *, Cvar_Find, const char *var_name );

#undef DECLARE_TYPEDEF_METHOD

struct cvar_import_s {
	Cvar_GetFn Cvar_Get;
	Cvar_SetFn Cvar_Set;
	Cvar_ForceSetFn Cvar_ForceSet;
	Cvar_FullSetFn Cvar_FullSet;
	Cvar_SetValueFn Cvar_SetValue;
	Cvar_ValueFn Cvar_Value;
	Cvar_StringFn Cvar_String;
	Cvar_IntegerFn Cvar_Integer;
	Cvar_FindFn Cvar_Find;
};


#define DECLARE_CVAR_STRUCT() { \
	Cvar_Get, \
	Cvar_Set, \
	Cvar_ForceSet, \
	Cvar_FullSet, \
	Cvar_SetValue, \
	Cvar_Value, \
	Cvar_String, \
	Cvar_Integer, \
	Cvar_Find, \
};


#if CVAR_DEFINE_INTERFACE_IMPL
static struct cvar_import_s cvar_import;

cvar_t *Cvar_Get( const char *var_name, const char *value, cvar_flag_t flags ) { return cvar_import.Cvar_Get( var_name, value, flags );}
cvar_t * Cvar_Set( const char *var_name, const char *value ) { return cvar_import.Cvar_Set( var_name, value);}
cvar_t * Cvar_ForceSet( const char *var_name, const char *value ){ return cvar_import.Cvar_ForceSet( var_name, value);}
cvar_t * Cvar_FullSet( const char *var_name, const char *value, cvar_flag_t flags, bool overwrite_flags ) { return cvar_import.Cvar_FullSet( var_name, value, flags, overwrite_flags);}
void Cvar_SetValue( const char *var_name, float value ) { return cvar_import.Cvar_SetValue( var_name, value);}
float Cvar_Value( const char *var_name ) { return cvar_import.Cvar_Value(var_name);}
const char * Cvar_String( const char *var_name ) { return cvar_import.Cvar_String(var_name);} 
int Cvar_Integer( const char *var_name ) { return cvar_import.Cvar_Integer(var_name);}  
cvar_t * Cvar_Find( const char *var_name ) { return cvar_import.Cvar_Find(var_name);}   

static inline void Q_ImportCvarModule( struct cvar_import_s *mod )
{
	cvar_import = *mod;
}
#endif



#endif

