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
	This class provides to manage some cvars into the 
	Graphics section.
 */

class VideoSetup
{
	// use to recognize changes and inhibit vid_restart cmd
	bool allowVidRestart;

	// whether video mode-related options should be shown
	bool showVideoFrame;
	
	// video profile
	Cvar ui_video_profile( "ui_video_profile", "medium", CVAR_ARCHIVE );
	
	// video mode
	Cvar vid_width( "vid_width", "0", 0 );
	Cvar vid_height( "vid_height", "0", 0 );

	// vertical sync
	Cvar r_swapinterval_min( "r_swapinterval_min", "0", 0 );
	
	// multithreading
	Cvar r_multithreading( "r_multithreading", "1", 0 );
	
	// renderer maxfps
	Cvar r_maxfps( "r_maxfps", "250", 0 );
	
	// quality of textures
	Cvar r_picmip( "r_picmip", "0", 0 );
	
	// filtering
	Cvar r_texturefilter( "r_texturefilter", "4", 0 ); // 0==1, 2, 4, .. values
	Cvar r_texturefilter_max( "r_texturefilter_max", "0", 0 );		
	Cvar r_soft_particles_available( "r_soft_particles_available", "0", 0 );		
	
	// lighting
	Cvar r_lighting_vertexlight( "r_lighting_vertexlight", "0", 0 );
	Cvar r_lighting_deluxemapping( "r_lighting_deluxemapping", "1", 0 );
	Cvar ui_lighting( "ui_lighting", "2", 0 ); // used to store index of selector
	
	// shadows
	Cvar cg_shadows( "cg_shadows", "1", CVAR_ARCHIVE );
	
	// ids
	String idProfile;
	String idVideoFrame;
	String idMode;
	String idModeFrame;
	String idFullscreenFrame;
	String idVsyncFrame;
	String idRMaxFpsFrame;
	String idClMaxFpsFrame;
	String idGammaFrame;
	String idPicmip;
	String idPicmipFrame;
	String idFiltering;
	String idFilteringFrame;
	String idLighting;
	String idSoftParticlesFrame;

	VideoSetup( Element @elem, 
				const String &idProfile,
				const String &idVideoFrame,
				const String &idMode, const String &idModeFrame,
				const String &idFullscreenFrame, const String &idVsyncFrame,
				const String &idRMaxFpsFrame, const String &idClMaxFpsFrame,
				const String &idGammaFrame,
				const String &idPicmip, const String &idPicmipFrame,
				const String &idFiltering,
				const String &idFilteringFrame,
				const String &idLighting, 
				const String &idSoftParticlesFrame )
	{
		this.idProfile = idProfile;
		this.idVideoFrame = idVideoFrame;
		this.idMode = idMode;
		this.idModeFrame = idModeFrame;
		this.idFullscreenFrame = idFullscreenFrame;
		this.idVsyncFrame = idVsyncFrame;
		this.idRMaxFpsFrame = idRMaxFpsFrame;
		this.idClMaxFpsFrame = idClMaxFpsFrame;
		this.idGammaFrame = idGammaFrame;
		this.idPicmip = idPicmip;
		this.idPicmipFrame = idPicmipFrame;
		this.idFiltering = idFiltering;
		this.idFilteringFrame = idFilteringFrame;
		this.idLighting = idLighting;
		this.idSoftParticlesFrame = idSoftParticlesFrame;

		// We only have 3 choices in lightning listbox:
		// vertex lighting: lighting_vertexlight = 1, lighting_deluxemapping = 0
		// lightmaps: lighting_vertexlight = 0, lighting_deluxemapping = 0
		// per-pixel lighting: lighting_vertexlight = 0, lighting_deluxemapping = 1
		// since the variant lighting_vertexlight = 1, lighting_deluxemapping = 1 is
		// equivalent to vertex lighting.
		if( r_lighting_vertexlight.value == 1 )
			ui_lighting.set( 0 );
		else if( r_lighting_deluxemapping.value == 1 )
			ui_lighting.set( 2 );
		else
			ui_lighting.set( 1 );

		Initialize( @elem );
	}
	
	~VideoSetup()
	{
	}
	
	void Initialize( Element @elem )
	{
		showVideoFrame = false;

		PopulateModeSelector( @elem );
		PopulateFilteringSelector( @elem );
		CheckFullscreenAvailability( @elem );
		CheckVsyncAvailability( @elem );
		CheckMultiThreadingAvailability( @elem );
		CheckGammaAvailability( @elem );
		CheckSoftParticlesAvailability( @elem );

		if( !showVideoFrame ) // all video options are hidden, so hide the separator as well
		{
			Element @frame = elem.getElementById( idVideoFrame );
			if( @frame != null )
				frame.css( 'display', 'none' );
		}

		// reset elements
		SelectGraphicsProfile( @elem, true );
		Reset( @elem );
	}

	void PopulateModeSelector( Element @elem )
	{
		Element @selector = elem.getElementById( idMode );
		if( @selector == null )
			return;

		DataSource @data = getDataSource( 'video' );
		int numberOfModes = data.numRows( 'list' );
		String selected = vid_width.string + ' x ' + vid_height.string;
		String mode;

		String rml = '';
		for( int i = 0; i < numberOfModes; i++ )
		{
			mode = data.getField( 'list', i, 'resolution' );
			rml += '<option value="' + mode + '"' + ( ( mode == selected ) ? ' selected' : '' ) + '>' + mode + '</option>';
		}
		selector.setInnerRML( rml );

		if( numberOfModes <= 1 )
		{
			Element @frame = elem.getElementById( idModeFrame );
			if( @frame != null )
				frame.css( 'display', 'none' );
		}
		else
		{
			showVideoFrame = true;
		}
	}

	void PopulateFilteringSelector( Element @elem )
	{
		Element @selector = elem.getElementById( idFiltering );
		
		if( @selector == null )
			return;
			
		int max = r_texturefilter_max.integer;

		String rml = '<option value="1">off</option>';
		for( int i = 2; i <= max; i*=2 )
			rml += '<option value="'+i+'">'+i+'x</option>';
			
		selector.setInnerRML( rml );

		if( max <= 1 )
		{
			Element @frame = elem.getElementById( idFilteringFrame );
			if( @frame != null )
				frame.css( 'display', 'none' );
		}
	}

	void CheckFullscreenAvailability( Element @elem )
	{
			showVideoFrame = true;
	}

	void CheckVsyncAvailability( Element @elem )
	{
		if( r_swapinterval_min.integer != 0 )
		{
			Element @frame = elem.getElementById( idVsyncFrame );
			if ( @frame != null )
				frame.css( 'display', 'none' );
		}
		else
		{
			showVideoFrame = true;
		}
	}
	
	void CheckMultiThreadingAvailability( Element @elem )
	{
		Element @frame;

		if( r_swapinterval_min.integer != 0 )
		{
			@frame = elem.getElementById( idClMaxFpsFrame );
			if ( @frame != null )
				frame.css( 'display', 'none' );

			@frame = elem.getElementById( idRMaxFpsFrame );
			if ( @frame != null )
				frame.css( 'display', 'none' );
		}
		else
		{
			showVideoFrame = true;

			if( r_multithreading.integer != 0 )
			{
				@frame = elem.getElementById( idClMaxFpsFrame );
				if ( @frame != null )
					frame.css( 'display', 'none' );
			}
			else
			{
				@frame = elem.getElementById( idRMaxFpsFrame );
				if ( @frame != null )
					frame.css( 'display', 'none' );
			}
		}
	}
	
	void CheckGammaAvailability( Element @elem )
	{
			showVideoFrame = true;
	}

	void CheckSoftParticlesAvailability( Element @elem )
	{
		if( r_soft_particles_available.integer == 0 )
		{
			Element @frame = elem.getElementById( idSoftParticlesFrame );
			if ( @frame != null )
				frame.css( 'display', 'none' );
		}
	}

	void SelectGraphicsProfile( Element @elem, bool reset )
	{
		ElementFormControl @profile = elem.getElementById( idProfile );
		
		if( @profile == null )
			return;
		
		String gfx;
		
		if( reset )
		{
			gfx = ui_video_profile.string;
			profile.value = gfx;
		}
		else
		{
			gfx = profile.value;
			game.execAppend( "exec profiles/" + gfx + "\n" );
			ui_video_profile.set( gfx );

			allowVidRestart = false;
		}
	}

	void SetMode( Element @elem, bool reset )
	{
		ElementFormControl @mode = elem.getElementById( idMode );
		if( @mode == null )
			return;

		if( reset )
		{
			mode.value = vid_width.string + ' x ' + vid_height.string;
		}
		else
		{
			array<String @> resolution = StringUtils::Split( mode.value, ' x ' );
			if( resolution.size() < 2 )
				return;

			vid_width.set( resolution[0] );
			vid_height.set( resolution[1] );
			Changed();
		}
	}
	
	void SetPicmip( Element @elem, bool reset )
	{
		Element @picmip_el = elem.getElementById( idPicmip );	
		ElementFormControl @picmip = @picmip_el;
		Element @picmip_frame = elem.getElementById( idPicmipFrame );

		if( @picmip == null )
			return;

		int maxvalue = picmip_el.getAttr( "max", "0" ).toInt();

		if( reset )
		{
			if( @picmip_frame != null )
				picmip_frame.css( 'display', ( r_picmip.integer <= maxvalue ) ? 'block' : 'none' );
			picmip.value = String( maxvalue - r_picmip.integer );
		}
		else
		{
			r_picmip.set( maxvalue - picmip.value.toInt() );
			Changed();
		}
	}
	
	void SetFiltering( Element @elem, bool reset )
	{
		ElementFormControl @filter = elem.getElementById( idFiltering );
		
		if( @filter == null )
			return;
	
		if( reset )
		{
			int value = r_texturefilter.integer;
			int max = r_texturefilter_max.integer;
			if( value > max )
				value = max;
			if( value < 1 )
				value = 1;
			filter.value = String( value );
		}
		else
		{
			r_texturefilter.set( filter.value.toInt() );
			Changed();		
		}		
	}

	void SetLighting( Element @elem, bool reset )
	{
		ElementFormControl @slideLighting = elem.getElementById( idLighting );

		if( @slideLighting == null )
			return;

		if( reset )
		{
			slideLighting.value = ui_lighting.string;
		}
		else
		{
			int value = slideLighting.value.toInt();
			
			switch( value )
			{
				case 0:
					r_lighting_vertexlight.set("1");
					r_lighting_deluxemapping.set("0");
					break;
				case 1:
					r_lighting_vertexlight.set("0");
					r_lighting_deluxemapping.set("0");
					break;
				default: // 2
					r_lighting_vertexlight.set("0");
					r_lighting_deluxemapping.set("1");
					break;
			}
			
			ui_lighting.set( value );
			Changed();
		}
	}

	void Changed( void )
	{
		allowVidRestart = true;
	}

	void Reset( Element @elem )
	{
		SetMode( @elem, true );
		SetPicmip( @elem, true );
		SetFiltering( @elem, true );
		SetLighting( @elem, true );
		
		// cvars are not changed
		allowVidRestart = false;
	}
	
	void Apply( Element @elem )
	{
		// apply changes if something changed
		if( allowVidRestart )
		{
			SetMode( @elem, false );
			SetPicmip( @elem, false );
			SetFiltering( @elem, false );
			SetLighting( @elem, false );
			
			game.execAppend ( "vid_restart\n" );
		}
	}
}
