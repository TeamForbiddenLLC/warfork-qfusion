/*
Copyright (C) 2013 Chasseur de bots

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

namespace Callvotes
{
	bool playerHasVoted( int playerNum )
	{
		String votes = ::game.cs( ::CS_ACTIVE_CALLVOTE_VOTES );

		if (votes.empty()) {
			return false;
		}

		array<String @>vectors = StringUtils::Split( votes, ' ' );
		int vectorid = playerNum / 31;

		if( playerNum < 0 || vectorid >= int(vectors.length()) ) {
			return false;
		}
		
		String hex = '0x' + vectors[vectorid];
		int bits = hex.toInt();
		bool voted = bits & (1<<(playerNum&31)) != 0;
		return voted;
	}
}
