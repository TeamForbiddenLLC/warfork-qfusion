/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "crashpad.h"

#ifdef USE_CRASHPAD
#include <map>
#include <string>
#include <vector>

#include "client/crashpad_client.h"
#include "client/crash_report_database.h"
#include "client/settings.h"
#include "version.h"

std::unique_ptr<crashpad::CrashReportDatabase> db;
crashpad::CrashpadClient* client;

void Exit_Crashpad() {
	db = NULL;
	delete client;
}

bool Init_Crashpad( const char *dir )
{
	// Cache directory that will store crashpad information and minidumps
#if BUILDFLAG(IS_POSIX)
	base::FilePath database = base::FilePath( dir).Append( FILE_PATH_LITERAL("crashpad") );
#elif BUILDFLAG(IS_WIN)
	base::FilePath database = base::FilePath(base::FilePath::StringType(dir, dir + std::char_traits<wchar_t>::length(dir))).Append( FILE_PATH_LITERAL("crashpad") );
#endif
	// Path to the out-of-process handler executable
	base::FilePath handler = base::FilePath(FILE_PATH_LITERAL("crashpad_handler.x86_64"));
  std::string minidump_url(APP_CRASHPAD_DUMP_URI);
 
	std::string http_proxy( "" );
	std::map<std::string, std::string> annotations;
	std::vector<std::string> arguments;
	client = new crashpad::CrashpadClient;

	arguments.push_back( "--no-rate-limit" );
	db = crashpad::CrashReportDatabase::Initialize( database );
	if( db != nullptr && db->GetSettings() != nullptr ) {
		db->GetSettings()->SetUploadsEnabled( true );
	}
  std::vector<base::FilePath> attachments;


	bool success = client->StartHandler( handler, database, database, minidump_url, http_proxy, annotations, arguments,
										 /* restartable */ true,
										 /* asynchronous_start */ false, attachments);

	return success;
}
#endif
