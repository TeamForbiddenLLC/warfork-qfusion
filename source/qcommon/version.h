/*
Copyright (C) 2007 Victor Luchits

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

#ifndef _WARFORK_VERSION_H_
#define _WARFORK_VERSION_H_

#undef STR_HELPER
#undef STR_TOSTR

#define STR_HELPER( s )                 # s
#define STR_TOSTR( x )                  STR_HELPER( x )

#define APPLICATION           "Warfork"
#define APPLICATION_UTF8      "Warfork"
#define DEFAULT_BASEGAME      "basewf"

#define APP_VERSION_MAJOR     2
#define APP_VERSION_MINOR     15
#define APP_VERSION_UPDATE    0 

// ...data21.pk3. we should never need to bump this
#define APP_PK3_VERSION       "21"

#ifdef PUBLIC_BUILD
#define APP_PROTOCOL_VERSION      27
#else
#define APP_PROTOCOL_VERSION      2400
#endif

#ifdef PUBLIC_BUILD
#define APP_DEMO_PROTOCOL_VERSION   22
#else
#define APP_DEMO_PROTOCOL_VERSION   22
#endif

#define APP_DEMO_PROTOCOL_VERSION_STR STR_TOSTR( APP_DEMO_PROTOCOL_VERSION )
#define APP_DEMO_EXTENSION_STR        ".wfdz" APP_DEMO_PROTOCOL_VERSION_STR
#define APP_URI_SCHEME                APPLICATION "://"
#define APP_URI_PROTO_SCHEME          APPLICATION STR_TOSTR( APP_PROTOCOL_VERSION ) "://"
#define APP_VERSION_STR               STR_TOSTR( APP_VERSION_MAJOR ) "." STR_TOSTR( APP_VERSION_MINOR ) "." STR_TOSTR( APP_VERSION_UPDATE )
#define APP_VERSION_STR_MAJORMINOR    STR_TOSTR( APP_VERSION_MAJOR ) STR_TOSTR( APP_VERSION_MINOR )
#define APP_PROTOCOL_VERSION_STR      STR_TOSTR( APP_PROTOCOL_VERSION )

//
// the following macros are only used by the windows resource file
//
#define APP_VERSION_RC              APP_VERSION_MAJOR.APP_VERSION_MINOR
#define APP_VERSION_RC_STR          STR_TOSTR( APP_VERSION_RC )
#define APP_FILEVERSION_RC          APP_VERSION_MAJOR,APP_VERSION_MINOR,APP_VERSION_UPDATE,0
#define APP_FILEVERSION_RC_STR      STR_TOSTR( APP_FILEVERSION_RC )

#ifdef PUBLIC_BUILD
#define APP_MATCHMAKER_URL        "https://warfork.com:1338"
#define APP_MATCHMAKER_WEB_URL    "https://warfork.com/wfmm/"
#else
#define APP_MATCHMAKER_URL        "http://warfork.com:1337"
#define APP_MATCHMAKER_WEB_URL    "http://warfork.com/wfmm/"
#endif
#define APP_URL                    "http://warfork.com/"

#define APP_UI_BASEPATH       "ui/porkui"
#define APP_STARTUP_COLOR     0x1c1416
#define APP_STEAMID           671610
#define APP_SCREENSHOTS_PREFIX     "wf_"
#define APP_COPYRIGHT_OWNER        "Team Forbidden LLC"
#define APP_DEFAULT_LANGUAGE  "en"
#define APP_ICO_ICON          "../../icons/warfork.ico"
#define APP_DEMO_ICO_ICON     "../../icons/warfork-demo.ico"
#define APP_XPM_ICON          "../../icons/warfork256x256.xpm"
#define APP_HOME_UNIX_DIR_FS  "warfork-2.1"
#define APP_HOME_OSX_DIR_FS   "warfork-2.1"
#define APP_HOME_WIN_DIR_FS   "Warfork 2.1"



#endif
