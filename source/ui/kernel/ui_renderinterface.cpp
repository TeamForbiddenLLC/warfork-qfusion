/*
 * UI_RenderInterface.cpp
 *
 *  Created on: 25.6.2011
 *      Author: hc
 */

#include "ui_precompiled.h"
#include "kernel/ui_common.h"
#include "../cgame/ref.h"
#include "kernel/ui_renderinterface.h"

namespace WSWUI
{

// shortcuts
typedef Rml::TextureHandle TextureHandle;
typedef Rml::Vertex Vertex;
typedef Rml::Vector2f Vector2f;
typedef Rml::Core::Colourb Colourb;
typedef Rml::Core::CompiledGeometryHandle CompiledGeometryHandle;

typedef struct shader_s shader_t;

UI_RenderInterface::UI_RenderInterface( int vidWidth, int vidHeight, float pixelRatio )
	: vid_width( vidWidth ), vid_height( vidHeight ), polyAlloc()
{
	pixelsPerInch = 160.0f * pixelRatio;

	texCounter = 0;

	scissorEnabled = false;
	scissorX = 0;
	scissorY = 0;
	scissorWidth = vid_width;
	scissorHeight = vid_height;

	whiteShader = R_RegisterPic( "$whiteimage" );
}

UI_RenderInterface::~UI_RenderInterface()
{
	this->RemoveReference();
}

Rml::Core::CompiledGeometryHandle UI_RenderInterface::CompileGeometry(Rml::Vertex *vertices, int num_vertices, int *indices, int num_indices, Rml::TextureHandle texture)
{
	poly_t *poly;

	poly = RocketGeometry2Poly( false, vertices, num_vertices, indices, num_indices, texture );

	return Rml::Core::CompiledGeometryHandle( poly );
}

void UI_RenderInterface::ReleaseCompiledGeometry(Rml::Core::CompiledGeometryHandle geometry)
{
	if( geometry == 0 ) {
		return;
	}

	poly_t *poly = ( poly_t * )geometry;
	polyAlloc.free( poly );
}

void UI_RenderInterface::RenderCompiledGeometry(Rml::Core::CompiledGeometryHandle geometry, const Rml::Vector2f & translation)
{
	if( geometry == 0 ) {
		return;
	}

	poly_t *poly = ( poly_t * )geometry;

	RF_DrawStretchPoly(poly, translation.x, translation.y);
}

void UI_RenderInterface::RenderGeometry(Rml::Vertex *vertices, int num_vertices, int *indices, int num_indices, Rml::TextureHandle texture, const Rml::Vector2f & translation)
{
	poly_t *poly;

	poly = RocketGeometry2Poly( true, vertices, num_vertices, indices, num_indices, texture );

	RF_DrawStretchPoly( poly, translation.x, translation.y );
}

void UI_RenderInterface::SetScissorRegion(int x, int y, int width, int height)
{
	scissorX = x;
	scissorY = y;
	scissorWidth = width;
	scissorHeight = height;

	if( scissorEnabled )
		RF_SetScissor( x, y, width, height );
}

void UI_RenderInterface::EnableScissorRegion(bool enable)
{
	if( enable )
		RF_SetScissor( scissorX, scissorY, scissorWidth, scissorHeight );
	else
		RF_ResetScissor();

	scissorEnabled = enable;
}

void UI_RenderInterface::ReleaseTexture(Rml::TextureHandle texture) {

} 


Rml::TextureHandle UI_RenderInterface::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions)
{
	shader_t *shader;
	Rml::String name( MAX_QPATH, "ui_raw_%d", texCounter++ );

	// Com_Printf("RenderInterface::GenerateTexture: going to register %s %dx%d\n", name.CString(), source_dimensions.x, source_dimensions.y );
	shader = R_RegisterRawPic( name.CString(), source_dimensions.x, source_dimensions.y, (uint8_t*)source, source_samples );
	if( !shader )
	{
		Com_Printf(S_COLOR_RED"Warning: RenderInterface couldnt register raw pic %s!\n", name.CString() );
		return NULL;
	}

	// Com_Printf( "RenderInterface::GenerateTexture %s successful\n", name.CString() );

	AddShaderToCache( name );

	return TextureHandle( shader );
}


virtual Rml::TextureHandle UI_RenderInterface::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) {
	shader_t *shader = NULL;
	Rml::String source2( source );

	if( source2[0] == '/' ) {
		source2.Erase( 0, 1 );
	}
	else if( source2[0] == '?' ) {
		String protocol = source2.Substring( 1, source2.Find( "::" ) - 1 );
		if( protocol == "fonthandle" ) {
			if( sscanf( source2.CString(), "?fonthandle::%p", &shader ) != 1 ) {
				Com_Printf( S_COLOR_RED "Warning: RenderInterface couldnt load pic %s!\n", source.CString() );
				return NULL;
			}
		}
	}

	if( !shader ) {
		shader = R_RegisterPic( source2.CString() );
	}

	if( !shader )
	{
		Com_Printf( S_COLOR_RED "Warning: RenderInterface couldnt load pic %s!\n", source.CString() );
		return false;
	}

	R_GetShaderDimensions( shader, &texture_dimensions.x, &texture_dimensions.y );

	if( source2[0] != '?' ) {
		AddShaderToCache( source2 );
	}

	// Com_Printf( "RenderInterface::LoadTexture %s successful (dimensions %dx%d\n", source.CString(), texture_dimensions.x, texture_dimensions.y );
	return TextureHandle( shader );
} 

int UI_RenderInterface::GetHeight( void )
{
	return this->vid_height;
}

int UI_RenderInterface::GetWidth( void )
{
	return this->vid_width;
}

float UI_RenderInterface::GetPixelsPerInch( void )
{
	return this->pixelsPerInch;
}

float UI_RenderInterface::GetBasePixelsPerInch( void )
{
	return 160.0f;
}

poly_t *UI_RenderInterface::RocketGeometry2Poly( bool temp, Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture )
{
	poly_t *poly;
	int i;

	if( temp ) {
		poly = polyAlloc.get_temp( num_vertices, num_indices );
	}
	else {
		poly = polyAlloc.alloc( num_vertices, num_indices );
	}

	// copy stuff over
	for( i = 0; i < num_vertices; i++ )
	{
		poly->verts[i][0] = vertices[i].position.x;
		poly->verts[i][1] = vertices[i].position.y;
		poly->verts[i][2] = 1; // ??
		poly->verts[i][3] = 1;

		poly->normals[i][0] = 0;
		poly->normals[i][1] = 0;
		poly->normals[i][2] = 1; // ??
		poly->normals[i][3] = 0;

		poly->stcoords[i][0] = vertices[i].tex_coord.x;
		poly->stcoords[i][1] = vertices[i].tex_coord.y;

		poly->colors[i][0] = vertices[i].colour.red;
		poly->colors[i][1] = vertices[i].colour.green;
		poly->colors[i][2] = vertices[i].colour.blue;
		poly->colors[i][3] = vertices[i].colour.alpha;
	}

	for( i = 0; i < num_indices; i++ )
		poly->elems[i] = indices[i];

	poly->shader = ( texture == 0 ? whiteShader : ( shader_t* )texture );

	return poly;
}

void UI_RenderInterface::AddShaderToCache( const Rml::String &shader )
{
	ShaderMap::const_iterator it;

	it = shaderMap.find( shader );
	if( it == shaderMap.end() ) {
		shaderMap[shader] = 1;
	}
}

void UI_RenderInterface::ClearShaderCache( void )
{
	shaderMap.clear();
}

void UI_RenderInterface::TouchAllCachedShaders( void )
{
	ShaderMap::const_iterator it;

	for( it = shaderMap.begin(); it != shaderMap.end(); ++it ) {
		R_RegisterPic( it->first.CString() );
	}
}

}
