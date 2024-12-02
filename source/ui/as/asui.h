#pragma once
#ifndef __ASUI_H__
#define __ASUI_H__

#include "as/asmodule.h"

namespace ASUI {

	// asui.cpp
	void BindAPI( ASInterface *as );
	void BindGlobals( ASInterface *as );
	void BindFrame( ASInterface *as );
	void BindShutdown( ASInterface *as );

	// asui_scriptdocument.cpp
	Rml::ElementInstancer *GetScriptDocumentInstancer( void );

	// asui_scriptevent.cpp
	Rml::EventListenerInstancer *GetScriptEventListenerInstancer( void );
	/// Releases Angelscript function pointers held by event listeners
	void ReleaseScriptEventListenersFunctions( Rml::EventListenerInstancer * );
	void GarbageCollectEventListenersFunctions( Rml::EventListenerInstancer *instancer );

	class UI_ScriptDocument : public Rml::ElementDocument
	{
		private:
			int numScriptsAdded;
			ASInterface *as;
			asIScriptModule *module;
			bool isLoading;
			unsigned numScripts;

			// TODO: proper PostponedEvent that handles reference counting and event instancing!

			// mechanism that calls onload events after all of AS scripts are built
			typedef std::list<Rml::Event*> PostponedList;
			PostponedList onloads;

			Rml::ScriptObject owner;

		public:
			UI_ScriptDocument( const Rml::String &tag = "body" );
			virtual ~UI_ScriptDocument( void );

			asIScriptModule *GetModule( void ) const;

			virtual void LoadScript( Rml::Stream *stream, const Rml::String &source_name );

			virtual void ProcessEvent( Rml::Event& event );

			virtual Rml::ScriptObject GetScriptObject( void ) const;

			bool IsLoading( void ) const { return isLoading; }
	};
}

#endif
