/*
Copyright (C) 2011 Cervesato Andrea ("koochi")

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

/* 
	This class provide to manage the model view properties.
 */
String basePath = 'models/players/';
String modelFile = 'tris.iqm';
String skinExt  = '.skin';

funcdef bool MVIEW_ANIM_CALLBACK(any &input);

class ModelView
{
	String modelId;
	String modelType;
	String modelSkin;
	
	/* These are css settings for modelview */
	/* 
	String modelPath;
	String skinPath;
	int fov;
	int scale;
	int outlineHeight;
	int outlineColor;
	String shaderColor;
	int xRotation;
	int yRotation;
	int zRotation;
	int xRotationSpeed;
	int yRotationSpeed;
	int zRotationSpeed;
	*/
	
	ModelView( String modelId )
	{
		this.modelId = modelId;
	}
	
	void Initialize( Element @elem, const String &modelType, const String &skin  )
	{
		this.modelType = modelType;
		this.modelSkin = skin;
		
		// initialize model and skin
		SetModel( @elem, modelType );
		SetSkin( @elem, skin );
	}
	
	Element@ GetModel( Element @elem )
	{
		return elem.getElementById( modelId );
	}
	
	// refresh the view
	void Refresh( Element @elem )
	{
		SetModel( @elem, this.modelType );
		SetSkin( @elem, this.modelSkin );
	}
	
	// Set the model
	void SetModel( Element @elem, const String &modelType )
	{
		Element @model = GetModel( @elem );
		
		if( @model != null && modelType.length() != 0  )
		{
			// save model type
			this.modelType = modelType;
			
			String modelPath = basePath + this.modelType + '/' + modelFile;
			
			model.setProp( 'model-scale', "0.0" );
			MVIEW_ANIM_CALLBACK @animate = MVIEW_ANIM_CALLBACK(this.__AnimModelScale);
			window.setInterval( animate, 10, any(@model) );
			
			// skin looks bad when model changes
			SetSkin( @elem, this.modelSkin );

			if(	!model.setProp( 'model-modelpath', modelPath ) )
				console.warn( "ModelView: modelpath parsing failed\n" );
		}
	}
	
	bool __AnimModelScale(any &obj)
	{
		Element @model;
		obj.retrieve(@model);
		
		float scale = model.getProp( 'model-scale' ).toFloat() + 0.05;
		if( scale >= 1.0 ) {
			model.setProp( 'model-scale', "1.0" );
			return false;
		}
		model.setProp( 'model-scale', "" + scale );
		return true;
	}
	
	// Set the model's skin
	void SetSkin( Element @elem, const String &skin )
	{
		Element @model = GetModel( @elem );
		
		if( @model != null )
		{
			if( skin.empty() ) {
				this.modelSkin = 'default';
			} else {
				// save current skin for future refresh
				this.modelSkin = skin;
			}

			String skinPath = basePath + this.modelType + '/' + this.modelSkin + skinExt;

			if( !model.setProp( 'model-skinpath', skinPath ) )
				console.warn("ModelView: skinpath parsing failed\n");
		}
	}
	
	// Set the horizontal fov to use for the model
	void SetFovX( Element @elem, const String &fov )
	{
		Element @model = GetModel( @elem );
		
		if( @model != null && !fov.empty() )
		{
			model.setProp( 'model-fov-x', fov );
		}
	}
	
	// Set the vertical fov to use for the model
	void SetFovY( Element @elem, const String &fov )
	{
		Element @model = GetModel( @elem );
		
		if( @model != null && !fov.empty() )
		{
			model.setProp( 'model-fov-y', fov );
		}
	}
	
	// Set the model's scaling factor
	void SetScale( Element @elem, const String &scale )
	{
		Element @model = GetModel( @elem );
		
		if( @model != null && !scale.empty() )
		{
			model.setProp( 'model-scale', scale );
		}
	}
	
	// Set the height of the model's outlines
	void SetOutlineHeight( Element @elem, const String &height )
	{
		Element @model = GetModel( @elem );
		
		if( @model != null && !height.empty() )
		{
			model.setProp( 'model-outline-height', height );
		}
	}
	
	// color in hex form: #f0f0f0f0
	void SetOutlineColor( Element @elem, const String &color )
	{
		Element @model = GetModel( @elem );
		
		if( @model != null && !color.empty() )
		{
			model.setProp( 'model-outline-color', color );
		}
	}		
	
	// color in hex form: #f0f0f0f0
	void SetShaderColor( Element @elem, const String &color )
	{
		Element @model = GetModel( @elem );
		
		if( @model != null && !color.empty() )
		{
			model.setProp( 'model-shader-color', color );
		}
	}		
	
	// model's x axis rotation
	void XRotation( Element @elem, const String &rotation ) 
	{
		Element @model = GetModel( @elem );
		
		if( @model != null && !rotation.empty() )
		{
			model.setProp( 'model-rotation-pitch', rotation );
		}
	}

	// model's y axis rotation
	void YRotation( Element @elem, const String &rotation )
	{
		Element @model = GetModel( @elem );
		
		if( @model != null && !rotation.empty() )
		{
			model.setProp( 'model-rotation-yaw', rotation );
		}
	}

	// model's z axis rotation
	void ZRotation( Element @elem, const String &rotation )
	{
		Element @model = GetModel( @elem );
		
		if( @model != null && !rotation.empty() )
		{
			model.setProp( 'model-rotation-roll', rotation );
		}
	}
	
	// set the model's x axis rotation speed
	void SetXRotationSpeed( Element @elem, const String &speed ) 
	{
		Element @model = GetModel( @elem );
		
		if( @model != null && !speed.empty() )
		{
			model.setProp( 'model-rotation-speed-pitch', speed );
		}
	}
	
	// set the model's y axis rotation speed
	void SetYRotationSpeed( Element @elem, const String &speed ) 
	{
		Element @model = GetModel( @elem );
		
		if( @model != null && !speed.empty() )
		{
			model.setProp( 'model-rotation-speed-yaw', speed );
		}
	}	
	
	// set the model's z axis rotation speed
	void SetZRotationSpeed( Element @elem, const String &speed ) 
	{
		Element @model = GetModel( @elem );
		
		if( @model != null && !speed.empty() )
		{
			model.setProp( 'model-rotation-speed-roll', speed );
		}
	}		
}