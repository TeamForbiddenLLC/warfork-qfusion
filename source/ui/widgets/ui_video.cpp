#include "ui_precompiled.h"
#include "kernel/ui_common.h"
#include "widgets/ui_video.h"
#include "widgets/ui_widgets.h"

namespace WSWUI
{	
	Video::Video(const Rml::String& tag) : ElementImage(tag)
	{

	} 

	void Video::OnAttributeChange(const Rml::Core::AttributeNameList& anl)
	{
		if(anl.find("src") != anl.end())
			R_RegisterVideo( GetAttribute<Rml::String>("src", "").CString() ); // register a default video-shader, so R_RegisterPic will return this shader
		ElementImage::OnAttributeChange(anl);
	}

	Rml::ElementInstancer *GetVideoInstancer(void)
	{
		return __new__( GenericElementInstancer<Video> )(); 
	}

}
