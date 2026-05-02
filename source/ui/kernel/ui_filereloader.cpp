#include "ui_precompiled.h"
#include "kernel/ui_common.h"
#include "kernel/ui_main.h"
#include "kernel/ui_utils.h"
#include "kernel/ui_filereloader.h"

#include <Rocket/Debugger/Debugger.h>

namespace WSWUI {

UI_FileReloader::UI_FileReloader()
	: needsScan( true ), lastCheckTime( 0 ), lastReloadTime( 0 )
{
}

UI_FileReloader::~UI_FileReloader()
{
}

void UI_FileReloader::clearTrackedFiles()
{
	trackedFiles.clear();
	needsScan = true;
}

void UI_FileReloader::checkForChanges( unsigned int currentTime )
{
	if( !Rocket::Debugger::IsVisible() )
		return;

	if( needsScan )
	{
		scanUIDirectory();
		needsScan = false;
		lastCheckTime = currentTime;
		return;
	}

	if( currentTime - lastCheckTime < CHECK_INTERVAL_MS )
		return;

	if( currentTime - lastReloadTime < COOLDOWN_MS )
		return;

	lastCheckTime = currentTime;
	pollFiles();
}

void UI_FileReloader::scanUIDirectory()
{
	UI_Main *ui = UI_Main::Get();
	if( !ui )
		return;

	trackedFiles.clear();

	UI_Main::UI_Navigation &nav = ui->getNavigation( UI_CONTEXT_MAIN );
	if( nav.empty() )
		return;

	NavigationStack *navigator = nav.front();
	if( !navigator )
		return;

	std::string basePath = navigator->getDefaultPath();
	if( basePath.empty() )
		return;

	std::string scanPath = basePath;
	if( scanPath[0] == '/' )
		scanPath = scanPath.substr( 1 );
	if( !scanPath.empty() && scanPath[scanPath.size()-1] == '/' )
		scanPath.erase( scanPath.size()-1, 1 );

	std::vector<std::string> files;

	getFileList( files, scanPath, ".rml", true );
	for( size_t i = 0; i < files.size(); i++ )
	{
		std::string fullPath = basePath + files[i];
		FileEntry entry;
		entry.path = fullPath;
		entry.lastMTime = FS_FileMTime( fullPath.c_str() + 1 );
		trackedFiles.push_back( entry );
	}

	std::string cssPath = scanPath + "/css";
	getFileList( files, cssPath, ".rcss", true );
	for( size_t i = 0; i < files.size(); i++ )
	{
		std::string fullPath = basePath + "css/" + files[i];
		FileEntry entry;
		entry.path = fullPath;
		entry.lastMTime = FS_FileMTime( fullPath.c_str() + 1 );
		trackedFiles.push_back( entry );
	}
}

void UI_FileReloader::pollFiles()
{
	for( size_t i = 0; i < trackedFiles.size(); i++ )
	{
		FileEntry &entry = trackedFiles[i];
		time_t mtime = FS_FileMTime( entry.path.c_str() + 1 );

		if( mtime != 0 && mtime != entry.lastMTime )
		{
			entry.lastMTime = mtime;
			Com_Printf( "UI_FileReloader: detected change in %s\n", entry.path.c_str() );

			lastReloadTime = lastCheckTime;

			UI_Main *ui = UI_Main::Get();
			if( !ui )
				return;

			UI_Main::UI_Navigation &nav = ui->getNavigation( UI_CONTEXT_MAIN );
			NavigationStack *navigator = nav.empty() ? nullptr : nav.front();
			if( !navigator )
				return;

			std::vector<std::string> names = navigator->getStackDocumentNames();
			std::string savedTop;
			if( !names.empty() )
				savedTop = names.back();

			std::string defaultPath = navigator->getDefaultPath();

			trap::Cmd_ExecuteText( EXEC_NOW, "ui_reload" );

			if( !savedTop.empty() && savedTop != defaultPath + "index.rml" )
			{
				std::string pageName = savedTop;
				if( pageName.substr( 0, defaultPath.size() ) == defaultPath )
					pageName = pageName.substr( defaultPath.size() );
				if( pageName.size() > 4 && pageName.substr( pageName.size() - 4 ) == ".rml" )
					pageName = pageName.substr( 0, pageName.size() - 4 );

				trap::Cmd_ExecuteText( EXEC_NOW, va( "menu_open %s", pageName.c_str() ) );
			}

			return;
		}
	}
}

}
