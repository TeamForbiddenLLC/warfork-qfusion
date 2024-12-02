/*
 * UI_EventListener.h
 *
 *  Created on: 27.6.2011
 *      Author: hc
 */

#ifndef UI_EVENTLISTENER_H_
#define UI_EVENTLISTENER_H_

#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>

namespace WSWUI
{

// just testing stuff
class BaseEventListener : public Rml::EventListener
{
	typedef Rml::Event Event;

public:
	BaseEventListener();
	virtual ~BaseEventListener();

	virtual void ProcessEvent( Event &event );

private:
	virtual void StartTargetPropertySound( Rml::Element *target, const Rml::String &property );
};

// Basic event listener with default handling for all elements
Rml::EventListener *GetBaseEventListener( void );
// get instance of global eventlistener
Rml::EventListener *UI_GetMainListener( void );
// get instance of eventlistener that opens the soft keyboard for text inputs
Rml::EventListener *UI_GetSoftKeyboardListener( void );

}

#endif /* UI_EVENTLISTENER_H_ */
