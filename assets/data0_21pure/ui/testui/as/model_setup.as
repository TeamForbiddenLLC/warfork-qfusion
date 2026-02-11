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

const String DEFAULT_MODEL_SKIN = 'default';

/*
	This class provides to manage the Warsow's Player Setup
	and the Team Setup section.
 */
class ModelSetup
{
	// cvar
	Cvar cModel;
	Cvar cSkin;
	Cvar cColor;
	
	// elements id
	String modelViewId;
	String skinId;
	String colorId;
	
	// model
	ModelView mView;
	int oldModel;		
	int currentModel;		
	int numberOfModels;
	
	// if true, cvars are set in realtime
	bool realtime;
	
	ModelSetup( String modelViewId, String skinId, String colorId,
	            String modelCvar, String skinCvar, String colorCvar,
				bool realtime )
	{
		this.modelViewId = modelViewId;
		this.skinId = skinId;
		this.colorId = colorId;
		
		Cvar cM( modelCvar, "bigvic", 0 );
		Cvar cS( skinCvar, "default", 0 );
		Cvar cC( colorCvar, "255 255 255", 0 );
		
		this.cModel = cM;
		this.cSkin  = cS;
		this.cColor = cC;
		
		this.realtime = realtime;
	}
	
	~ModelSetup( void )
	{
	}
	
	// this must be called after constructor
	void InitializeModelSetup( Element @elem )
	{
		String model = cModel.string;
		
		// model
		DataSource @data = getDataSource( 'models' );
		numberOfModels = data.numRows( 'list' );

		// find the current model in the data source
		for( int i = 0; i < numberOfModels; i++ )
		{
			if( model == data.getField( 'list', i, 'name' ) )
			{
				oldModel = currentModel = i;
				break;
			}
		}
		
		// model view decorator
		ModelView m( modelViewId );
		m.Initialize( @elem , model, /*cSkin.string*/DEFAULT_MODEL_SKIN ); 
		m.SetYRotationSpeed( @elem, '220' );
		m.SetScale( @elem, '0.9' );
		m.SetFovX( @elem, 'auto' );
		m.SetFovY( @elem, '30' );
		mView = m;
		
		// reset elements and model view
		Reset( @elem );	

		SetColor( @elem );
	}
	
	String getCurrentModel( void )
	{
		DataSource @data = getDataSource( 'models' );
		return data.getField( 'list', currentModel, 'name' );		
	}		
	
	String getOldModel( void )
	{
		DataSource @data = getDataSource( 'models' );
		return data.getField( 'list', oldModel, 'name' );	
	}
	
	void UpdateModel( Element @elem )
	{
		String model = getCurrentModel();
		mView.SetModel( elem, model );
	}	

	//===========================
	// called by rocket elements
	//===========================		
	void SetSkin( Element @elem )
	{
		Element @skinElement = elem.getElementById( skinId );
		if( @skinElement == null )
			return;
			
		String skin = DEFAULT_MODEL_SKIN;
		
		if( skinElement.hasAttr( 'checked' ) )
			skin = 'fullbright';
		
		mView.SetSkin( @elem, skin );		
		
		if( realtime ) 
			cSkin.set( skin );
	}
	
	void SetColor( Element @elem )
	{
		ElementFormControl @colorElement = elem.getElementById( colorId );
		if( @colorElement == null )
			return;
			
		String color = colorElement.value;

		String colorHex = "#", outlineHex = "#";
		array<String @> @components = StringUtils::Split( color, " " );
		uint count = 0;
		uint components_size = components.size();
		for( uint i = 0; i < components_size; i++ ) {
			int component = components[i].toInt();
			colorHex += StringUtils::FormatInt( component, '0h', 2 );
			outlineHex += StringUtils::FormatInt( int( float( component ) * 0.25f ), '0h', 2 );
			count++;
			if( count == 3 ) {
				break;
			}
		}
		mView.SetShaderColor( @elem, colorHex );
		mView.SetOutlineColor( @elem, outlineHex );
		
		if( realtime )
			cColor.set( color );
	}
	
	void SelectPrevModel( Element @elem )
	{
		if( currentModel == 0 )
			currentModel = numberOfModels-1;
		else
			currentModel--;
		
		UpdateModel( @elem );
		
		if( realtime )
		{
			String model = getCurrentModel();
			cModel.set( model );
			oldModel = currentModel;
		}
	}		
	
	void SelectNextModel( Element @elem )
	{
		currentModel++;
		currentModel %= numberOfModels;
		
		UpdateModel( @elem );
		
		if( realtime )
		{
			String model = getCurrentModel();
			cModel.set( model );
			oldModel = currentModel;		
		}		
	}
	
	//===========================
	// called on submit
	//===========================	

	// Reset elements and model view to the old values
	void Reset( Element @elem )
	{
		// ===== model =====
		String oldM = getOldModel();
		mView.SetModel( @elem, oldM );
		currentModel = oldModel;
		
		
		// ===== skin =====
		Element @skinElement = elem.getElementById( skinId );
		if( @skinElement == null )
			return;

		String oldSkin = cSkin.string;

		// koochi: libRocket accept 2 checked for 1 checkbox
		if( skinElement.hasAttr( 'checked' ) )
			skinElement.removeAttr( 'checked' );		
		
		if( oldSkin == 'fullbright' )
			skinElement.setAttr( 'checked', '1' );
		
		mView.SetSkin( @elem, oldSkin );	
		
		// ====== color ======
		mView.SetShaderColor( @elem, RGB2Hex( cColor.string ) );
	}
	
	void Fix( Element @elem )
	{
		// ===== model =====
		String model = getCurrentModel();
		cModel.set( model );
		oldModel = currentModel;
		
		
		// ===== skin =====
		Element @skinElement = elem.getElementById( skinId );
		if( @skinElement == null )
			return;
		
		String skin = DEFAULT_MODEL_SKIN;
		
		if( skinElement.hasAttr( 'checked' ) )
			skin = 'fullbright';
		
		cSkin.set( skin );
	}
}