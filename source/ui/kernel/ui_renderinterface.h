/*
 * ui_renderinterface.h
 *
 *  Created on: 25.6.2011
 *      Author: hc
 *
 * Implements the RenderInterface for libRocket
 */
#pragma once
#ifndef UI_RENDERINTERFACE_H_
#define UI_RENDERINTERFACE_H_

#include <RmlUi/Core/RenderInterface.h>
#include "kernel/ui_polyallocator.h"

namespace WSWUI
{

class UI_RenderInterface : public Rml::RenderInterface
{
public:
	UI_RenderInterface( int vidWidth, int vidHeight, float pixelRatio );
	virtual ~UI_RenderInterface();

	//// Implement the RenderInterface

	/// Called by Rocket when it wants to render geometry that it does not wish to optimise.
	virtual void RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture, const Rml::Vector2f& translation);

	/// Called by Rocket when it wants to compile geometry it believes will be static for the forseeable future.
	virtual Rml::CompiledGeometryHandle CompileGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture);

	/// Called by Rocket when it wants to render application-compiled geometry.
	virtual void RenderCompiledGeometry(Rml::CompiledGeometryHandle geometry, const Rml::Vector2f& translation);
	/// Called by Rocket when it wants to release application-compiled geometry.
	virtual void ReleaseCompiledGeometry(Rml::CompiledGeometryHandle geometry);

	/// Called by Rocket when it wants to enable or disable scissoring to clip content.
	virtual void EnableScissorRegion(bool enable);
	/// Called by Rocket when it wants to change the scissor region.
	virtual void SetScissorRegion(int x, int y, int width, int height);


	virtual Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source);
	virtual Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions);
	virtual void ReleaseTexture(Rml::TextureHandle texture);


	///// Called by Rocket when a texture is required by the library.
	//virtual bool LoadTexture(Rml::TextureHandle& texture_handle, Rml::Core::Vector2i& texture_dimensions, const Rml::String& source);
	///// Called by Rocket when a texture is required to be built from an internally-generated sequence of pixels.
	//virtual bool GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::Core::byte* source, const Rml::Core::Vector2i& source_dimensions, int source_samples);
	///// Called by Rocket when a loaded texture is no longer required.
	//virtual void ReleaseTexture(Rml::TextureHandle texture_handle);

	/// Returns the number of pixels per inch.
	virtual float GetPixelsPerInch(void);
	/// Returns the number of pixels per inch when 1dp equals to 1px.
	virtual float GetBasePixelsPerInch(void);

	//// Methods
	int GetWidth( void );
	int GetHeight( void );

	void AddShaderToCache( const Rml::String &shader );
	void ClearShaderCache( void );
	void TouchAllCachedShaders( void );

private:
	int vid_width;
	int vid_height;

	float pixelsPerInch;

	int texCounter;

	bool scissorEnabled;
	int scissorX, scissorY;
	int scissorWidth, scissorHeight;

	PolyAllocator polyAlloc;
	struct shader_s *whiteShader;

	typedef std::map<Rml::String, char> ShaderMap;
	ShaderMap shaderMap;

	poly_t *RocketGeometry2Poly(bool temp, Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture);
};

}

#endif /* UI_RENDERINTERFACE_H_ */
