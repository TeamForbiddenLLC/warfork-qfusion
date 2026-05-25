/* notification bar */
class NotificationPopup
{
	float alpha;
	uint timerStart;
	uint timerUpdate;
	uint timerStayDuration;
	Element @notbar;

	NotificationPopup( const String &in id, const String &in text, uint stayTime, const String &in css_class = ".ok" )
	{
		// FIXME: cache this somehow?
		@this.notbar = window.document.body.getElementById( id );

		this.init( text, stayTime, css_class );
	}

	~NotificationPopup()
	{
		@this.notbar = null;
	}

	private	void init( const String &in text, uint stayTime, const String &in css_class )
	{
		Element @notbar = @this.notbar;

		if( @notbar == null ) {
			return;				
		}
		if ( text.length() == 0 ) {
			// empty text
			return;
		}
		if( stayTime < 0 ) // wtf?
			return; 
			
		timerStayDuration = stayTime;

		alpha = 0.0;
		timerStart = timerUpdate = window.time;
		
		/* set visibility and text */
		notbar.css( 'display', 'block' ).addClass( css_class );
		notbar.setInnerRML( text );

		window.setInterval( NotificationPopupCallbacks::FadeInCallback, 10, any(@this) );
	}
	
	bool fadeIn()
	{
		alpha += (window.time - timerUpdate) * 0.012; // fadein should be really fast
		timerUpdate = window.time;

		bool enabled = (alpha < 1.0);				
		if( !enabled ) {
			alpha = 1.0;
			window.setInterval( NotificationPopupCallbacks::StartFadeCallback, timerStayDuration, any(@this) );		
		}
		
		this.updateNotBarColorAlpha( 'color' );
		this.updateNotBarColorAlpha( 'background-color' );
		
		if( !enabled )
			return false;
		return true;
	}
	
	void startFade()
	{
		timerStart = timerUpdate = window.time;
		window.setInterval( NotificationPopupCallbacks::FadeCallback, 10, any(@this) );		
	}

	bool fade()
	{
		Element @notbar = @this.notbar;

		if( @notbar == null ) {
			return false;				
		}

		bool enabled;

		/* alpha fade background and font colors, preserving RGB values */
		alpha -= (window.time - timerUpdate) * 0.001;
		timerUpdate = window.time;

		enabled = (alpha > 0.0);				
		if( !enabled ) {
			/* DONE */
			alpha = 0.0;
			notbar.css( 'display', 'none' );
			return false;
		}

		this.updateNotBarColorAlpha( 'color' );
		this.updateNotBarColorAlpha( 'background-color' );
		return true;
	}

	private	void updateNotBarColorAlpha( const String &in colorProperty )
	{
		Element @notbar = @this.notbar;

		/* get current color value */
		String color = notbar.getProp( colorProperty );

		/* strip alpha from 'R, G, B, A' output */
		String rgb = color.subString( 0, color.locate( ',', 2 ) );
		String rgba = rgb + ', ' + (255 * alpha);

		/* set new value */
		notbar.css( colorProperty, 'rgba(' + rgba + ')' );			
	}
};

/* FIXME: move this to NotificationPopup as a public static method */
namespace NotificationPopupCallbacks
{
	bool FadeInCallback( any & obj )
	{
		NotificationPopup @popup;
		obj.retrieve(@popup);
		return popup.fadeIn();
	}

	bool StartFadeCallback( any & obj )
	{
		NotificationPopup @popup;
		obj.retrieve(@popup);
		popup.startFade();
		return false;
	}

	bool FadeCallback( any & obj )
	{
		NotificationPopup @popup;
		obj.retrieve(@popup);
		return popup.fade();
	}
}
