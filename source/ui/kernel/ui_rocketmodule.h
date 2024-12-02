#ifndef __UI_ROCKETMODULE_H__
#define __UI_ROCKETMODULE_H__

#include "kernel/ui_systeminterface.h"
#include "kernel/ui_fileinterface.h"
#include "kernel/ui_renderinterface.h"
#include "kernel/ui_keyconverter.h"

namespace WSWUI
{
	// RocketModule contains details and interface to libRocket
	class RocketModule
	{
		// some typical Rocket shortcuts
		typedef Rml::Element Element;
		typedef Rml::Event Event;

	public:
		RocketModule( int vidWidth, int vidHeight, float pixelRatio );
		~RocketModule();

		// post-initialization
		void registerCustoms();

		// pre-shutdown
		void unregisterCustoms();

		// cursor functions
		enum HideCursorBits {
			HIDECURSOR_REFRESH	= 1 << 0, // hidden by UI_Main::refreshScreen
			HIDECURSOR_INPUT	= 1 << 1, // hidden by an input source such as touchscreen
			HIDECURSOR_ELEMENT	= 1 << 2, // hidden by an element
			HIDECURSOR_ALL		= ( 1 << 3 ) - 1
		};
		void loadCursor( int contextId, const String& rmlCursor );
		void hideCursor( int contextId, unsigned int addBits, unsigned int clearBits );

		// system events
		void mouseMove( int contextId, int mousex, int mousey );
		void textInput( int contextId, wchar_t c );
		void keyEvent( int contextId, int key, bool pressed );
		bool touchEvent( int contextId, int id, touchevent_t type, int x, int y );
		bool isTouchDown( int contextId, int id );
		void cancelTouches( int contextId );

		void update( void );
		void render( int contextId );

		Rml::ElementDocument *loadDocument( int contextId, const char *filename, bool show=false, void *user_data = NULL );
		void closeDocument( Rml::ElementDocument *doc );

		// called from ElementInstancer after it instances an element, set up default
		// attributes, properties, events etc..
		void registerElementDefaults( Rml::Element *);

		// GET/SET Submodules
		UI_SystemInterface *getSystemInterface() { return systemInterface; }
		UI_FileInterface *getFileInterface() { return fsInterface; }
		UI_RenderInterface *getRenderInterface() { return renderInterface; }

		void clearShaderCache( void );
		void touchAllCachedShaders( void );

		int idForContext( Rml::Context *context );

	private:
		void registerElement( const char *tag, Rml::ElementInstancer* );
		void registerFontEffect( const char *name, Rml::FontEffectInstancer *);
		void registerDecorator( const char *name, Rml::DecoratorInstancer *);
		void registerEventInstancer( Rml::EventInstancer *);
		void registerEventListener( Rml::EventListenerInstancer *);

		// translates UI_CONTEXT_ constants to rocket contexts and vise versa
		Rml::Context *contextForId( int contextId );

		bool rocketInitialized;
		unsigned int hideCursorBits;

		struct contextTouch {
			int id;
			Rml::Vector2f origin;
			int y;
			bool scroll;
		};
		contextTouch contextsTouch[UI_NUM_CONTEXTS];

		UI_SystemInterface *systemInterface;
		UI_FileInterface *fsInterface;
		UI_RenderInterface *renderInterface;

		Rml::Context *contextMain;
		Rml::Context *contextQuick;

		// hold this so we can unref these in the end
		std::list<Rml::ElementInstancer*> elementInstancers;
		Rml::EventListenerInstancer *scriptEventListenerInstancer;
	};
}

#endif // __UI_ROCKETMODULE_H__
