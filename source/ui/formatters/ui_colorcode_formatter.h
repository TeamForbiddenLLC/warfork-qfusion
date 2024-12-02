/*
Copyright (C) 2011 Victor Luchits

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

#ifndef __UI_COLORCODE_FORMATTER_H__
#define __UI_COLORCODE_FORMATTER_H__

#include <time.h>
#include <stddef.h>
#include <Rocket/Controls/DataFormatter.h>

namespace WSWUI
{

class ColorCodeFormatter : public Rml::DataFormatter
{
public:
	ColorCodeFormatter() : Rml::DataFormatter("colorcode") {}

	// Formats string into colored spans
	// FIXME: this isn't terribly efficient, converting UTF-8 characters back and forth
	void FormatData( Rml::String& formatted_data, const Rml::StringList& raw_data )
	{
		formatted_data = "";

		// emit styled text span for each colored text block.
		for( Rml::StringList::const_iterator it = raw_data.begin(); it != raw_data.end(); it++ ) {
			int colorindex, old_colorindex;
			int gc;
			wchar_t num;
			const char *s;
			Rml::String colorblock;

			colorblock = "";
			colorindex = old_colorindex = -1;

			s = it->CString();
			while( s ) {
				gc = Q_GrabWCharFromColorString( &s, &num, &colorindex, NULL, NULL, NULL );

				if( gc == GRABCHAR_CHAR ) {
					colorblock += Q_WCharToUtf8Char( num );
				}
				else if( gc == GRABCHAR_COLOR ) {
					if( !colorblock.Empty() ) {
						htmlEncode( colorblock );
						formatted_data += colorblock;
						colorblock = "";
					}

					if( old_colorindex != -1 ) {
						formatted_data += "</span>";
					}

					if( colorindex < 0 || colorindex >= MAX_S_COLORS ) {
						colorindex = -1;
					} else {
						vec_t *c = color_table[colorindex];
						formatted_data += Rml::String( 64,
							"<span style=\"color:rgb(%i%%,%i%%,%i%%);\">",
							bound(0,(int)(c[0] * 100.0f),100),
							bound(0,(int)(c[1] * 100.0f),100),
							bound(0,(int)(c[2] * 100.0f),100)
						);
					}

					old_colorindex = colorindex;
				}
				else if( gc == GRABCHAR_END ) {
					if( !colorblock.Empty() ) {
						htmlEncode( colorblock );
						formatted_data += colorblock;
					}

					if( colorindex != -1 ) {
						formatted_data += "</span>";
					}
					break;
				}
				else if (gc == GRABCHAR_ANSI || gc == GRABCHAR_RGB ) {
					// ignore
					break;
				}
				else
					assert( 0 );
			}
		}
	}

	// FIXME: this is a mess...

	// html encode single string inplace
	void htmlEncode( Rml::String &s )
	{
		s = s.Replace( "&", "&amp;" );
		s = s.Replace( "<", "&lt;" );
		s = s.Replace( ">", "&gt;" );
		s = s.Replace( "\"", "&quot;" );
		s = s.Replace( "\n", "<br/>" );
	}
};

}

#endif // __UI_COLORCODE_FORMATTER_H__
