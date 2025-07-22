/*
Copyright (C) 2012 Jannik Kolodziej ("drahtmaul")

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

namespace Favorites
{
	void init()
	{
		// cycle through favorites
		int favcount = Cvar( 'favorites', '0', ::CVAR_ARCHIVE ).integer;
		for( int i = 1; i <= favcount; i++ ) 
			::serverBrowser.addFavorite( Cvar( 'favorite_' + i, '', ::CVAR_ARCHIVE ).string ); // inform UI of the favorites
	}

	void add( const String &address )
	{
		if( address.empty() ) {
			return;
		}
		if( !::serverBrowser.addFavorite( address ) ) {
			// duplicate entry, or maybe some other error
			return;
		}

		Cvar favorites( 'favorites', '0', ::CVAR_ARCHIVE );
		int favcount = favorites.integer;

		Cvar favorite( 'favorite_' + (favcount + 1), '', ::CVAR_ARCHIVE );
		favorite.set( address );

		// update counter
		favorites.set( favcount + 1 );
	}
	
	void remove( const String &address )
	{
		if( address.empty() ) {
			return;
		}
		if( !::serverBrowser.removeFavorite( address ) ) {
			// no such server in favorites or some other error?
			return;
		}

		int favslot = getSlot( address );
		if( favslot == 0 )
			return;

		// replace this entry with the last one as we don't care about the 
		// order we store favorites in
		Cvar favorites( 'favorites', '0', ::CVAR_ARCHIVE );
		int favcount = favorites.integer;

		Cvar favlast( 'favorite_' + favcount, '', ::CVAR_ARCHIVE );
		Cvar favorite( 'favorite_' + favslot, '', ::CVAR_ARCHIVE );

		favorite.set( favlast.string );
		favlast.set( '' );

		// update counter
		favorites.set( favcount - 1 );
	}
	
	int getSlot( const String &address )
	{
		int favcount = Cvar( 'favorites', '', ::CVAR_ARCHIVE ).integer;
		for( int i = 1; i <= favcount; i++ ) 
		{
			if( Cvar( 'favorite_' + i, '', ::CVAR_ARCHIVE ).string == address )
				return i;
		}
		return 0;
	}
}
