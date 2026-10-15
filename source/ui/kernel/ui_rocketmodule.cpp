#include "ui_precompiled.h"
#include "kernel/ui_common.h"
#include "kernel/ui_main.h"
#include "kernel/ui_keyconverter.h"
#include "kernel/ui_rocketmodule.h"
#include "kernel/ui_eventlistener.h"

#include "widgets/ui_widgets.h"
#include "as/asui.h"

#include "decorators/ui_decorators.h"

#include "../gameshared/q_keycodes.h"

#include "parsers/ui_parsersound.h"

#include <Rocket/Core/FontEffectInstancer.h>
#include <Rocket/Controls.h>

namespace WSWUI
{

//==================================================

class MyEventInstancer : public Rocket::Core::EventInstancer
{
	typedef Rocket::Core::Event Event;
	typedef Rocket::Core::Element Element;
	typedef Rocket::Core::Dictionary Dictionary;

public:
	MyEventInstancer() : Rocket::Core::EventInstancer() {}
	~MyEventInstancer() {}

	// rocket overrides
	virtual Event *InstanceEvent(Element *target, const String &name, const Dictionary &parameters, bool interruptible)
	{
		// Com_Printf("MyEventInstancer: instancing %s %s\n", name.CString(), target->GetTagName().CString() );
		return __new__( Event )( target, name, parameters, interruptible );
	}

	virtual void ReleaseEvent(Event *event)
	{
		// Com_Printf("MyEventInstancer: releasing %s %s\n", event->GetType().CString(), event->GetTargetElement()->GetTagName().CString() );
		__delete__( event );
	}

	virtual void Release()
	{
		__delete__( this );
	}
};

//==================================================

RocketModule::RocketModule( int vidWidth, int vidHeight, float pixelRatio )
	: rocketInitialized( false ), hideCursorBits( 0 ),
	// pointers
	systemInterface(0), fsInterface(0), renderInterface(0), 
	contextMain(0), contextQuick(0)
{
	Rocket::Core::String contextName = trap::Cvar_String( "gamename" );

	renderInterface = __new__( UI_RenderInterface )( vidWidth, vidHeight, pixelRatio );
	Rocket::Core::SetRenderInterface( renderInterface );
	systemInterface = __new__( UI_SystemInterface )();
	Rocket::Core::SetSystemInterface( systemInterface );
	fsInterface = __new__( UI_FileInterface )();
	Rocket::Core::SetFileInterface( fsInterface );
	fontProviderInterface = __new__( UI_FontProviderInterface )( renderInterface );
	Rocket::Core::SetFontProviderInterface( fontProviderInterface );

	// TODO: figure out why renderinterface has +1 refcount
	renderInterface->AddReference();
	fontProviderInterface->AddReference();

	rocketInitialized = Rocket::Core::Initialise();
	if( !rocketInitialized )
		throw std::runtime_error( "UI: Rocket::Core::Initialise failed" );

	// initialize the controls plugin
	Rocket::Controls::Initialise();

	// Create our contexts
	contextMain = Rocket::Core::CreateContext( contextName, Vector2i( vidWidth, vidHeight ) );

	contextQuick = Rocket::Core::CreateContext( contextName + "_quick", Vector2i( vidWidth, vidHeight ) );
	if( contextQuick )
		contextQuick->ShowMouseCursor( false );

	contextsTouch[UI_CONTEXT_MAIN].id = -1;
	contextsTouch[UI_CONTEXT_QUICK].id = -1;
}

// here for hax0rz, TODO: move to "common" area
template<typename T>
void unref_object( T *obj ) {
	obj->RemoveReference();
}

RocketModule::~RocketModule()
{
	if( fontProviderInterface )
		fontProviderInterface->RemoveReference();
	
	if( contextMain )
		contextMain->RemoveReference();
	contextMain = 0;

	if( contextQuick )
		contextQuick->RemoveReference();
	contextQuick = 0;

	if( rocketInitialized )
		Rocket::Core::Shutdown();
	rocketInitialized = false;

	__SAFE_DELETE_NULLIFY( fontProviderInterface );
	__SAFE_DELETE_NULLIFY( fsInterface );
	__SAFE_DELETE_NULLIFY( systemInterface );
	__SAFE_DELETE_NULLIFY( renderInterface );
}

//==================================================

void RocketModule::mouseMove( int contextId, int mousex, int mousey )
{
	auto *context = contextForId( contextId );
	context->ProcessMouseMove( mousex, mousey, KeyConverter::getModifiers() );
}

void RocketModule::textInput( int contextId, wchar_t c )
{
	auto *context = contextForId( contextId );
	if( c >= ' ' )
		context->ProcessTextInput( c );
}

void RocketModule::keyEvent( int contextId, int key, bool pressed )
{
	// DEBUG
#if 0
	if( key >= 32 && key <= 126 )
		Com_Printf("**KEYEVENT CHAR %c\n", key & 0xff );
	else
		Com_Printf("**KEYEVENT KEY %d\n", key );
#endif

	if( key == K_MOUSE1DBLCLK )
		return; // Rocket handles double click internally

	auto *context = contextForId( contextId );
	Element *element = context->GetFocusElement();

	int mod = KeyConverter::getModifiers();

	// send the blur event, to the current focused element, when ESC key is pressed
	if( ( key == K_ESCAPE ) && element )
		element->Blur();

	if( element && ( element->GetTagName() == "keyselect" ) )
	{
		if( pressed )
		{
			Rocket::Core::Dictionary parameters;
			parameters.Set( "key", key );
			element->DispatchEvent( "keyselect", parameters );
		}
	}
	else if( key >= K_MOUSE1 && key <= K_MOUSE8 ) // warsow sends mousebuttons as keys
	{
		if( pressed )
			context->ProcessMouseButtonDown( key-K_MOUSE1, mod );
		else
			context->ProcessMouseButtonUp( key-K_MOUSE1, mod );
	}
	else if( key == K_MWHEELDOWN ) // and ditto for wheel
	{
		context->ProcessMouseWheel( 1, mod );
	}
	else if( key == K_MWHEELUP )
	{
		context->ProcessMouseWheel( -1, mod );
	}
	else
	{
		if( ( key == K_A_BUTTON ) || ( key == K_DPAD_CENTER ) )
		{
			if( pressed )
				context->ProcessMouseButtonDown( 0, mod );
			else
				context->ProcessMouseButtonUp( 0, mod );
		}
		else
		{
			int rkey = KeyConverter::toRocketKey( key );

			if( key == K_B_BUTTON )
			{
				rkey = Rocket::Core::Input::KI_ESCAPE;
				if( element )
					element->Blur();
			}

			if( rkey != 0 )
			{
				if( pressed )
					context->ProcessKeyDown( Rocket::Core::Input::KeyIdentifier( rkey ), mod );
				else
					context->ProcessKeyUp( Rocket::Core::Input::KeyIdentifier( rkey ), mod );
			}
		}
	}
}

bool RocketModule::touchEvent( int contextId, int id, touchevent_t type, int x, int y )
{
	auto &contextTouch = contextsTouch[contextId];
	auto *context = contextForId( contextId );

	if( ( type == TOUCH_DOWN ) && ( contextTouch.id < 0 ) ) {
		if( contextId == UI_CONTEXT_QUICK ) {
			Rocket::Core::Vector2f position( (float)x, (float)y );
			Element *element = context->GetElementAtPoint( position );
			if( !element || element->GetTagName() == "body" ) {
				return false;
			}
		}

		contextTouch.id = id;
		contextTouch.origin.x = x;
		contextTouch.origin.y = y;
		contextTouch.y = y;
		contextTouch.scroll = false;
	}

	if( id != contextTouch.id ) {
		return false;
	}

	UI_Main::Get()->mouseMove( contextId, x, y, true, false );

	if( type == TOUCH_DOWN ) {
		context->ProcessMouseButtonDown( 0, KeyConverter::getModifiers() );
	} else {
		int delta = contextTouch.y - y;
		if( delta ) {
			if( !contextTouch.scroll ) {
				int threshold = 32 * ( renderInterface->GetPixelsPerInch() / renderInterface->GetBasePixelsPerInch() );
				if( abs( delta ) > threshold ) {
					contextTouch.scroll = true;
					contextTouch.y += ( ( delta < 0 ) ? threshold : -threshold );
					delta = contextTouch.y - y;
				}
			}

			if( contextTouch.scroll ) {
				Element *focusElement = context->GetFocusElement();
				if( !focusElement || ( focusElement->GetTagName() != "keyselect" ) ) {
					Element *element;
					for( element = context->GetElementAtPoint( contextTouch.origin ); element; element = element->GetParentNode() ) {
						if( element->GetTagName() == "scrollbarvertical" ) {
							break;
						}

						int overflow = element->GetProperty< int >( "overflow-y" );
						if( ( overflow != Rocket::Core::OVERFLOW_AUTO ) && ( overflow != Rocket::Core::OVERFLOW_SCROLL ) ) {
							continue;
						}

						int scrollTop = element->GetScrollTop();
						if( ( ( delta < 0 ) && ( scrollTop > 0 ) ) ||
							( ( delta > 0 ) && ( element->GetScrollHeight() > scrollTop + element->GetClientHeight() ) ) ) {
							element->SetScrollTop( element->GetScrollTop() + delta );
							break;
						}
					}
				}
				contextTouch.y = y;
			}
		}

		if( type == TOUCH_UP ) {
			cancelTouches( contextId );
		}
	}

	return true;
}

bool RocketModule::isTouchDown( int contextId, int id )
{
	return contextsTouch[contextId].id == id;
}

void RocketModule::cancelTouches( int contextId )
{
	auto &contextTouch = contextsTouch[contextId];

	if( contextTouch.id < 0 ) {
		return;
	}

	auto *context = contextForId( contextId );

	contextTouch.id = -1;
	context->ProcessMouseButtonUp( 0, KeyConverter::getModifiers() );
	UI_Main::Get()->mouseMove( contextId, 0, 0, true, false );
}

//==================================================

#ifdef __WASM__
static bool WasmUiIsOptionsDocument( const char *filename )
{
	const char *p;

	if( !filename ) {
		return false;
	}
	for( p = filename; *p; p++ ) {
		if( ( *p == 'o' || *p == 'O' ) && !Q_strnicmp( p, "options", 7 ) ) {
			return true;
		}
	}
	return false;
}

static void WasmSet( Rocket::Core::Element *el, const char *prop, const char *value )
{
	if( el ) {
		el->SetProperty( prop, value );
	}
}

static void WasmClear( Rocket::Core::Element *el, const char *prop )
{
	if( el ) {
		el->RemoveProperty( prop );
	}
}

static bool WasmUiIsPhone( void )
{
	return trap::Cvar_Value( "in_touchcontrols" ) != 0;
}

static void InjectWebTouchOptionsTab( Rocket::Core::ElementDocument *document, const char *filename )
{
	if( !document || !WasmUiIsOptionsDocument( filename ) ) {
		return;
	}
	Rocket::Core::Element *sidebar = document->GetElementById( "sidebar2" );
	if( !sidebar || document->GetElementById( "opt-touch-link" ) ) {
		return;
	}
	Rocket::Core::Element *link = document->CreateElement( "a" );
	if( !link ) {
		return;
	}
	link->SetId( "opt-touch-link" );
	link->SetAttribute( "href", "options_touch.rml" );
	link->SetInnerRML( "Touch" );
	sidebar->AppendChild( link );
	link->RemoveReference();
}

static void ApplyWasmCompactUiToDocument( Rocket::Core::ElementDocument *document, int vidWidth, int vidHeight )
{
	Rocket::Core::Element *navi, *navibar, *content, *left, *sidebar, *sidebar2, *social, *footer;
	Rocket::Core::Element *grid, *frame, *serverblock, *parent;
	char heightCss[32];
	bool compact;
	int i, n, topBar, sideBarH, leftH, frameH;

	if( !document ) {
		return;
	}

	/* Only real phones (in_touchcontrols), never a resized desktop window. */
	compact = WasmUiIsPhone();
	navi = document->GetElementById( "navi" );
	navibar = document->GetElementById( "navibar" );
	content = document->GetElementById( "content" );
	left = document->GetElementById( "leftside" );
	sidebar = document->GetElementById( "sidebar" );
	sidebar2 = document->GetElementById( "sidebar2" );
	social = document->GetElementById( "social-links" );
	footer = document->GetElementById( "footer" );
	grid = document->GetElementById( "serverbrowser_datagrid" );
	frame = document->GetElementById( "serverbrowser_frame" );
	serverblock = document->GetElementById( "serverblock" );

	if( !compact ) {
		if( sidebar && left ) {
			parent = sidebar->GetParentNode();
			if( parent && parent == left->GetParentNode() && left->GetPreviousSibling() == sidebar ) {
				parent->InsertBefore( left, sidebar );
			}
		}
		WasmClear( navi, "top" );
		WasmClear( navi, "height" );
		WasmClear( navi, "white-space" );
		if( navi ) {
			n = navi->GetNumChildren();
			for( i = 0; i < n; i++ ) {
				Rocket::Core::Element *child = navi->GetChild( i );
				WasmClear( child, "font-size" );
				WasmClear( child, "padding" );
				WasmClear( child, "margin" );
			}
		}
		WasmClear( content, "top" );
		WasmClear( content, "left" );
		WasmClear( content, "padding" );
		WasmClear( content, "overflow-y" );
		WasmClear( content, "height" );
		document->RemoveAttribute( "wasm_phone_ui" );
		WasmClear( left, "float" );
		WasmClear( left, "width" );
		WasmClear( left, "height" );
		WasmClear( left, "overflow-y" );
		WasmClear( left, "position" );
		WasmClear( left, "top" );
		WasmClear( left, "left" );
		WasmClear( sidebar, "float" );
		WasmClear( sidebar, "width" );
		WasmClear( sidebar, "height" );
		WasmClear( sidebar, "border-left-width" );
		WasmClear( sidebar, "position" );
		WasmClear( sidebar, "top" );
		WasmClear( sidebar, "left" );
		WasmClear( sidebar, "overflow" );
		WasmClear( sidebar2, "height" );
		WasmClear( sidebar2, "overflow-y" );
		if( sidebar2 ) {
			n = sidebar2->GetNumChildren();
			for( i = 0; i < n; i++ ) {
				Rocket::Core::Element *child = sidebar2->GetChild( i );
				WasmClear( child, "display" );
				WasmClear( child, "width" );
				WasmClear( child, "margin" );
				WasmClear( child, "padding" );
				WasmClear( child, "font-size" );
			}
		}
		WasmClear( serverblock, "display" );
		if( navibar ) {
			n = navibar->GetNumChildren();
			for( i = 0; i < n; i++ ) {
				Rocket::Core::Element *child = navibar->GetChild( i );
				WasmClear( child, "width" );
				WasmClear( child, "display" );
				WasmClear( child, "font-size" );
				WasmClear( child, "padding" );
				WasmClear( child, "margin" );
			}
		}
		WasmClear( frame, "height" );
		WasmClear( frame, "overflow-y" );
		WasmClear( social, "display" );
		WasmClear( footer, "display" );
		WasmClear( document->GetElementById( "header" ), "height" );
		WasmClear( document->GetElementById( "header-bottom" ), "display" );
		WasmClear( document->GetElementById( "logo" ), "width" );
		WasmClear( document->GetElementById( "logo" ), "max-height" );
		WasmClear( document->GetElementById( "logo" ), "height" );
		WasmClear( document->GetElementById( "logo" ), "display" );
		WasmClear( document->GetElementById( "profile_container" ), "height" );
		WasmClear( document->GetElementById( "profile_container" ), "overflow" );
		WasmClear( document->GetElementById( "news_container" ), "display" );
		WasmClear( document->GetElementById( "friends_container" ), "display" );
		return;
	}

	topBar = 64;
	WasmSet( navi, "top", "2dp" );
	WasmSet( navi, "height", "auto" );
	WasmSet( navi, "white-space", "normal" );
	if( navi ) {
		n = navi->GetNumChildren();
		for( i = 0; i < n; i++ ) {
			Rocket::Core::Element *child = navi->GetChild( i );
			WasmSet( child, "font-size", "22dp" );
			WasmSet( child, "padding", "10dp 14dp" );
			WasmSet( child, "margin", "0dp 4dp" );
		}
	}

	WasmSet( content, "top", "58dp" );
	WasmSet( content, "left", "0dp" );
	WasmSet( content, "padding", "4dp 8dp" );
	WasmSet( content, "overflow-y", "auto" );
	if( vidHeight > topBar + 24 ) {
		Q_snprintfz( heightCss, sizeof( heightCss ), "%dpx", vidHeight - topBar - 8 );
		WasmSet( content, "height", heightCss );
	}

	/* Buttons first, server list below, so the grid stays on screen. */
	if( sidebar && left ) {
		parent = sidebar->GetParentNode();
		if( parent && parent == left->GetParentNode() && left->GetPreviousSibling() != sidebar ) {
			parent->InsertBefore( sidebar, left );
		}
	}

	WasmSet( sidebar, "float", "none" );
	WasmSet( sidebar, "position", "absolute" );
	WasmSet( sidebar, "top", "0dp" );
	WasmSet( sidebar, "left", "0dp" );
	WasmSet( sidebar, "width", "100%" );
	WasmSet( sidebar, "height", "auto" );
	WasmSet( sidebar, "overflow", "visible" );
	WasmSet( sidebar, "border-left-width", "0dp" );
	WasmSet( sidebar2, "height", "auto" );
	WasmSet( sidebar2, "overflow-y", "visible" );

	if( sidebar2 ) {
		n = sidebar2->GetNumChildren();
		for( i = 0; i < n; i++ ) {
			Rocket::Core::Element *child = sidebar2->GetChild( i );
			Rocket::Core::String tag = child->GetTagName();
			if( tag == "hr" || tag == "br" ) {
				WasmSet( child, "display", "none" );
				continue;
			}
			if( tag == "a" ) {
				Rocket::Core::String inner = child->GetInnerRML();
				if( inner.Find( "Server browser" ) != Rocket::Core::String::npos ) {
					WasmSet( child, "display", "none" );
					continue;
				}
				WasmSet( child, "display", "inline-block" );
				WasmSet( child, "width", "23%" );
				WasmSet( child, "margin", "2dp 1%" );
				WasmSet( child, "padding", "8dp 4dp" );
				WasmSet( child, "font-size", "16dp" );
				continue;
			}
			if( tag != "button" ) {
				continue;
			}
			WasmSet( child, "width", "30%" );
			WasmSet( child, "display", "inline-block" );
			WasmSet( child, "margin", "4dp 1%" );
			WasmSet( child, "padding", "10dp 6dp" );
			WasmSet( child, "font-size", "16dp" );
			if( child->GetId() == "favorite-btn" ) {
				child->SetInnerRML( "Favorite" );
			}
		}
	}

	WasmSet( serverblock, "display", "none" );

	if( navibar ) {
		n = navibar->GetNumChildren();
		for( i = 0; i < n; i++ ) {
			Rocket::Core::Element *child = navibar->GetChild( i );
			WasmSet( child, "width", "18%" );
			WasmSet( child, "display", "inline-block" );
			WasmSet( child, "font-size", "15dp" );
			WasmSet( child, "padding", "8dp 4dp" );
			WasmSet( child, "margin", "0dp 0.5%" );
		}
	}

	sideBarH = grid ? 108 : 0;
	leftH = vidHeight - topBar - sideBarH - 12;
	if( leftH < 120 ) {
		leftH = 120;
	}
	WasmSet( left, "float", "none" );
	WasmSet( left, "position", "absolute" );
	WasmSet( left, "left", "0dp" );
	Q_snprintfz( heightCss, sizeof( heightCss ), "%dpx", sideBarH );
	WasmSet( left, "top", heightCss );
	WasmSet( left, "width", "100%" );
	Q_snprintfz( heightCss, sizeof( heightCss ), "%dpx", leftH );
	WasmSet( left, "height", heightCss );
	WasmSet( left, "overflow-y", "auto" );

	frameH = leftH - 36;
	if( frameH < 80 ) {
		frameH = 80;
	}
	if( frame ) {
		Q_snprintfz( heightCss, sizeof( heightCss ), "%dpx", frameH );
		WasmSet( frame, "height", heightCss );
		WasmSet( frame, "overflow-y", "auto" );
	}

	if( grid ) {
		WasmSet( grid, "width", "100%" );
		WasmSet( grid, "font-size", "16dp" );
		n = grid->GetNumChildren();
		for( i = 0; i < n; i++ ) {
			Rocket::Core::Element *col = grid->GetChild( i );
			Rocket::Core::String fields = col->GetAttribute( "fields", Rocket::Core::String() );
			if( fields == "steam" ) {
				WasmSet( col, "display", "none" );
				WasmSet( col, "width", "0dp" );
			} else if( fields == "cleanname" ) {
				WasmSet( col, "width", "42%" );
			} else if( fields == "gametype" ) {
				WasmSet( col, "width", "72dp" );
			} else if( fields == "players" ) {
				WasmSet( col, "width", "56dp" );
			} else if( fields == "ping" ) {
				WasmSet( col, "width", "40dp" );
			}
		}
	}

	WasmSet( social, "display", "none" );
	WasmSet( footer, "display", "none" );
	WasmSet( document->GetElementById( "header" ), "height", "4dp" );
	WasmSet( document->GetElementById( "header-bottom" ), "display", "none" );
	{
		Rocket::Core::Element *logo = document->GetElementById( "logo" );
		if( logo ) {
			WasmSet( logo, "display", "block" );
			WasmSet( logo, "width", "auto" );
			WasmSet( logo, "height", "56dp" );
			WasmSet( logo, "margin-left", "auto" );
			WasmSet( logo, "margin-right", "auto" );
		}
		WasmSet( document->GetElementById( "profile_container" ), "height", "72dp" );
		WasmSet( document->GetElementById( "profile_container" ), "overflow", "hidden" );
		WasmSet( document->GetElementById( "news_container" ), "display", "none" );
		WasmSet( document->GetElementById( "friends_container" ), "display", "none" );
	}
}

void RocketModule::applyWasmCompactUi( int vidWidth, int vidHeight )
{
	int ctx, i, n;
	Rocket::Core::Context *contexts[2] = { contextMain, contextQuick };

	for( ctx = 0; ctx < 2; ctx++ ) {
		if( !contexts[ctx] ) {
			continue;
		}
		n = contexts[ctx]->GetNumDocuments();
		for( i = 0; i < n; i++ ) {
			ApplyWasmCompactUiToDocument( contexts[ctx]->GetDocument( i ), vidWidth, vidHeight );
		}
	}
}
#endif

Rocket::Core::ElementDocument *RocketModule::loadDocument( int contextId, const char *filename, bool show, void *user_data )
{
	auto *context = contextForId( contextId );
	ASUI::UI_ScriptDocument *document = dynamic_cast<ASUI::UI_ScriptDocument *>(context->LoadDocument( filename ));
	if( !document ) {
		return NULL;
	}

#ifdef __WASM__
	InjectWebTouchOptionsTab( document, filename );
	{
		int w = (int)trap::Cvar_Value( "vid_width" );
		int h = (int)trap::Cvar_Value( "vid_height" );
		ApplyWasmCompactUiToDocument( document, w, h );
	}
#endif

	if( show )
	{
		// load documents with autofocus disabled
		document->Show( Rocket::Core::ElementDocument::NONE );
		document->Focus();

		// reference counting may bog on us if we cache documents!
		document->RemoveReference();

		// optional element specific eventlisteners here

		// only for UI documents! FIXME: we are already doing this in NavigationStack
		Rocket::Core::EventListener *listener = UI_GetMainListener();
		document->AddEventListener( "keydown", listener );
		document->AddEventListener( "change", listener );
	}

	return document;
}

void RocketModule::closeDocument( Rocket::Core::ElementDocument *doc )
{
	doc->Close();
}

//==================================================

void RocketModule::registerElementDefaults( Rocket::Core::Element *element )
{
	// add these as they pile up in BaseEventListener
	element->AddEventListener( "mouseover", GetBaseEventListener() );
	element->AddEventListener( "click", GetBaseEventListener() );
}

void RocketModule::registerElement( const char *tag, Rocket::Core::ElementInstancer *instancer )
{
	Rocket::Core::Factory::RegisterElementInstancer( tag, instancer );
	instancer->RemoveReference();
	elementInstancers.push_back( instancer );
}

void RocketModule::registerFontEffect( const char *name, Rocket::Core::FontEffectInstancer *instancer )
{
	Rocket::Core::Factory::RegisterFontEffectInstancer( name, instancer );
	instancer->RemoveReference();
}

void RocketModule::registerDecorator( const char *name, Rocket::Core::DecoratorInstancer *instancer )
{
	Rocket::Core::Factory::RegisterDecoratorInstancer( name, instancer );
	instancer->RemoveReference();
}

void RocketModule::registerEventInstancer( Rocket::Core::EventInstancer *instancer )
{
	Rocket::Core::Factory::RegisterEventInstancer( instancer );
	instancer->RemoveReference();
}

void RocketModule::registerEventListener( Rocket::Core::EventListenerInstancer *instancer )
{
	Rocket::Core::Factory::RegisterEventListenerInstancer( instancer );
	instancer->RemoveReference();
}

Rocket::Core::Context *RocketModule::contextForId( int contextId )
{
	switch( contextId ) {
		case UI_CONTEXT_MAIN:
			return contextMain;
		case UI_CONTEXT_QUICK:
			return contextQuick;
		default:
			assert( contextId != UI_CONTEXT_MAIN && contextId != UI_CONTEXT_QUICK );
			return NULL;
	}
}

int RocketModule::idForContext( Rocket::Core::Context *context )
{
	if( context == contextMain )
		return UI_CONTEXT_MAIN;
	if( context == contextQuick )
		return UI_CONTEXT_QUICK;
	return UI_NUM_CONTEXTS;
}

// Load the mouse cursor and release the caller's reference:
// NOTE: be sure to use it before initRocket( .. ) function
void RocketModule::loadCursor( int contextId, const String& rmlCursor )
{
	Rocket::Core::ElementDocument* cursor = contextForId( contextId )->LoadMouseCursor( rmlCursor );

	if( cursor )
		cursor->RemoveReference();
}

void RocketModule::hideCursor( int contextId, unsigned int addBits, unsigned int clearBits )
{
	if( contextId == UI_CONTEXT_QUICK ) {
		contextQuick->ShowMouseCursor( false );
		return;
	}

	hideCursorBits = ( hideCursorBits & ~clearBits ) | addBits;
	contextForId( contextId )->ShowMouseCursor( hideCursorBits == 0 );
}

void RocketModule::update( void )
{
	ASUI::GarbageCollectEventListenersFunctions( scriptEventListenerInstancer );

	contextQuick->Update();
	contextMain->Update();
#ifdef __WASM__
	applyWasmCompactUi( (int)trap::Cvar_Value( "vid_width" ), (int)trap::Cvar_Value( "vid_height" ) );
#endif
}

void RocketModule::updateScreenSize( int vidWidth, int vidHeight )
{
	Rocket::Core::Vector2i dims( vidWidth, vidHeight );

	if( renderInterface ) {
		renderInterface->SetViewport( vidWidth, vidHeight );
	}
	if( contextMain ) {
		contextMain->SetDimensions( dims );
	}
	if( contextQuick ) {
		contextQuick->SetDimensions( dims );
	}
#ifdef __WASM__
	applyWasmCompactUi( vidWidth, vidHeight );
#endif
}

void RocketModule::render( int contextId )
{
	contextForId( contextId )->Render();
}

void RocketModule::registerCustoms()
{
	//
	// ELEMENTS
	registerElement( "*", __new__( GenericElementInstancer<Element> )() );

	// Main document that implements <script> tags
	registerElement( "body", ASUI::GetScriptDocumentInstancer() );
	// Soft keyboard listener
	registerElement( "input",
		__new__( GenericElementInstancerSoftKeyboard<Rocket::Controls::ElementFormControlInput> )() );
	registerElement( "textarea",
		__new__( GenericElementInstancerSoftKeyboard<Rocket::Controls::ElementFormControlTextArea> )() );
	// other widgets
	registerElement( "keyselect", GetKeySelectInstancer() );
	registerElement( "a", GetAnchorWidgetInstancer() );
	registerElement( "optionsform", GetOptionsFormInstancer() );
	registerElement( "levelshot", GetLevelShotInstancer() );
	registerElement( "datagrid", GetSelectableDataGridInstancer() );
	registerElement( "dataspinner", GetDataSpinnerInstancer() );
	registerElement( "modelview", GetModelviewInstancer() );
	registerElement( "worldview", GetWorldviewInstancer() );
	registerElement( "colorselector", GetColorSelectorInstancer() );
	registerElement( "color", GetColorBlockInstancer() );
	registerElement( "idiv", GetInlineDivInstancer() );
	registerElement( "img", GetImageWidgetInstancer() );
	registerElement( "field", GetElementFieldInstancer() );
	registerElement( "video", GetVideoInstancer() );
	registerElement( "iframe", GetIFrameWidgetInstancer() );
	registerElement( "l10n", GetElementL10nInstancer() );

	//
	// EVENTS
	registerEventInstancer( __new__( MyEventInstancer )() );

	//
	// EVENT LISTENERS

	// inline script events
	scriptEventListenerInstancer = ASUI::GetScriptEventListenerInstancer();
	scriptEventListenerInstancer->AddReference();

	registerEventListener( scriptEventListenerInstancer );

	//
	// FONT EFFECTS

	//
	// DECORATORS
	registerDecorator( "gradient", GetGradientDecoratorInstancer() );
	registerDecorator( "ninepatch", GetNinePatchDecoratorInstancer() );

	//
	// GLOBAL CUSTOM PROPERTIES

	Rocket::Core::StyleSheetSpecification::RegisterProperty("background-music", "", false).AddParser("string");

	Rocket::Core::StyleSheetSpecification::RegisterParser("sound", new PropertyParserSound());

	Rocket::Core::StyleSheetSpecification::RegisterProperty("sound-hover", "", false)
		.AddParser("sound");
	Rocket::Core::StyleSheetSpecification::RegisterProperty("sound-click", "", false)
		.AddParser("sound");
}

void RocketModule::unregisterCustoms()
{
	if( scriptEventListenerInstancer ) {
		ASUI::ReleaseScriptEventListenersFunctions( scriptEventListenerInstancer );
		scriptEventListenerInstancer->RemoveReference();
		scriptEventListenerInstancer = NULL;
	}
}

//==================================================

void RocketModule::clearShaderCache( void )
{
	renderInterface->ClearShaderCache();
}

void RocketModule::touchAllCachedShaders( void )
{
	renderInterface->TouchAllCachedShaders();
}

}
