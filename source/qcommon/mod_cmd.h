#ifndef __MODULE_CMD_H
#define __MODULE_CMD_H

//#include "../gameshared/q_arch.h"
//#include "../gameshared/q_shared.h"

#define DECLARE_TYPEDEF_METHOD( ret, name, ... ) \
	typedef ret(*name##Fn )( __VA_ARGS__ ); \
	ret name (__VA_ARGS__);

typedef void ( *xcommand_t )( void );
typedef char ** ( *xcompletionf_t )( const char *partial );

/*
Command execution takes a null terminated string, breaks it into tokens,
then searches for a command or variable that matches the first token.
*/

DECLARE_TYPEDEF_METHOD( void, Cmd_AddCommand, const char *cmd_name, xcommand_t function );
DECLARE_TYPEDEF_METHOD( void, Cmd_RemoveCommand, const char *cmd_name );
DECLARE_TYPEDEF_METHOD( bool, Cmd_Exists, const char *cmd_name );
DECLARE_TYPEDEF_METHOD( bool, Cmd_CheckForCommand, char *text );
DECLARE_TYPEDEF_METHOD( void, Cmd_WriteAliases, int file );
DECLARE_TYPEDEF_METHOD( int, Cmd_CompleteAliasCountPossible, const char *partial );
DECLARE_TYPEDEF_METHOD( char **, Cmd_CompleteAliasBuildList, const char *partial );
DECLARE_TYPEDEF_METHOD( int, Cmd_CompleteCountPossible, const char *partial );
DECLARE_TYPEDEF_METHOD( char **, Cmd_CompleteBuildList, const char *partial );
DECLARE_TYPEDEF_METHOD( char **, Cmd_CompleteBuildArgList, const char *partial );
DECLARE_TYPEDEF_METHOD( char **, Cmd_CompleteBuildArgListExt, const char *command, const char *arguments );
DECLARE_TYPEDEF_METHOD( char **, Cmd_CompleteFileList, const char *partial, const char *basedir, const char *extension, bool subdirectories );
DECLARE_TYPEDEF_METHOD( int, Cmd_Argc, void );
DECLARE_TYPEDEF_METHOD( char *, Cmd_Argv, int arg );
DECLARE_TYPEDEF_METHOD( char *, Cmd_Args, void );
DECLARE_TYPEDEF_METHOD( void, Cmd_TokenizeString, const char *text );
DECLARE_TYPEDEF_METHOD( void, Cmd_ExecuteString, const char *text );
DECLARE_TYPEDEF_METHOD( void, Cmd_SetCompletionFunc, const char *cmd_name, xcompletionf_t completion_func );

#undef DECLARE_TYPEDEF_METHOD

struct cmd_import_s {
	Cmd_AddCommandFn Cmd_AddCommand;
	Cmd_RemoveCommandFn Cmd_RemoveCommand;
	Cmd_ExistsFn Cmd_Exists;
	Cmd_CheckForCommandFn Cmd_CheckForCommand;
	Cmd_WriteAliasesFn Cmd_WriteAliases;
	Cmd_CompleteAliasCountPossibleFn Cmd_CompleteAliasCountPossible;
	Cmd_CompleteAliasBuildListFn Cmd_CompleteAliasBuildList;
	Cmd_CompleteCountPossibleFn Cmd_CompleteCountPossible;
	Cmd_CompleteBuildListFn Cmd_CompleteBuildList;
	Cmd_CompleteBuildArgListFn Cmd_CompleteBuildArgList;
	Cmd_CompleteBuildArgListExtFn Cmd_CompleteBuildArgListExt;
	Cmd_CompleteFileListFn Cmd_CompleteFileList;
	Cmd_ArgcFn Cmd_Argc;
	Cmd_ArgvFn Cmd_Argv;
	Cmd_ArgsFn Cmd_Args;
	Cmd_TokenizeStringFn Cmd_TokenizeString;
	Cmd_ExecuteStringFn Cmd_ExecuteString;
	Cmd_SetCompletionFuncFn Cmd_SetCompletionFunc;
};

#define DECLARE_CMD_STRUCT() { \
	Cmd_AddCommand, \
	Cmd_RemoveCommand, \
	Cmd_Exists, \
	Cmd_CheckForCommand, \
	Cmd_WriteAliases, \
	Cmd_CompleteAliasCountPossible, \
	Cmd_CompleteAliasBuildList, \
	Cmd_CompleteCountPossible, \
	Cmd_CompleteBuildList, \
	Cmd_CompleteBuildArgList, \
	Cmd_CompleteBuildArgListExt, \
	Cmd_CompleteFileList, \
	Cmd_Argc, \
	Cmd_Argv, \
	Cmd_Args, \
	Cmd_TokenizeString, \
	Cmd_ExecuteString, \
	Cmd_SetCompletionFunc, \
};

#if CMD_DEFINE_INTERFACE_IMPL
static struct cmd_import_s cmd_import;

void Cmd_AddCommand( const char *cmd_name, xcommand_t function ) { cmd_import.Cmd_AddCommand(cmd_name, function); }
void Cmd_RemoveCommand( const char *cmd_name ) { cmd_import.Cmd_RemoveCommand(cmd_name); }
bool Cmd_Exists( const char *cmd_name ){ return cmd_import.Cmd_Exists(cmd_name); }
bool Cmd_CheckForCommand( char *text ){ return cmd_import.Cmd_CheckForCommand(text); }
void Cmd_WriteAliases( int file ){ cmd_import.Cmd_WriteAliases(file); }
int Cmd_CompleteAliasCountPossible( const char *partial ){ return cmd_import.Cmd_CompleteAliasCountPossible(partial); }
char **Cmd_CompleteAliasBuildList( const char *partial ){ return cmd_import.Cmd_CompleteAliasBuildList(partial); }
int Cmd_CompleteCountPossible( const char *partial ){ return cmd_import.Cmd_CompleteCountPossible(partial); }
char **Cmd_CompleteBuildList( const char *partial ){ return cmd_import.Cmd_CompleteBuildList(partial); }
char **Cmd_CompleteBuildArgList( const char *partial ){ return cmd_import.Cmd_CompleteBuildArgList(partial); }
char **Cmd_CompleteBuildArgListExt( const char *command, const char *arguments ){ return cmd_import.Cmd_CompleteBuildArgListExt(command, arguments); }
char **Cmd_CompleteFileList( const char *partial, const char *basedir, const char *extension, bool subdirectories ){ return cmd_import.Cmd_CompleteFileList(partial, basedir, extension, subdirectories);}
int Cmd_Argc( void ){ return cmd_import.Cmd_Argc(); }
char *Cmd_Argv( int arg ){ return cmd_import.Cmd_Argv(arg); }
char *Cmd_Args( void ){ return cmd_import.Cmd_Args();}
void Cmd_TokenizeString( const char *text ){ cmd_import.Cmd_TokenizeString(text); }
void Cmd_ExecuteString( const char *text ){ cmd_import.Cmd_ExecuteString(text); }
void Cmd_SetCompletionFunc( const char *cmd_name, xcompletionf_t completion_func ){ cmd_import.Cmd_SetCompletionFunc(cmd_name, completion_func); }

static inline void Q_ImportCmdModule(struct cmd_import_s* mod) {
  cmd_import = *mod;
}
#endif

#endif
