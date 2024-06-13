/*
Copyright (C) 2015 Chasseur de bots

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

namespace Tutorial
{

void OpenModal1()
{
	Cvar ui_tutorial_launch( 'ui_tutorial_launch', '0', 0 );
	Cvar ui_tutorial_taken( 'ui_tutorial_taken', '0', ::CVAR_ARCHIVE );
	int taken = ui_tutorial_launch.integer | ui_tutorial_taken.integer;

	if( taken == 0 ) {
		// don't ask again this game launch
		ui_tutorial_launch.set( '1' );

		String text = "%3Ch3%3EWelcome%20to%20Warfork%21%3C%2Fh3%3E";

		text += "%3Ct%3EWarfork+is+an+Early+Access+game+currently+in+development.+All+maps%2C+models%2C+textures%2C+etc.+are+in+the+process+of+being+recreated.+In+the+interim+there+will+be+no+marketing+and+player+population+will+remain+low.+We+invite+you+to+join+us+on+Warfork+Wednesdays+%28Europe%29+and+Fork+Fridays+%28North+America%29.+Please+join+chat+for+more+information%21%3C%2Ft%3E";
		window.modal( "modal_basic.rml?text=" + text + "&other=Don't%20tell%20me%20again&ypos=0.5" );

		int val = window.getModalValue();
		if( val == 0 ) {
			ui_tutorial_taken.set( '1' );
			return;
		}

		if( val == 1 ) {
			// no/cancel
		}		
		else if( val == 2 ) {
			ui_tutorial_taken.set( '2' );
		}
	}
}

void OpenModal2()
{
	window.modal('modal_basic.rml?text=You%20are%20about%20to%20launch%20the%20tutorial%20stage%20of%20Warfork.%20Continue%3F&ypos=0.25');
	if (window.getModalValue() == 0) 
		game.execAppend('map wftutorial1\n');
}

}
