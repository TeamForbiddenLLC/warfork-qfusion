/*
Copyright (C) 2012 Chasseur de bots

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

const String S_COLOR_BLACK = "^0";
const String S_COLOR_RED = "^1";
const String S_COLOR_GREEN = "^2";
const String S_COLOR_YELLOW = "^3";
const String S_COLOR_BLUE = "^4";
const String S_COLOR_CYAN = "^5";
const String S_COLOR_MAGENTA = "^6";
const String S_COLOR_WHITE = "^7";
const String S_COLOR_ORANGE = "^8";
const String S_COLOR_GREY = "^9";

/*
* Converts "R G B" decimal representation to #hhhhhh representation
*/
String @RGB2Hex( const String &in rgb )
{
	array<String @> @components = StringUtils::Split( rgb, " " );

	String hex = "#";

	uint count = 0;
	uint components_size = components.size();
	for( uint i = 0; i < components_size; i++ ) {
		String @component = components[i];
		if( component.empty() ) {
			continue;
		}
			
		hex += StringUtils::FormatInt( component.toInt(), '0h', 2 );

		count++;
		if( count == 3 ) {
			// parsed all 3 components
			break;
		}
	}
	
	return hex;
}

/*
* Converts #hhhhhh representation to "R G B" decimal representation
*/
const String @Hex2RGB( const String &in hex )
{
	if( hex.length() < 7 || hex.substr( 0, 1 ) != '#' ) {
		// wrong format
		return '0 0 0';
	}

	uint r = StringUtils::Strtol( hex.substr( 1, 2 ), 16 ) & 0xff;
	uint g = StringUtils::Strtol( hex.substr( 3, 2 ), 16 ) & 0xff;
	uint b = StringUtils::Strtol( hex.substr( 5, 2 ), 16 ) & 0xff;

	return '' + r + ' ' + g + ' ' + b;
}

void OpenQuitDialog( void )
{
	window.modal('modal_basic.rml?text=Are%20you%20sure%20you%20want%20to%20quit%3F&ypos=0.25');
	if (window.getModalValue() == 0) 
		game.execAppend('quit\n');
}
