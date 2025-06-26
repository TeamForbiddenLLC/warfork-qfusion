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

funcdef void callback();

const int EASE_NONE = 0;
const int EASE_IN = 1;
const int EASE_OUT = 2;
const int EASE_INOUT = 3;

Vec3 ANIM_LEFT( -120, 0, 0 );
Vec3 ANIM_BOTTOM( 0, 100, 0 );
Vec3 ANIM_RIGHT( 100, 0, 0 );
Vec3 ANIM_TOP( 0, -100, 0 );
Vec3 ANIM_ORIGIN( 0, 0, 0 );

const int ANIMATION_DURATION_FAST = 150;		// how long will animations last?

const int ANIMATION_TICK = 10;

// This class will move your element from a certain point to another point. cVec3s need to be percantage values!
// Be aware that the elements position-property will be set to 'relative'!
// Will call the callback as soon as it is done with animation.
class MoveAnimation
{
	int animTime;
	int animStartTime;
	int animLastTime;
	int animMoveTime;
	Vec3 animStart;
	Vec3 animDest;
	Element @animElement;
	int animEase;
	bool ceased;

	callback @animDoneCallback;
	
	MoveAnimation( Element @elem, int time, Vec3 start, Vec3 dest, int ease, callback @animDoneCallback = null )
	{
		@this.animElement = @elem;
		this.animTime = time;
		this.animStart = start;
		this.animDest = dest;
		this.animEase = ease;
		this.ceased = false;
		@this.animDoneCallback = @animDoneCallback;
		if( time <= 0 ) {
			animate();
		}
		else {
			this.startAnimation();
		}
	}
	
	~MoveAnimation( )
	{
		@this.animDoneCallback = null;
		@this.animElement = null;
	}
	
	private void startAnimation()
	{
		if( @animElement == null || animTime <= 0 )
			return;
		animMoveTime = 0;
		animStartTime = animLastTime = window.time;
		animElement.css( 'position', 'relative' )
			.css( 'left', animStart.x + "%" )
			.css( 'top', animStart.y + "%" );
		window.setInterval( __MoveAnimationCallback, ANIMATION_TICK, any(@this) );
	}
	
	bool animate() // do not call this one, it's just for the scheduler
	{
		if( @this.animElement == null )
			return false; // something went wrong
			
		if( this.ceased )
		{
			if( @this.animDoneCallback is null )	// check whether we need to inform someone that we are done
				return false;						// doesn't look like it
			else
				this.animDoneCallback();
			return false;		
		}

		float frac;
		
		if( this.animTime > 0 )
		{
			int moveTime = window.time - this.animLastTime;			
			if( moveTime > ANIMATION_TICK )
				moveTime = ANIMATION_TICK;

			animMoveTime += moveTime;
			frac = float(animMoveTime) / this.animTime;
			frac = applyEase( frac, this.animEase );
			if( frac > 1 )
				frac = 1;
		}
		else
		{
			frac = 1;
		}
	
		this.animLastTime = window.time;
		this.setElementPosition( 'left', animStart.x, animDest.x, frac );
		this.setElementPosition( 'top', animStart.y, animDest.y, frac );

		if( frac == 1 ) 							// we are done
			this.ceased = true;
		return true; // continue to call this function
	}
	
	private void setElementPosition( String prop, float start, float dest, float frac )
	{
		float tmp = dest - start;
		tmp = start + tmp * frac;
		this.animElement.css( prop, int( tmp ) + "%" );
	}		
}

float applyEase( float x, int ease ) // x needs to be between 0 and 1
{
	if( ease == EASE_NONE ) // FIXME: switch()?
		return x;
	else if( ease == EASE_IN )
		return x*x*x*x; // f(x)=x^4
	else if( ease == EASE_OUT )
		return pow( x, 0.25 ); // f(x)=x^0.25
	else if( ease == EASE_INOUT )
		return x * x * ( -2.0f * x + 3.0f ); // f(x)=-2x^3+3x^2 -- That math was fucking hard if you're not used to to that kind of stuff...
	return 0;
}

bool __MoveAnimationCallback( any & obj ) // this one will serve as a relay, handing scheduler-call over to the class
{
	MoveAnimation @anim;
	obj.retrieve(@anim);
	
	if( @anim == null )
		return false; // something went wrong, just disable that scheduler
	
	return anim.animate();
}
