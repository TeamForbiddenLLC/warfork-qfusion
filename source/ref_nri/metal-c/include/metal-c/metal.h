// SPDX-License-Identifier: GPL-2.0-or-later
//
// Metal (MTL) bindings -- the curated subset the RHI backend needs. Each type is
// a typed view over an objc_object*; every function forwards to objc_msgSend
// with the selector named in the matching metal-cpp header. Extend on demand.
//
// Port of rhi-zig/deps/metal/src/metal.zig.
//
// Ownership follows Cocoa (see MTLC_HANDLE in types.h): anything from a
// *_new_* function or a descriptor _init is caller-owned -- release it. Getters
// return autoreleased objects you do not own.

#ifndef MTLC_METAL_H
#define MTLC_METAL_H

#include "metal-c/foundation.h"
#include "metal-c/types.h"

// -- Handles ----------------------------------------------------------------
//
// Declared up front because they are passed and returned by value, so every
// signature below needs the complete type.

MTLC_HANDLE(mtlc_device);
MTLC_HANDLE(mtlc_command_queue);
MTLC_HANDLE(mtlc_command_buffer);
MTLC_HANDLE(mtlc_render_command_encoder);
MTLC_HANDLE(mtlc_buffer);
MTLC_HANDLE(mtlc_texture);
MTLC_HANDLE(mtlc_library);
MTLC_HANDLE(mtlc_function);
MTLC_HANDLE(mtlc_render_pipeline_state);
MTLC_HANDLE(mtlc_depth_stencil_state);
MTLC_HANDLE(mtlc_sampler_state);
MTLC_HANDLE(mtlc_compile_options);
MTLC_HANDLE(mtlc_render_pipeline_descriptor);
MTLC_HANDLE(mtlc_vertex_descriptor);
MTLC_HANDLE(mtlc_vertex_attribute_descriptor_array);
MTLC_HANDLE(mtlc_vertex_attribute_descriptor);
MTLC_HANDLE(mtlc_vertex_buffer_layout_descriptor_array);
MTLC_HANDLE(mtlc_vertex_buffer_layout_descriptor);
MTLC_HANDLE(mtlc_render_pipeline_color_attachment_descriptor_array);
MTLC_HANDLE(mtlc_render_pipeline_color_attachment_descriptor);
MTLC_HANDLE(mtlc_render_pass_descriptor);
MTLC_HANDLE(mtlc_render_pass_color_attachment_descriptor_array);
MTLC_HANDLE(mtlc_render_pass_color_attachment_descriptor);
MTLC_HANDLE(mtlc_render_pass_depth_attachment_descriptor);
MTLC_HANDLE(mtlc_texture_descriptor);
MTLC_HANDLE(mtlc_depth_stencil_descriptor);
MTLC_HANDLE(mtlc_sampler_descriptor);
MTLC_HANDLE(mtlc_argument_encoder);

// -- Device -----------------------------------------------------------------

// MTLCreateSystemDefaultDevice() -- the system default GPU, or nil if Metal is
// unavailable (check with mtlc_device_is_nil). Owned by the caller.
MTLC_EXTERN_C struct mtlc_device mtlc_create_system_default_device(void);

// -[MTLDevice name] (autoreleased NSString). Never nil.
MTLC_EXTERN_C struct ns_string mtlc_device_name(struct mtlc_device self);

// -[MTLDevice isLowPower] -- true for integrated GPUs.
MTLC_EXTERN_C bool mtlc_device_is_low_power(struct mtlc_device self);

// -[MTLDevice hasUnifiedMemory] -- true on Apple Silicon and other UMA parts,
// where a Shared buffer is directly GPU-visible and no staging copy is needed.
MTLC_EXTERN_C bool mtlc_device_has_unified_memory(struct mtlc_device self);

// -[MTLDevice argumentBuffersSupport] -- the argument-buffer tier this GPU
// supports. Every Metal-capable GPU is at least Tier1.
MTLC_EXTERN_C enum mtlc_argument_buffers_tier
mtlc_device_argument_buffers_support(struct mtlc_device self);

// -[MTLDevice maxArgumentBufferSamplerCount] -- how many distinct sampler
// states may be reachable from argument buffers bound at once. Typically 16 on
// Tier1, so a descriptor set with more samplers than this cannot use one.
MTLC_EXTERN_C mtlc_uinteger
mtlc_device_max_argument_buffer_sampler_count(struct mtlc_device self);

// -[MTLDevice newCommandQueue]. Owned by the caller.
MTLC_EXTERN_C struct mtlc_command_queue
mtlc_device_new_command_queue(struct mtlc_device self);

// -[MTLDevice newBufferWithLength:options:]. Owned by the caller.
// `options` is an OR of MTLC_RESOURCE_* constants; 0 is Metal's default.
MTLC_EXTERN_C struct mtlc_buffer
mtlc_device_new_buffer(struct mtlc_device self, mtlc_uinteger length,
                       mtlc_uinteger options);

// -[MTLDevice newBufferWithBytes:length:options:] -- initialise from CPU
// memory. Owned by the caller.
MTLC_EXTERN_C struct mtlc_buffer
mtlc_device_new_buffer_with_bytes(struct mtlc_device self, const void *bytes,
                                  mtlc_uinteger length, mtlc_uinteger options);

// -[MTLDevice newTextureWithDescriptor:]. Owned by the caller.
MTLC_EXTERN_C struct mtlc_texture
mtlc_device_new_texture(struct mtlc_device self,
                        struct mtlc_texture_descriptor descriptor);

// -[MTLDevice newDepthStencilStateWithDescriptor:]. Owned by the caller.
MTLC_EXTERN_C struct mtlc_depth_stencil_state
mtlc_device_new_depth_stencil_state(
    struct mtlc_device self, struct mtlc_depth_stencil_descriptor descriptor);

// -[MTLDevice newSamplerStateWithDescriptor:]. Owned by the caller.
MTLC_EXTERN_C struct mtlc_sampler_state
mtlc_device_new_sampler_state(struct mtlc_device self,
                              struct mtlc_sampler_descriptor descriptor);

// -[MTLDevice newLibraryWithSource:options:error:]. Owned by the caller.
//
// `options` may be nil (pass mtlc_compile_options_from_id(NULL)). On failure
// the result is nil and, when `out_err` is non-NULL, *out_err holds the
// autoreleased NSError -- do not release it. *out_err is always written when
// out_err is non-NULL, so it is nil on success.
MTLC_EXTERN_C struct mtlc_library mtlc_device_new_library_with_source(
    struct mtlc_device self, struct ns_string source,
    struct mtlc_compile_options options, struct ns_error *out_err);

// -[MTLDevice newRenderPipelineStateWithDescriptor:error:]. Owned by the
// caller. `out_err` behaves as in mtlc_device_new_library_with_source.
MTLC_EXTERN_C struct mtlc_render_pipeline_state
mtlc_device_new_render_pipeline_state(
    struct mtlc_device self, struct mtlc_render_pipeline_descriptor descriptor,
    struct ns_error *out_err);

// -- Command submission -----------------------------------------------------

// -[MTLCommandQueue commandBuffer] (autoreleased).
MTLC_EXTERN_C struct mtlc_command_buffer
mtlc_command_queue_command_buffer(struct mtlc_command_queue self);

// -[MTLCommandQueue setLabel:].
MTLC_EXTERN_C void mtlc_command_queue_set_label(struct mtlc_command_queue self,
                                                struct ns_string label);

// -[MTLCommandBuffer renderCommandEncoderWithDescriptor:] (autoreleased).
MTLC_EXTERN_C struct mtlc_render_command_encoder
mtlc_command_buffer_render_command_encoder(
    struct mtlc_command_buffer self,
    struct mtlc_render_pass_descriptor descriptor);

// -[MTLCommandBuffer presentDrawable:] -- schedules the drawable for
// presentation. `drawable` is any id conforming to MTLDrawable; for a
// CAMetalDrawable pass ca_metal_drawable_id(d).
MTLC_EXTERN_C void
mtlc_command_buffer_present_drawable(struct mtlc_command_buffer self,
                                     void *drawable);

// -[MTLCommandBuffer commit].
MTLC_EXTERN_C void mtlc_command_buffer_commit(struct mtlc_command_buffer self);

// -[MTLCommandBuffer waitUntilCompleted].
MTLC_EXTERN_C void
mtlc_command_buffer_wait_until_completed(struct mtlc_command_buffer self);

// -[MTLCommandBuffer waitUntilScheduled] -- blocks until the command buffer is
// scheduled by the GPU (a weaker barrier than waitUntilCompleted). Needed for
// the presentsWithTransaction present path: schedule the work, then present the
// drawable on the CPU thread.
MTLC_EXTERN_C void
mtlc_command_buffer_wait_until_scheduled(struct mtlc_command_buffer self);

// -[MTLCommandBuffer addCompletedHandler:] -- fn(ctx) runs on a Metal-internal
// thread when this command buffer finishes on the GPU. Used for frames-in-flight
// pacing (signal a semaphore). fn must be thread-safe.
MTLC_EXTERN_C void mtlc_command_buffer_add_completed_handler(
    struct mtlc_command_buffer self, void ( *fn )( void *ctx ), void *ctx );

// -- Render command encoder -------------------------------------------------

// -[MTLRenderCommandEncoder setRenderPipelineState:].
MTLC_EXTERN_C void mtlc_render_command_encoder_set_render_pipeline_state(
    struct mtlc_render_command_encoder self,
    struct mtlc_render_pipeline_state state);

// -[MTLRenderCommandEncoder setVertexBuffer:offset:atIndex:].
MTLC_EXTERN_C void mtlc_render_command_encoder_set_vertex_buffer(
    struct mtlc_render_command_encoder self, struct mtlc_buffer buffer,
    mtlc_uinteger offset, mtlc_uinteger index);

// -[MTLRenderCommandEncoder setVertexBytes:length:atIndex:] -- inline constant
// data (push constants).
MTLC_EXTERN_C void mtlc_render_command_encoder_set_vertex_bytes(
    struct mtlc_render_command_encoder self, const void *bytes,
    mtlc_uinteger length, mtlc_uinteger index);

// -[MTLRenderCommandEncoder setFragmentBuffer:offset:atIndex:].
MTLC_EXTERN_C void mtlc_render_command_encoder_set_fragment_buffer(
    struct mtlc_render_command_encoder self, struct mtlc_buffer buffer,
    mtlc_uinteger offset, mtlc_uinteger index);

// -[MTLRenderCommandEncoder setFragmentBytes:length:atIndex:].
MTLC_EXTERN_C void mtlc_render_command_encoder_set_fragment_bytes(
    struct mtlc_render_command_encoder self, const void *bytes,
    mtlc_uinteger length, mtlc_uinteger index);

// -[MTLRenderCommandEncoder setVertexTexture:atIndex:].
MTLC_EXTERN_C void mtlc_render_command_encoder_set_vertex_texture(
    struct mtlc_render_command_encoder self, struct mtlc_texture texture,
    mtlc_uinteger index);

// -[MTLRenderCommandEncoder setFragmentTexture:atIndex:].
MTLC_EXTERN_C void mtlc_render_command_encoder_set_fragment_texture(
    struct mtlc_render_command_encoder self, struct mtlc_texture texture,
    mtlc_uinteger index);

// -[MTLRenderCommandEncoder setVertexSamplerState:atIndex:].
MTLC_EXTERN_C void mtlc_render_command_encoder_set_vertex_sampler_state(
    struct mtlc_render_command_encoder self, struct mtlc_sampler_state sampler,
    mtlc_uinteger index);

// -[MTLRenderCommandEncoder setFragmentSamplerState:atIndex:].
MTLC_EXTERN_C void mtlc_render_command_encoder_set_fragment_sampler_state(
    struct mtlc_render_command_encoder self, struct mtlc_sampler_state sampler,
    mtlc_uinteger index);

// -[MTLRenderCommandEncoder setViewport:].
MTLC_EXTERN_C void
mtlc_render_command_encoder_set_viewport(struct mtlc_render_command_encoder self,
                                         struct mtlc_viewport viewport);

// -[MTLRenderCommandEncoder setDepthStencilState:].
MTLC_EXTERN_C void mtlc_render_command_encoder_set_depth_stencil_state(
    struct mtlc_render_command_encoder self,
    struct mtlc_depth_stencil_state state);

// -[MTLRenderCommandEncoder setScissorRect:].
MTLC_EXTERN_C void mtlc_render_command_encoder_set_scissor_rect(
    struct mtlc_render_command_encoder self, struct mtlc_scissor_rect rect);

// -[MTLRenderCommandEncoder
//   drawPrimitives:vertexStart:vertexCount:instanceCount:baseInstance:].
MTLC_EXTERN_C void mtlc_render_command_encoder_draw_primitives(
    struct mtlc_render_command_encoder self,
    enum mtlc_primitive_type primitive_type, mtlc_uinteger vertex_start,
    mtlc_uinteger vertex_count, mtlc_uinteger instance_count,
    mtlc_uinteger base_instance);

// -[MTLRenderCommandEncoder drawIndexedPrimitives:indexCount:indexType:
//   indexBuffer:indexBufferOffset:instanceCount:baseVertex:baseInstance:].
// base_vertex is a signed NSInteger (added to each fetched index).
MTLC_EXTERN_C void mtlc_render_command_encoder_draw_indexed_primitives(
    struct mtlc_render_command_encoder self,
    enum mtlc_primitive_type primitive_type, mtlc_uinteger index_count,
    enum mtlc_index_type index_type, struct mtlc_buffer index_buffer,
    mtlc_uinteger index_buffer_offset, mtlc_uinteger instance_count,
    mtlc_integer base_vertex, mtlc_uinteger base_instance);

// -[MTLRenderCommandEncoder endEncoding].
MTLC_EXTERN_C void
mtlc_render_command_encoder_end_encoding(struct mtlc_render_command_encoder self);

// -- Resources --------------------------------------------------------------

// -[MTLBuffer contents] -- CPU-accessible pointer. NULL for private storage.
MTLC_EXTERN_C void *mtlc_buffer_contents(struct mtlc_buffer self);

// -[MTLBuffer didModifyRange:] -- publish a CPU write to the GPU. Required for
// Managed storage (discrete GPUs); a no-op worth skipping on Shared.
MTLC_EXTERN_C void mtlc_buffer_did_modify_range(struct mtlc_buffer self,
                                                struct ns_range range);

// -[MTLBuffer length].
MTLC_EXTERN_C mtlc_uinteger mtlc_buffer_length(struct mtlc_buffer self);

// -[MTLTexture width].
MTLC_EXTERN_C mtlc_uinteger mtlc_texture_width(struct mtlc_texture self);

// -[MTLTexture height].
MTLC_EXTERN_C mtlc_uinteger mtlc_texture_height(struct mtlc_texture self);

// -[MTLTexture replaceRegion:mipmapLevel:slice:withBytes:bytesPerRow:
//   bytesPerImage:] -- CPU upload into a Shared/Managed texture. bytes_per_image
// may be 0 for 2D.
MTLC_EXTERN_C void mtlc_texture_replace_region(
    struct mtlc_texture self, struct mtlc_region region, mtlc_uinteger mip_level,
    mtlc_uinteger slice, const void *bytes, mtlc_uinteger bytes_per_row,
    mtlc_uinteger bytes_per_image);

// -- Shaders / pipeline state -----------------------------------------------

// -[MTLLibrary newFunctionWithName:]. Owned by the caller; nil if the entry
// point is not in the library.
MTLC_EXTERN_C struct mtlc_function
mtlc_library_new_function(struct mtlc_library self,
                          struct ns_string function_name);

// -- Descriptors ------------------------------------------------------------

// [[MTLCompileOptions alloc] init]. Owned by the caller.
MTLC_EXTERN_C struct mtlc_compile_options mtlc_compile_options_init(void);

// [[MTLRenderPipelineDescriptor alloc] init]. Owned by the caller.
MTLC_EXTERN_C struct mtlc_render_pipeline_descriptor
mtlc_render_pipeline_descriptor_init(void);

MTLC_EXTERN_C void mtlc_render_pipeline_descriptor_set_vertex_function(
    struct mtlc_render_pipeline_descriptor self, struct mtlc_function function);

MTLC_EXTERN_C void mtlc_render_pipeline_descriptor_set_fragment_function(
    struct mtlc_render_pipeline_descriptor self, struct mtlc_function function);

MTLC_EXTERN_C void mtlc_render_pipeline_descriptor_set_label(
    struct mtlc_render_pipeline_descriptor self, struct ns_string label);

MTLC_EXTERN_C void mtlc_render_pipeline_descriptor_set_vertex_descriptor(
    struct mtlc_render_pipeline_descriptor self,
    struct mtlc_vertex_descriptor vertex_descriptor);

MTLC_EXTERN_C void
mtlc_render_pipeline_descriptor_set_depth_attachment_pixel_format(
    struct mtlc_render_pipeline_descriptor self,
    enum mtlc_pixel_format format);

// -[MTLRenderPipelineDescriptor colorAttachments] (autoreleased). Never nil.
MTLC_EXTERN_C struct mtlc_render_pipeline_color_attachment_descriptor_array
mtlc_render_pipeline_descriptor_color_attachments(
    struct mtlc_render_pipeline_descriptor self);

// -- MTLVertexDescriptor ----------------------------------------------------

// +[MTLVertexDescriptor vertexDescriptor] (autoreleased). Never nil.
MTLC_EXTERN_C struct mtlc_vertex_descriptor mtlc_vertex_descriptor_new(void);

MTLC_EXTERN_C struct mtlc_vertex_attribute_descriptor_array
mtlc_vertex_descriptor_attributes(struct mtlc_vertex_descriptor self);

MTLC_EXTERN_C struct mtlc_vertex_buffer_layout_descriptor_array
mtlc_vertex_descriptor_layouts(struct mtlc_vertex_descriptor self);

// -objectAtIndexedSubscript:. Never nil for an in-range index.
MTLC_EXTERN_C struct mtlc_vertex_attribute_descriptor
mtlc_vertex_attribute_descriptor_array_object(
    struct mtlc_vertex_attribute_descriptor_array self, mtlc_uinteger index);

MTLC_EXTERN_C void mtlc_vertex_attribute_descriptor_set_format(
    struct mtlc_vertex_attribute_descriptor self,
    enum mtlc_vertex_format format);

MTLC_EXTERN_C void mtlc_vertex_attribute_descriptor_set_offset(
    struct mtlc_vertex_attribute_descriptor self, mtlc_uinteger offset);

MTLC_EXTERN_C void mtlc_vertex_attribute_descriptor_set_buffer_index(
    struct mtlc_vertex_attribute_descriptor self, mtlc_uinteger index);

// -objectAtIndexedSubscript:. Never nil for an in-range index.
MTLC_EXTERN_C struct mtlc_vertex_buffer_layout_descriptor
mtlc_vertex_buffer_layout_descriptor_array_object(
    struct mtlc_vertex_buffer_layout_descriptor_array self,
    mtlc_uinteger index);

MTLC_EXTERN_C void mtlc_vertex_buffer_layout_descriptor_set_stride(
    struct mtlc_vertex_buffer_layout_descriptor self, mtlc_uinteger stride);

MTLC_EXTERN_C void mtlc_vertex_buffer_layout_descriptor_set_step_function(
    struct mtlc_vertex_buffer_layout_descriptor self,
    enum mtlc_vertex_step_function step);

MTLC_EXTERN_C void mtlc_vertex_buffer_layout_descriptor_set_step_rate(
    struct mtlc_vertex_buffer_layout_descriptor self, mtlc_uinteger rate);

// -- Render pipeline colour attachments -------------------------------------

// -objectAtIndexedSubscript:. Never nil for an in-range index.
MTLC_EXTERN_C struct mtlc_render_pipeline_color_attachment_descriptor
mtlc_render_pipeline_color_attachment_descriptor_array_object(
    struct mtlc_render_pipeline_color_attachment_descriptor_array self,
    mtlc_uinteger index);

MTLC_EXTERN_C void mtlc_render_pipeline_color_attachment_descriptor_set_pixel_format(
    struct mtlc_render_pipeline_color_attachment_descriptor self,
    enum mtlc_pixel_format format);

// -[MTLRenderPipelineColorAttachmentDescriptor setBlendingEnabled:] and the
// per-attachment blend state. Blending is pipeline-static in Metal, so every
// distinct blend setup needs its own MTLRenderPipelineState.
MTLC_EXTERN_C void
mtlc_render_pipeline_color_attachment_descriptor_set_blending_enabled(
    struct mtlc_render_pipeline_color_attachment_descriptor self,
    bool enabled);

MTLC_EXTERN_C void
mtlc_render_pipeline_color_attachment_descriptor_set_rgb_blend_factors(
    struct mtlc_render_pipeline_color_attachment_descriptor self,
    enum mtlc_blend_factor source, enum mtlc_blend_factor destination);

MTLC_EXTERN_C void
mtlc_render_pipeline_color_attachment_descriptor_set_alpha_blend_factors(
    struct mtlc_render_pipeline_color_attachment_descriptor self,
    enum mtlc_blend_factor source, enum mtlc_blend_factor destination);

MTLC_EXTERN_C void
mtlc_render_pipeline_color_attachment_descriptor_set_blend_operations(
    struct mtlc_render_pipeline_color_attachment_descriptor self,
    enum mtlc_blend_operation rgb, enum mtlc_blend_operation alpha);

MTLC_EXTERN_C void
mtlc_render_pipeline_color_attachment_descriptor_set_write_mask(
    struct mtlc_render_pipeline_color_attachment_descriptor self,
    mtlc_uinteger mask);

// -- Encoder raster state ---------------------------------------------------

// -[MTLRenderCommandEncoder setCullMode:] / setFrontFacingWinding: /
// setTriangleFillMode: / setDepthBias:slopeScale:clamp:. Unlike Vulkan these
// are encoder state rather than pipeline state, so they are set per draw.
MTLC_EXTERN_C void mtlc_render_command_encoder_set_cull_mode(
    struct mtlc_render_command_encoder self, enum mtlc_cull_mode mode);

MTLC_EXTERN_C void mtlc_render_command_encoder_set_front_facing_winding(
    struct mtlc_render_command_encoder self, enum mtlc_winding winding);

MTLC_EXTERN_C void mtlc_render_command_encoder_set_triangle_fill_mode(
    struct mtlc_render_command_encoder self,
    enum mtlc_triangle_fill_mode mode);

MTLC_EXTERN_C void mtlc_render_command_encoder_set_depth_bias(
    struct mtlc_render_command_encoder self, float bias, float slope_scale,
    float clamp);

// -- Render pass ------------------------------------------------------------

// +[MTLRenderPassDescriptor renderPassDescriptor] (autoreleased). Never nil.
MTLC_EXTERN_C struct mtlc_render_pass_descriptor
mtlc_render_pass_descriptor_new(void);

// -[MTLRenderPassDescriptor colorAttachments] (autoreleased). Never nil.
MTLC_EXTERN_C struct mtlc_render_pass_color_attachment_descriptor_array
mtlc_render_pass_descriptor_color_attachments(
    struct mtlc_render_pass_descriptor self);

// -[MTLRenderPassDescriptor depthAttachment] (autoreleased). Never nil.
MTLC_EXTERN_C struct mtlc_render_pass_depth_attachment_descriptor
mtlc_render_pass_descriptor_depth_attachment(
    struct mtlc_render_pass_descriptor self);

// -objectAtIndexedSubscript:. Never nil for an in-range index.
MTLC_EXTERN_C struct mtlc_render_pass_color_attachment_descriptor
mtlc_render_pass_color_attachment_descriptor_array_object(
    struct mtlc_render_pass_color_attachment_descriptor_array self,
    mtlc_uinteger index);

MTLC_EXTERN_C void mtlc_render_pass_color_attachment_descriptor_set_texture(
    struct mtlc_render_pass_color_attachment_descriptor self,
    struct mtlc_texture texture);

MTLC_EXTERN_C void mtlc_render_pass_color_attachment_descriptor_set_load_action(
    struct mtlc_render_pass_color_attachment_descriptor self,
    enum mtlc_load_action action);

MTLC_EXTERN_C void mtlc_render_pass_color_attachment_descriptor_set_store_action(
    struct mtlc_render_pass_color_attachment_descriptor self,
    enum mtlc_store_action action);

MTLC_EXTERN_C void mtlc_render_pass_color_attachment_descriptor_set_clear_color(
    struct mtlc_render_pass_color_attachment_descriptor self,
    struct mtlc_clear_color color);

MTLC_EXTERN_C void mtlc_render_pass_depth_attachment_descriptor_set_texture(
    struct mtlc_render_pass_depth_attachment_descriptor self,
    struct mtlc_texture texture);

MTLC_EXTERN_C void mtlc_render_pass_depth_attachment_descriptor_set_load_action(
    struct mtlc_render_pass_depth_attachment_descriptor self,
    enum mtlc_load_action action);

MTLC_EXTERN_C void mtlc_render_pass_depth_attachment_descriptor_set_store_action(
    struct mtlc_render_pass_depth_attachment_descriptor self,
    enum mtlc_store_action action);

MTLC_EXTERN_C void mtlc_render_pass_depth_attachment_descriptor_set_clear_depth(
    struct mtlc_render_pass_depth_attachment_descriptor self, double depth);

// -- MTLTextureDescriptor ---------------------------------------------------

// [[MTLTextureDescriptor alloc] init]. Owned by the caller.
MTLC_EXTERN_C struct mtlc_texture_descriptor mtlc_texture_descriptor_init(void);

// +[MTLTextureDescriptor
//   texture2DDescriptorWithPixelFormat:width:height:mipmapped:]
// (autoreleased). Never nil.
MTLC_EXTERN_C struct mtlc_texture_descriptor
mtlc_texture_descriptor_texture_2d(enum mtlc_pixel_format format,
                                   mtlc_uinteger width, mtlc_uinteger height,
                                   bool mipmapped);

MTLC_EXTERN_C void
mtlc_texture_descriptor_set_pixel_format(struct mtlc_texture_descriptor self,
                                         enum mtlc_pixel_format format);

MTLC_EXTERN_C void
mtlc_texture_descriptor_set_width(struct mtlc_texture_descriptor self,
                                  mtlc_uinteger width);

MTLC_EXTERN_C void
mtlc_texture_descriptor_set_height(struct mtlc_texture_descriptor self,
                                   mtlc_uinteger height);

MTLC_EXTERN_C void
mtlc_texture_descriptor_set_texture_type(struct mtlc_texture_descriptor self,
                                         enum mtlc_texture_type texture_type);

// Number of mip levels the texture is allocated with. Pin this to the count the
// caller actually uploads; relying on texture2DDescriptor's mipmapped: flag makes
// Metal allocate a full floor(log2(max))+1 chain, leaving the smallest levels
// uninitialized when fewer are uploaded.
MTLC_EXTERN_C void
mtlc_texture_descriptor_set_mipmap_level_count(
    struct mtlc_texture_descriptor self, mtlc_uinteger count);

// `usage` is an OR of MTLC_TEXTURE_USAGE_* flags.
MTLC_EXTERN_C void
mtlc_texture_descriptor_set_usage(struct mtlc_texture_descriptor self,
                                  mtlc_uinteger usage);

// Takes the unshifted MTLC_STORAGE_MODE_* value, not the pre-shifted
// MTLC_RESOURCE_STORAGE_MODE_* one.
MTLC_EXTERN_C void
mtlc_texture_descriptor_set_storage_mode(struct mtlc_texture_descriptor self,
                                         enum mtlc_storage_mode mode);

// -- MTLDepthStencilDescriptor ----------------------------------------------

// [[MTLDepthStencilDescriptor alloc] init]. Owned by the caller.
MTLC_EXTERN_C struct mtlc_depth_stencil_descriptor
mtlc_depth_stencil_descriptor_init(void);

MTLC_EXTERN_C void mtlc_depth_stencil_descriptor_set_depth_compare_function(
    struct mtlc_depth_stencil_descriptor self,
    enum mtlc_compare_function func);

MTLC_EXTERN_C void mtlc_depth_stencil_descriptor_set_depth_write_enabled(
    struct mtlc_depth_stencil_descriptor self, bool enabled);

// -- MTLSamplerDescriptor ---------------------------------------------------

// [[MTLSamplerDescriptor alloc] init]. Owned by the caller.
MTLC_EXTERN_C struct mtlc_sampler_descriptor mtlc_sampler_descriptor_init(void);

MTLC_EXTERN_C void mtlc_sampler_descriptor_set_min_filter(
    struct mtlc_sampler_descriptor self, enum mtlc_sampler_min_mag_filter f);
MTLC_EXTERN_C void mtlc_sampler_descriptor_set_mag_filter(
    struct mtlc_sampler_descriptor self, enum mtlc_sampler_min_mag_filter f);
MTLC_EXTERN_C void mtlc_sampler_descriptor_set_mip_filter(
    struct mtlc_sampler_descriptor self, enum mtlc_sampler_mip_filter f);
MTLC_EXTERN_C void mtlc_sampler_descriptor_set_address_mode_s(
    struct mtlc_sampler_descriptor self, enum mtlc_sampler_address_mode m);
MTLC_EXTERN_C void mtlc_sampler_descriptor_set_address_mode_t(
    struct mtlc_sampler_descriptor self, enum mtlc_sampler_address_mode m);
MTLC_EXTERN_C void mtlc_sampler_descriptor_set_address_mode_r(
    struct mtlc_sampler_descriptor self, enum mtlc_sampler_address_mode m);
MTLC_EXTERN_C void mtlc_sampler_descriptor_set_max_anisotropy(
    struct mtlc_sampler_descriptor self, mtlc_uinteger value);
MTLC_EXTERN_C void mtlc_sampler_descriptor_set_compare_function(
    struct mtlc_sampler_descriptor self, enum mtlc_compare_function func);
MTLC_EXTERN_C void mtlc_sampler_descriptor_set_lod_max_clamp(
    struct mtlc_sampler_descriptor self, float value);

// -[MTLSamplerDescriptor setSupportArgumentBuffers:] (macOS 11+).
//
// Required for any sampler that will be encoded into an argument buffer -- without it the sampler has
// no GPU-side handle to encode. Metal API validation fails the encode outright ("Sampler state did not
// have supportArgumentBuffers flag set on creation"); with validation off the behaviour is undefined.
// Costs nothing on samplers that are only ever bound directly.
MTLC_EXTERN_C void mtlc_sampler_descriptor_set_support_argument_buffers(
    struct mtlc_sampler_descriptor self, bool value);

// -- Argument buffers -------------------------------------------------------
//
// An argument buffer is a plain MTLBuffer holding an encoded table of resource
// references, bound to the shader with one setVertex/FragmentBuffer call in
// place of one call per resource. The layout is opaque -- only the MTLArgument
// Encoder vended by the shader function knows it -- which is why the encoder is
// obtained from the function rather than described by hand.
//
// Two rules the compiler cannot enforce for you:
//   * A resource referenced from an argument buffer is invisible to Metal's
//     automatic residency tracking. Declare it with
//     mtlc_render_command_encoder_use_resource(s) or the GPU faults.
//   * An encoder stays associated with its destination buffer between
//     set_argument_buffer and the following set_* calls, and is not thread
//     safe. Keep one buffer's encode contiguous.

// -[MTLFunction newArgumentEncoderWithBufferIndex:]. Owned by the caller.
//
// `buffer_index` MUST name an argument buffer the function actually declares.
// Metal does not return nil for one that does not exist -- it fails an internal
// assertion and aborts the process ("bufferIndex N does not identify an
// argument buffer"), verified on macOS 15 / Apple M3. Decide from your own
// reflection whether the stage uses the set before calling.
//
// Deprecated in the macOS 13 SDK in favour of
// -[MTLDevice newArgumentEncoderWithBufferBinding:], which needs a pipeline
// reflection object. Since this shim dispatches through objc_msgSend and never
// includes a framework header, the deprecation cannot affect the build, and the
// method is still implemented. The alternative would mean describing the layout
// by hand, i.e. predicting indices the cross-compiler chose.
MTLC_EXTERN_C struct mtlc_argument_encoder
mtlc_function_new_argument_encoder(struct mtlc_function self,
                                   mtlc_uinteger buffer_index);

// -[MTLArgumentEncoder encodedLength] -- bytes one encoded table occupies.
MTLC_EXTERN_C mtlc_uinteger
mtlc_argument_encoder_encoded_length(struct mtlc_argument_encoder self);

// -[MTLArgumentEncoder alignment] -- required alignment of the destination
// offset. Do not assume a value; suballocation stride is
// align(encodedLength, alignment).
MTLC_EXTERN_C mtlc_uinteger
mtlc_argument_encoder_alignment(struct mtlc_argument_encoder self);

// -[MTLArgumentEncoder setArgumentBuffer:offset:] -- point the encoder at where
// it should write. Call before any of the set_* calls below.
MTLC_EXTERN_C void
mtlc_argument_encoder_set_argument_buffer(struct mtlc_argument_encoder self,
                                          struct mtlc_buffer buffer,
                                          mtlc_uinteger offset);

// -[MTLArgumentEncoder setTexture:atIndex:]. `index` is the [[id(n)]] the
// shader declares, not a texture-table slot.
MTLC_EXTERN_C void
mtlc_argument_encoder_set_texture(struct mtlc_argument_encoder self,
                                  struct mtlc_texture texture,
                                  mtlc_uinteger index);

// -[MTLArgumentEncoder setSamplerState:atIndex:].
MTLC_EXTERN_C void
mtlc_argument_encoder_set_sampler_state(struct mtlc_argument_encoder self,
                                        struct mtlc_sampler_state sampler,
                                        mtlc_uinteger index);

// -[MTLArgumentEncoder setBuffer:offset:atIndex:].
MTLC_EXTERN_C void
mtlc_argument_encoder_set_buffer(struct mtlc_argument_encoder self,
                                 struct mtlc_buffer buffer,
                                 mtlc_uinteger offset, mtlc_uinteger index);

// -[MTLArgumentEncoder setLabel:].
MTLC_EXTERN_C void
mtlc_argument_encoder_set_label(struct mtlc_argument_encoder self,
                                struct ns_string label);

// -- Residency --------------------------------------------------------------
//
// `resource` is any id conforming to MTLResource -- pass a texture's or
// buffer's .obj directly, the same way mtlc_command_buffer_present_drawable
// takes a raw drawable id. `usage` is an OR of MTLC_RESOURCE_USAGE_*, `stages`
// of MTLC_RENDER_STAGE_*. Residency is encoder state and does not survive
// end_encoding.

// -[MTLRenderCommandEncoder useResource:usage:stages:].
MTLC_EXTERN_C void mtlc_render_command_encoder_use_resource(
    struct mtlc_render_command_encoder self, void *resource,
    mtlc_uinteger usage, mtlc_uinteger stages);

// -[MTLRenderCommandEncoder useResources:count:usage:stages:] -- the batched
// form. `resources` is an array of raw ids; because every mtlc handle is
// asserted to be exactly pointer-sized, an array of handles can be passed
// directly with a cast.
MTLC_EXTERN_C void mtlc_render_command_encoder_use_resources(
    struct mtlc_render_command_encoder self, void *const *resources,
    mtlc_uinteger count, mtlc_uinteger usage, mtlc_uinteger stages);

#endif // MTLC_METAL_H
