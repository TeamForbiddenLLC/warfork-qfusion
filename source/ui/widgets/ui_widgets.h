/*
 * widgets.h
 *
 *  Created on: 29.6.2011
 *      Author: hc
 */

#ifndef __WIDGETS_H__
#define __WIDGETS_H__

#include "kernel/ui_main.h"
#include "kernel/ui_eventlistener.h"
#include <RmlUi/Core/Element.h>

namespace WSWUI
{

	// "my generic element instancer"
	template<typename T>
	struct GenericElementInstancer : Rml::ElementInstancer
	{
		Rml::Element *InstanceElement(Rml::Element *parent, const String &tag, const Rml::Core::XMLAttributes &attributes)
		{
			Rml::Element *elem = __new__(T)( tag );
			UI_Main::Get()->getRocket()->registerElementDefaults(elem);
			return elem;
		}

		void ReleaseElement(Rml::Element *element)
		{
			__delete__(element);
		}

		void Release ()
		{
			__delete__(this);
		}
	};

	// "my generic element instancer" that sends attributes to the child
	template<typename T>
	struct GenericElementInstancerAttr : Rml::ElementInstancer
	{
		Rml::Element *InstanceElement(Rml::Element *parent, const String &tag, const Rml::Core::XMLAttributes &attributes)
		{
			Rml::Element *elem = __new__(T)( tag, attributes );
			UI_Main::Get()->getRocket()->registerElementDefaults(elem);
			return elem;
		}

		void ReleaseElement(Rml::Element *element)
		{
			__delete__(element);
		}

		void Release ()
		{
			__delete__(this);
		}
	};

	// "my generic element instancer" that attaches click/blur events that toggle the soft keyboard
	template<typename T>
	struct GenericElementInstancerSoftKeyboard : GenericElementInstancer<T>
	{
		Rml::Element *InstanceElement(Rml::Element *parent, const String &tag, const Rml::Core::XMLAttributes &attributes)
		{
			Rml::Element *elem = GenericElementInstancer<T>::InstanceElement( parent, tag, attributes );
			elem->AddEventListener( "click", UI_GetSoftKeyboardListener() );
			elem->AddEventListener( "blur", UI_GetSoftKeyboardListener() );
			return elem;
		}
	};

	//=======================================

	Rml::ElementInstancer *GetKeySelectInstancer( void );
	Rml::ElementInstancer *GetAnchorWidgetInstancer( void );
	Rml::ElementInstancer *GetOptionsFormInstancer( void );
	Rml::ElementInstancer *GetLevelShotInstancer(void);
	Rml::ElementInstancer *GetSelectableDataGridInstancer(void);
	Rml::ElementInstancer *GetDataSpinnerInstancer( void );
	Rml::ElementInstancer *GetModelviewInstancer( void );
	Rml::ElementInstancer *GetWorldviewInstancer( void );
	Rml::ElementInstancer *GetColorBlockInstancer( void );
	Rml::ElementInstancer *GetColorSelectorInstancer( void );
	Rml::ElementInstancer *GetInlineDivInstancer( void );
	Rml::ElementInstancer *GetImageWidgetInstancer( void );
	Rml::ElementInstancer *GetElementFieldInstancer( void );
	Rml::ElementInstancer *GetVideoInstancer( void );
	Rml::ElementInstancer *GetIFrameWidgetInstancer( void );
	Rml::ElementInstancer *GetElementL10nInstancer( void );
}

#endif /* __WIDGETS_H__ */
