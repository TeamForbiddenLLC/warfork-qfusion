// SPDX-License-Identifier: GPL-2.0-or-later
//
// Port of rhi-zig/deps/metal/src/metal.zig. Selector strings are copied
// verbatim from the Zig source -- they are the contract with the framework.

#include "metal-c/metal.h"

#include "internal.h"

__MTLC_HANDLE_IMPL(mtlc_device);
__MTLC_HANDLE_IMPL(mtlc_command_queue);
__MTLC_HANDLE_IMPL(mtlc_command_buffer);
__MTLC_HANDLE_IMPL(mtlc_render_command_encoder);
__MTLC_HANDLE_IMPL(mtlc_buffer);
__MTLC_HANDLE_IMPL(mtlc_texture);
__MTLC_HANDLE_IMPL(mtlc_library);
__MTLC_HANDLE_IMPL(mtlc_function);
__MTLC_HANDLE_IMPL(mtlc_render_pipeline_state);
__MTLC_HANDLE_IMPL(mtlc_depth_stencil_state);
__MTLC_HANDLE_IMPL(mtlc_sampler_state);
__MTLC_HANDLE_IMPL(mtlc_compile_options);
__MTLC_HANDLE_IMPL(mtlc_render_pipeline_descriptor);
__MTLC_HANDLE_IMPL(mtlc_vertex_descriptor);
__MTLC_HANDLE_IMPL(mtlc_vertex_attribute_descriptor_array);
__MTLC_HANDLE_IMPL(mtlc_vertex_attribute_descriptor);
__MTLC_HANDLE_IMPL(mtlc_vertex_buffer_layout_descriptor_array);
__MTLC_HANDLE_IMPL(mtlc_vertex_buffer_layout_descriptor);
__MTLC_HANDLE_IMPL(mtlc_render_pipeline_color_attachment_descriptor_array);
__MTLC_HANDLE_IMPL(mtlc_render_pipeline_color_attachment_descriptor);
__MTLC_HANDLE_IMPL(mtlc_render_pass_descriptor);
__MTLC_HANDLE_IMPL(mtlc_render_pass_color_attachment_descriptor_array);
__MTLC_HANDLE_IMPL(mtlc_render_pass_color_attachment_descriptor);
__MTLC_HANDLE_IMPL(mtlc_render_pass_depth_attachment_descriptor);
__MTLC_HANDLE_IMPL(mtlc_texture_descriptor);
__MTLC_HANDLE_IMPL(mtlc_depth_stencil_descriptor);
__MTLC_HANDLE_IMPL(mtlc_sampler_descriptor);
__MTLC_HANDLE_IMPL(mtlc_argument_encoder);

// A plain C function exported by the Metal framework, not an Objective-C
// method. Returns nil if Metal is unavailable.
extern id MTLCreateSystemDefaultDevice(void);

// -- Device -----------------------------------------------------------------

struct mtlc_device mtlc_create_system_default_device(void) {
  return mtlc_device_from_id((void *)MTLCreateSystemDefaultDevice());
}

struct ns_string mtlc_device_name(struct mtlc_device self) {
  return __MTLC_GET_OBJ(ns_string, self, "name");
}

bool mtlc_device_is_low_power(struct mtlc_device self) {
  __mtlc_bool r = __MTLC_MSG(__mtlc_bool)((id)self.obj,
                                          __MTLC_SEL("isLowPower"));
  return __MTLC_FROM_BOOL(r);
}

bool mtlc_device_has_unified_memory(struct mtlc_device self) {
  __mtlc_bool r =
      __MTLC_MSG(__mtlc_bool)((id)self.obj, __MTLC_SEL("hasUnifiedMemory"));
  return __MTLC_FROM_BOOL(r);
}

enum mtlc_argument_buffers_tier
mtlc_device_argument_buffers_support(struct mtlc_device self) {
  return (enum mtlc_argument_buffers_tier)__MTLC_MSG(mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("argumentBuffersSupport"));
}

mtlc_uinteger
mtlc_device_max_argument_buffer_sampler_count(struct mtlc_device self) {
  return __MTLC_MSG(mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("maxArgumentBufferSamplerCount"));
}

struct mtlc_command_queue
mtlc_device_new_command_queue(struct mtlc_device self) {
  return __MTLC_GET_OBJ(mtlc_command_queue, self, "newCommandQueue");
}

struct mtlc_buffer mtlc_device_new_buffer(struct mtlc_device self,
                                          mtlc_uinteger length,
                                          mtlc_uinteger options) {
  id r = __MTLC_MSG(id, mtlc_uinteger, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("newBufferWithLength:options:"), length,
      options);
  return mtlc_buffer_from_id((void *)r);
}

struct mtlc_buffer mtlc_device_new_buffer_with_bytes(struct mtlc_device self,
                                                     const void *bytes,
                                                     mtlc_uinteger length,
                                                     mtlc_uinteger options) {
  id r = __MTLC_MSG(id, const void *, mtlc_uinteger, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("newBufferWithBytes:length:options:"), bytes,
      length, options);
  return mtlc_buffer_from_id((void *)r);
}

struct mtlc_texture
mtlc_device_new_texture(struct mtlc_device self,
                        struct mtlc_texture_descriptor descriptor) {
  id r = __MTLC_MSG(id, id)((id)self.obj,
                            __MTLC_SEL("newTextureWithDescriptor:"),
                            (id)descriptor.obj);
  return mtlc_texture_from_id((void *)r);
}

struct mtlc_depth_stencil_state mtlc_device_new_depth_stencil_state(
    struct mtlc_device self, struct mtlc_depth_stencil_descriptor descriptor) {
  id r = __MTLC_MSG(id, id)(
      (id)self.obj, __MTLC_SEL("newDepthStencilStateWithDescriptor:"),
      (id)descriptor.obj);
  return mtlc_depth_stencil_state_from_id((void *)r);
}

struct mtlc_sampler_state
mtlc_device_new_sampler_state(struct mtlc_device self,
                              struct mtlc_sampler_descriptor descriptor) {
  id r = __MTLC_MSG(id, id)((id)self.obj,
                            __MTLC_SEL("newSamplerStateWithDescriptor:"),
                            (id)descriptor.obj);
  return mtlc_sampler_state_from_id((void *)r);
}

struct mtlc_library mtlc_device_new_library_with_source(
    struct mtlc_device self, struct ns_string source,
    struct mtlc_compile_options options, struct ns_error *out_err) {
  id err = NULL;
  id r = __MTLC_MSG(id, id, id, id *)(
      (id)self.obj, __MTLC_SEL("newLibraryWithSource:options:error:"),
      (id)source.obj, (id)options.obj, &err);
  if (out_err != NULL)
    *out_err = ns_error_from_id((void *)err);
  return mtlc_library_from_id((void *)r);
}

struct mtlc_render_pipeline_state mtlc_device_new_render_pipeline_state(
    struct mtlc_device self, struct mtlc_render_pipeline_descriptor descriptor,
    struct ns_error *out_err) {
  id err = NULL;
  id r = __MTLC_MSG(id, id, id *)(
      (id)self.obj, __MTLC_SEL("newRenderPipelineStateWithDescriptor:error:"),
      (id)descriptor.obj, &err);
  if (out_err != NULL)
    *out_err = ns_error_from_id((void *)err);
  return mtlc_render_pipeline_state_from_id((void *)r);
}

// -- Command submission -----------------------------------------------------

struct mtlc_command_buffer
mtlc_command_queue_command_buffer(struct mtlc_command_queue self) {
  return __MTLC_GET_OBJ(mtlc_command_buffer, self, "commandBuffer");
}

void mtlc_command_queue_set_label(struct mtlc_command_queue self,
                                  struct ns_string label) {
  __MTLC_MSG(void, id)((id)self.obj, __MTLC_SEL("setLabel:"), (id)label.obj);
}

struct mtlc_render_command_encoder mtlc_command_buffer_render_command_encoder(
    struct mtlc_command_buffer self,
    struct mtlc_render_pass_descriptor descriptor) {
  id r = __MTLC_MSG(id, id)(
      (id)self.obj, __MTLC_SEL("renderCommandEncoderWithDescriptor:"),
      (id)descriptor.obj);
  return mtlc_render_command_encoder_from_id((void *)r);
}

void mtlc_command_buffer_present_drawable(struct mtlc_command_buffer self,
                                          void *drawable) {
  __MTLC_MSG(void, id)((id)self.obj, __MTLC_SEL("presentDrawable:"),
                       (id)drawable);
}

void mtlc_command_buffer_commit(struct mtlc_command_buffer self) {
  __MTLC_MSG(void)((id)self.obj, __MTLC_SEL("commit"));
}

void mtlc_command_buffer_wait_until_completed(struct mtlc_command_buffer self) {
  __MTLC_MSG(void)((id)self.obj, __MTLC_SEL("waitUntilCompleted"));
}

void mtlc_command_buffer_wait_until_scheduled(struct mtlc_command_buffer self) {
  __MTLC_MSG(void)((id)self.obj, __MTLC_SEL("waitUntilScheduled"));
}

void mtlc_command_buffer_add_completed_handler(struct mtlc_command_buffer self,
                                               void (*fn)(void *ctx),
                                               void *ctx) {
  // addCompletedHandler: copies the block, so the stack block is safe to pass.
  // Apple clang enables blocks by default. fn/ctx are captured by value.
  void (^blk)(id) = ^(id cb) {
    (void)cb;
    if (fn)
      fn(ctx);
  };
  __MTLC_MSG(void, id)((id)self.obj, __MTLC_SEL("addCompletedHandler:"),
                       (id)blk);
}

// -- Render command encoder -------------------------------------------------

void mtlc_render_command_encoder_set_render_pipeline_state(
    struct mtlc_render_command_encoder self,
    struct mtlc_render_pipeline_state state) {
  __MTLC_MSG(void, id)((id)self.obj, __MTLC_SEL("setRenderPipelineState:"),
                       (id)state.obj);
}

void mtlc_render_command_encoder_set_vertex_buffer(
    struct mtlc_render_command_encoder self, struct mtlc_buffer buffer,
    mtlc_uinteger offset, mtlc_uinteger index) {
  __MTLC_MSG(void, id, mtlc_uinteger, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("setVertexBuffer:offset:atIndex:"),
      (id)buffer.obj, offset, index);
}

void mtlc_render_command_encoder_set_vertex_bytes(
    struct mtlc_render_command_encoder self, const void *bytes,
    mtlc_uinteger length, mtlc_uinteger index) {
  __MTLC_MSG(void, const void *, mtlc_uinteger, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("setVertexBytes:length:atIndex:"), bytes, length,
      index);
}

void mtlc_render_command_encoder_set_fragment_buffer(
    struct mtlc_render_command_encoder self, struct mtlc_buffer buffer,
    mtlc_uinteger offset, mtlc_uinteger index) {
  __MTLC_MSG(void, id, mtlc_uinteger, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("setFragmentBuffer:offset:atIndex:"),
      (id)buffer.obj, offset, index);
}

void mtlc_render_command_encoder_set_fragment_bytes(
    struct mtlc_render_command_encoder self, const void *bytes,
    mtlc_uinteger length, mtlc_uinteger index) {
  __MTLC_MSG(void, const void *, mtlc_uinteger, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("setFragmentBytes:length:atIndex:"), bytes,
      length, index);
}

void mtlc_render_command_encoder_set_vertex_texture(
    struct mtlc_render_command_encoder self, struct mtlc_texture texture,
    mtlc_uinteger index) {
  __MTLC_MSG(void, id, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("setVertexTexture:atIndex:"), (id)texture.obj,
      index);
}

void mtlc_render_command_encoder_set_fragment_texture(
    struct mtlc_render_command_encoder self, struct mtlc_texture texture,
    mtlc_uinteger index) {
  __MTLC_MSG(void, id, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("setFragmentTexture:atIndex:"), (id)texture.obj,
      index);
}

void mtlc_render_command_encoder_set_vertex_sampler_state(
    struct mtlc_render_command_encoder self, struct mtlc_sampler_state sampler,
    mtlc_uinteger index) {
  __MTLC_MSG(void, id, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("setVertexSamplerState:atIndex:"),
      (id)sampler.obj, index);
}

void mtlc_render_command_encoder_set_fragment_sampler_state(
    struct mtlc_render_command_encoder self, struct mtlc_sampler_state sampler,
    mtlc_uinteger index) {
  __MTLC_MSG(void, id, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("setFragmentSamplerState:atIndex:"),
      (id)sampler.obj, index);
}

void mtlc_render_command_encoder_set_viewport(
    struct mtlc_render_command_encoder self, struct mtlc_viewport viewport) {
  __MTLC_MSG(void, struct mtlc_viewport)((id)self.obj,
                                         __MTLC_SEL("setViewport:"), viewport);
}

void mtlc_render_command_encoder_set_depth_stencil_state(
    struct mtlc_render_command_encoder self,
    struct mtlc_depth_stencil_state state) {
  __MTLC_MSG(void, id)((id)self.obj, __MTLC_SEL("setDepthStencilState:"),
                       (id)state.obj);
}

void mtlc_render_command_encoder_set_scissor_rect(
    struct mtlc_render_command_encoder self, struct mtlc_scissor_rect rect) {
  __MTLC_MSG(void, struct mtlc_scissor_rect)(
      (id)self.obj, __MTLC_SEL("setScissorRect:"), rect);
}

void mtlc_render_command_encoder_draw_primitives(
    struct mtlc_render_command_encoder self,
    enum mtlc_primitive_type primitive_type, mtlc_uinteger vertex_start,
    mtlc_uinteger vertex_count, mtlc_uinteger instance_count,
    mtlc_uinteger base_instance) {
  __MTLC_MSG(void, mtlc_uinteger, mtlc_uinteger, mtlc_uinteger, mtlc_uinteger,
             mtlc_uinteger)(
      (id)self.obj,
      __MTLC_SEL("drawPrimitives:vertexStart:vertexCount:instanceCount:"
                 "baseInstance:"),
      (mtlc_uinteger)primitive_type, vertex_start, vertex_count, instance_count,
      base_instance);
}

void mtlc_render_command_encoder_draw_indexed_primitives(
    struct mtlc_render_command_encoder self,
    enum mtlc_primitive_type primitive_type, mtlc_uinteger index_count,
    enum mtlc_index_type index_type, struct mtlc_buffer index_buffer,
    mtlc_uinteger index_buffer_offset, mtlc_uinteger instance_count,
    mtlc_integer base_vertex, mtlc_uinteger base_instance) {
  __MTLC_MSG(void, mtlc_uinteger, mtlc_uinteger, mtlc_uinteger, id,
             mtlc_uinteger, mtlc_uinteger, mtlc_integer, mtlc_uinteger)(
      (id)self.obj,
      __MTLC_SEL("drawIndexedPrimitives:indexCount:indexType:indexBuffer:"
                 "indexBufferOffset:instanceCount:baseVertex:baseInstance:"),
      (mtlc_uinteger)primitive_type, index_count, (mtlc_uinteger)index_type,
      (id)index_buffer.obj, index_buffer_offset, instance_count, base_vertex,
      base_instance);
}

void mtlc_render_command_encoder_end_encoding(
    struct mtlc_render_command_encoder self) {
  __MTLC_MSG(void)((id)self.obj, __MTLC_SEL("endEncoding"));
}

// -- Resources --------------------------------------------------------------

void *mtlc_buffer_contents(struct mtlc_buffer self) {
  return __MTLC_MSG(void *)((id)self.obj, __MTLC_SEL("contents"));
}

void mtlc_buffer_did_modify_range(struct mtlc_buffer self,
                                  struct ns_range range) {
  __MTLC_MSG(void, struct ns_range)(
      (id)self.obj, __MTLC_SEL("didModifyRange:"), range);
}

mtlc_uinteger mtlc_buffer_length(struct mtlc_buffer self) {
  return __MTLC_MSG(mtlc_uinteger)((id)self.obj, __MTLC_SEL("length"));
}

mtlc_uinteger mtlc_texture_width(struct mtlc_texture self) {
  return __MTLC_MSG(mtlc_uinteger)((id)self.obj, __MTLC_SEL("width"));
}

mtlc_uinteger mtlc_texture_height(struct mtlc_texture self) {
  return __MTLC_MSG(mtlc_uinteger)((id)self.obj, __MTLC_SEL("height"));
}

void mtlc_texture_replace_region(struct mtlc_texture self,
                                 struct mtlc_region region,
                                 mtlc_uinteger mip_level, mtlc_uinteger slice,
                                 const void *bytes, mtlc_uinteger bytes_per_row,
                                 mtlc_uinteger bytes_per_image) {
  __MTLC_MSG(void, struct mtlc_region, mtlc_uinteger, mtlc_uinteger,
             const void *, mtlc_uinteger, mtlc_uinteger)(
      (id)self.obj,
      __MTLC_SEL("replaceRegion:mipmapLevel:slice:withBytes:bytesPerRow:"
                 "bytesPerImage:"),
      region, mip_level, slice, bytes, bytes_per_row, bytes_per_image);
}

// -- Shaders / pipeline state -----------------------------------------------

struct mtlc_function mtlc_library_new_function(struct mtlc_library self,
                                               struct ns_string function_name) {
  id r = __MTLC_MSG(id, id)((id)self.obj, __MTLC_SEL("newFunctionWithName:"),
                            (id)function_name.obj);
  return mtlc_function_from_id((void *)r);
}

// -- Descriptors ------------------------------------------------------------

struct mtlc_compile_options mtlc_compile_options_init(void) {
  return mtlc_compile_options_from_id(
      (void *)__mtlc_alloc_init("MTLCompileOptions"));
}

struct mtlc_render_pipeline_descriptor
mtlc_render_pipeline_descriptor_init(void) {
  return mtlc_render_pipeline_descriptor_from_id(
      (void *)__mtlc_alloc_init("MTLRenderPipelineDescriptor"));
}

void mtlc_render_pipeline_descriptor_set_vertex_function(
    struct mtlc_render_pipeline_descriptor self, struct mtlc_function function) {
  __MTLC_MSG(void, id)((id)self.obj, __MTLC_SEL("setVertexFunction:"),
                       (id)function.obj);
}

void mtlc_render_pipeline_descriptor_set_fragment_function(
    struct mtlc_render_pipeline_descriptor self, struct mtlc_function function) {
  __MTLC_MSG(void, id)((id)self.obj, __MTLC_SEL("setFragmentFunction:"),
                       (id)function.obj);
}

void mtlc_render_pipeline_descriptor_set_label(
    struct mtlc_render_pipeline_descriptor self, struct ns_string label) {
  __MTLC_MSG(void, id)((id)self.obj, __MTLC_SEL("setLabel:"), (id)label.obj);
}

void mtlc_render_pipeline_descriptor_set_vertex_descriptor(
    struct mtlc_render_pipeline_descriptor self,
    struct mtlc_vertex_descriptor vertex_descriptor) {
  __MTLC_MSG(void, id)((id)self.obj, __MTLC_SEL("setVertexDescriptor:"),
                       (id)vertex_descriptor.obj);
}

void mtlc_render_pipeline_descriptor_set_depth_attachment_pixel_format(
    struct mtlc_render_pipeline_descriptor self,
    enum mtlc_pixel_format format) {
  __MTLC_MSG(void, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("setDepthAttachmentPixelFormat:"),
      (mtlc_uinteger)format);
}

struct mtlc_render_pipeline_color_attachment_descriptor_array
mtlc_render_pipeline_descriptor_color_attachments(
    struct mtlc_render_pipeline_descriptor self) {
  return __MTLC_GET_OBJ(mtlc_render_pipeline_color_attachment_descriptor_array,
                        self, "colorAttachments");
}

// -- MTLVertexDescriptor ----------------------------------------------------

struct mtlc_vertex_descriptor mtlc_vertex_descriptor_new(void) {
  return mtlc_vertex_descriptor_from_id(
      (void *)__mtlc_class_msg("MTLVertexDescriptor", "vertexDescriptor"));
}

struct mtlc_vertex_attribute_descriptor_array
mtlc_vertex_descriptor_attributes(struct mtlc_vertex_descriptor self) {
  return __MTLC_GET_OBJ(mtlc_vertex_attribute_descriptor_array, self,
                        "attributes");
}

struct mtlc_vertex_buffer_layout_descriptor_array
mtlc_vertex_descriptor_layouts(struct mtlc_vertex_descriptor self) {
  return __MTLC_GET_OBJ(mtlc_vertex_buffer_layout_descriptor_array, self,
                        "layouts");
}

struct mtlc_vertex_attribute_descriptor
mtlc_vertex_attribute_descriptor_array_object(
    struct mtlc_vertex_attribute_descriptor_array self, mtlc_uinteger index) {
  return __MTLC_GET_INDEXED(mtlc_vertex_attribute_descriptor, self, index);
}

void mtlc_vertex_attribute_descriptor_set_format(
    struct mtlc_vertex_attribute_descriptor self,
    enum mtlc_vertex_format format) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setFormat:"),
                                  (mtlc_uinteger)format);
}

void mtlc_vertex_attribute_descriptor_set_offset(
    struct mtlc_vertex_attribute_descriptor self, mtlc_uinteger offset) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setOffset:"),
                                  offset);
}

void mtlc_vertex_attribute_descriptor_set_buffer_index(
    struct mtlc_vertex_attribute_descriptor self, mtlc_uinteger index) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setBufferIndex:"),
                                  index);
}

struct mtlc_vertex_buffer_layout_descriptor
mtlc_vertex_buffer_layout_descriptor_array_object(
    struct mtlc_vertex_buffer_layout_descriptor_array self,
    mtlc_uinteger index) {
  return __MTLC_GET_INDEXED(mtlc_vertex_buffer_layout_descriptor, self, index);
}

void mtlc_vertex_buffer_layout_descriptor_set_stride(
    struct mtlc_vertex_buffer_layout_descriptor self, mtlc_uinteger stride) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setStride:"),
                                  stride);
}

void mtlc_vertex_buffer_layout_descriptor_set_step_function(
    struct mtlc_vertex_buffer_layout_descriptor self,
    enum mtlc_vertex_step_function step) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setStepFunction:"),
                                  (mtlc_uinteger)step);
}

void mtlc_vertex_buffer_layout_descriptor_set_step_rate(
    struct mtlc_vertex_buffer_layout_descriptor self, mtlc_uinteger rate) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setStepRate:"),
                                  rate);
}

// -- Render pipeline colour attachments -------------------------------------

struct mtlc_render_pipeline_color_attachment_descriptor
mtlc_render_pipeline_color_attachment_descriptor_array_object(
    struct mtlc_render_pipeline_color_attachment_descriptor_array self,
    mtlc_uinteger index) {
  return __MTLC_GET_INDEXED(mtlc_render_pipeline_color_attachment_descriptor,
                            self, index);
}

void mtlc_render_pipeline_color_attachment_descriptor_set_pixel_format(
    struct mtlc_render_pipeline_color_attachment_descriptor self,
    enum mtlc_pixel_format format) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setPixelFormat:"),
                                  (mtlc_uinteger)format);
}

void mtlc_render_pipeline_color_attachment_descriptor_set_blending_enabled(
    struct mtlc_render_pipeline_color_attachment_descriptor self,
    bool enabled) {
  __MTLC_MSG(void, __mtlc_bool)((id)self.obj,
                                __MTLC_SEL("setBlendingEnabled:"),
                                (__mtlc_bool)(enabled ? 1 : 0));
}

void mtlc_render_pipeline_color_attachment_descriptor_set_rgb_blend_factors(
    struct mtlc_render_pipeline_color_attachment_descriptor self,
    enum mtlc_blend_factor source, enum mtlc_blend_factor destination) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj,
                                  __MTLC_SEL("setSourceRGBBlendFactor:"),
                                  (mtlc_uinteger)source);
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj,
                                  __MTLC_SEL("setDestinationRGBBlendFactor:"),
                                  (mtlc_uinteger)destination);
}

void mtlc_render_pipeline_color_attachment_descriptor_set_alpha_blend_factors(
    struct mtlc_render_pipeline_color_attachment_descriptor self,
    enum mtlc_blend_factor source, enum mtlc_blend_factor destination) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj,
                                  __MTLC_SEL("setSourceAlphaBlendFactor:"),
                                  (mtlc_uinteger)source);
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj,
                                  __MTLC_SEL("setDestinationAlphaBlendFactor:"),
                                  (mtlc_uinteger)destination);
}

void mtlc_render_pipeline_color_attachment_descriptor_set_blend_operations(
    struct mtlc_render_pipeline_color_attachment_descriptor self,
    enum mtlc_blend_operation rgb, enum mtlc_blend_operation alpha) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj,
                                  __MTLC_SEL("setRgbBlendOperation:"),
                                  (mtlc_uinteger)rgb);
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj,
                                  __MTLC_SEL("setAlphaBlendOperation:"),
                                  (mtlc_uinteger)alpha);
}

void mtlc_render_pipeline_color_attachment_descriptor_set_write_mask(
    struct mtlc_render_pipeline_color_attachment_descriptor self,
    mtlc_uinteger mask) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setWriteMask:"),
                                  mask);
}

// -- Encoder raster state ---------------------------------------------------

void mtlc_render_command_encoder_set_cull_mode(
    struct mtlc_render_command_encoder self, enum mtlc_cull_mode mode) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setCullMode:"),
                                  (mtlc_uinteger)mode);
}

void mtlc_render_command_encoder_set_front_facing_winding(
    struct mtlc_render_command_encoder self, enum mtlc_winding winding) {
  __MTLC_MSG(void, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("setFrontFacingWinding:"),
      (mtlc_uinteger)winding);
}

void mtlc_render_command_encoder_set_triangle_fill_mode(
    struct mtlc_render_command_encoder self,
    enum mtlc_triangle_fill_mode mode) {
  __MTLC_MSG(void, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("setTriangleFillMode:"), (mtlc_uinteger)mode);
}

void mtlc_render_command_encoder_set_depth_bias(
    struct mtlc_render_command_encoder self, float bias, float slope_scale,
    float clamp) {
  __MTLC_MSG(void, float, float, float)(
      (id)self.obj, __MTLC_SEL("setDepthBias:slopeScale:clamp:"), bias,
      slope_scale, clamp);
}

// -- Render pass ------------------------------------------------------------

struct mtlc_render_pass_descriptor mtlc_render_pass_descriptor_new(void) {
  return mtlc_render_pass_descriptor_from_id((void *)__mtlc_class_msg(
      "MTLRenderPassDescriptor", "renderPassDescriptor"));
}

struct mtlc_render_pass_color_attachment_descriptor_array
mtlc_render_pass_descriptor_color_attachments(
    struct mtlc_render_pass_descriptor self) {
  return __MTLC_GET_OBJ(mtlc_render_pass_color_attachment_descriptor_array,
                        self, "colorAttachments");
}

struct mtlc_render_pass_depth_attachment_descriptor
mtlc_render_pass_descriptor_depth_attachment(
    struct mtlc_render_pass_descriptor self) {
  return __MTLC_GET_OBJ(mtlc_render_pass_depth_attachment_descriptor, self,
                        "depthAttachment");
}

struct mtlc_render_pass_color_attachment_descriptor
mtlc_render_pass_color_attachment_descriptor_array_object(
    struct mtlc_render_pass_color_attachment_descriptor_array self,
    mtlc_uinteger index) {
  return __MTLC_GET_INDEXED(mtlc_render_pass_color_attachment_descriptor, self,
                            index);
}

void mtlc_render_pass_color_attachment_descriptor_set_texture(
    struct mtlc_render_pass_color_attachment_descriptor self,
    struct mtlc_texture texture) {
  __MTLC_MSG(void, id)((id)self.obj, __MTLC_SEL("setTexture:"),
                       (id)texture.obj);
}

void mtlc_render_pass_color_attachment_descriptor_set_load_action(
    struct mtlc_render_pass_color_attachment_descriptor self,
    enum mtlc_load_action action) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setLoadAction:"),
                                  (mtlc_uinteger)action);
}

void mtlc_render_pass_color_attachment_descriptor_set_store_action(
    struct mtlc_render_pass_color_attachment_descriptor self,
    enum mtlc_store_action action) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setStoreAction:"),
                                  (mtlc_uinteger)action);
}

void mtlc_render_pass_color_attachment_descriptor_set_clear_color(
    struct mtlc_render_pass_color_attachment_descriptor self,
    struct mtlc_clear_color color) {
  __MTLC_MSG(void, struct mtlc_clear_color)(
      (id)self.obj, __MTLC_SEL("setClearColor:"), color);
}

void mtlc_render_pass_depth_attachment_descriptor_set_texture(
    struct mtlc_render_pass_depth_attachment_descriptor self,
    struct mtlc_texture texture) {
  __MTLC_MSG(void, id)((id)self.obj, __MTLC_SEL("setTexture:"),
                       (id)texture.obj);
}

void mtlc_render_pass_depth_attachment_descriptor_set_load_action(
    struct mtlc_render_pass_depth_attachment_descriptor self,
    enum mtlc_load_action action) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setLoadAction:"),
                                  (mtlc_uinteger)action);
}

void mtlc_render_pass_depth_attachment_descriptor_set_store_action(
    struct mtlc_render_pass_depth_attachment_descriptor self,
    enum mtlc_store_action action) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setStoreAction:"),
                                  (mtlc_uinteger)action);
}

void mtlc_render_pass_depth_attachment_descriptor_set_clear_depth(
    struct mtlc_render_pass_depth_attachment_descriptor self, double depth) {
  __MTLC_MSG(void, double)((id)self.obj, __MTLC_SEL("setClearDepth:"), depth);
}

// -- MTLTextureDescriptor ---------------------------------------------------

struct mtlc_texture_descriptor mtlc_texture_descriptor_init(void) {
  return mtlc_texture_descriptor_from_id(
      (void *)__mtlc_alloc_init("MTLTextureDescriptor"));
}

struct mtlc_texture_descriptor
mtlc_texture_descriptor_texture_2d(enum mtlc_pixel_format format,
                                   mtlc_uinteger width, mtlc_uinteger height,
                                   bool mipmapped) {
  id cls = (id)__mtlc_class("MTLTextureDescriptor");
  id r = __MTLC_MSG(id, mtlc_uinteger, mtlc_uinteger, mtlc_uinteger,
                    __mtlc_bool)(
      cls,
      __MTLC_SEL("texture2DDescriptorWithPixelFormat:width:height:mipmapped:"),
      (mtlc_uinteger)format, width, height, __MTLC_TO_BOOL(mipmapped));
  return mtlc_texture_descriptor_from_id((void *)r);
}

void mtlc_texture_descriptor_set_pixel_format(
    struct mtlc_texture_descriptor self, enum mtlc_pixel_format format) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setPixelFormat:"),
                                  (mtlc_uinteger)format);
}

void mtlc_texture_descriptor_set_width(struct mtlc_texture_descriptor self,
                                       mtlc_uinteger width) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setWidth:"), width);
}

void mtlc_texture_descriptor_set_height(struct mtlc_texture_descriptor self,
                                        mtlc_uinteger height) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setHeight:"),
                                  height);
}

void mtlc_texture_descriptor_set_texture_type(
    struct mtlc_texture_descriptor self, enum mtlc_texture_type texture_type) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setTextureType:"),
                                  (mtlc_uinteger)texture_type);
}

void mtlc_texture_descriptor_set_mipmap_level_count(
    struct mtlc_texture_descriptor self, mtlc_uinteger count) {
  __MTLC_MSG(void, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("setMipmapLevelCount:"), count);
}

void mtlc_texture_descriptor_set_usage(struct mtlc_texture_descriptor self,
                                       mtlc_uinteger usage) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setUsage:"), usage);
}

void mtlc_texture_descriptor_set_storage_mode(
    struct mtlc_texture_descriptor self, enum mtlc_storage_mode mode) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setStorageMode:"),
                                  (mtlc_uinteger)mode);
}

// -- MTLDepthStencilDescriptor ----------------------------------------------

struct mtlc_depth_stencil_descriptor mtlc_depth_stencil_descriptor_init(void) {
  return mtlc_depth_stencil_descriptor_from_id(
      (void *)__mtlc_alloc_init("MTLDepthStencilDescriptor"));
}

void mtlc_depth_stencil_descriptor_set_depth_compare_function(
    struct mtlc_depth_stencil_descriptor self,
    enum mtlc_compare_function func) {
  __MTLC_MSG(void, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("setDepthCompareFunction:"),
      (mtlc_uinteger)func);
}

void mtlc_depth_stencil_descriptor_set_depth_write_enabled(
    struct mtlc_depth_stencil_descriptor self, bool enabled) {
  __MTLC_MSG(void, __mtlc_bool)((id)self.obj,
                                __MTLC_SEL("setDepthWriteEnabled:"),
                                __MTLC_TO_BOOL(enabled));
}

// -- MTLSamplerDescriptor ---------------------------------------------------

struct mtlc_sampler_descriptor mtlc_sampler_descriptor_init(void) {
  return mtlc_sampler_descriptor_from_id(
      (void *)__mtlc_alloc_init("MTLSamplerDescriptor"));
}

void mtlc_sampler_descriptor_set_min_filter(
    struct mtlc_sampler_descriptor self, enum mtlc_sampler_min_mag_filter f) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setMinFilter:"),
                                  (mtlc_uinteger)f);
}

void mtlc_sampler_descriptor_set_mag_filter(
    struct mtlc_sampler_descriptor self, enum mtlc_sampler_min_mag_filter f) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setMagFilter:"),
                                  (mtlc_uinteger)f);
}

void mtlc_sampler_descriptor_set_mip_filter(
    struct mtlc_sampler_descriptor self, enum mtlc_sampler_mip_filter f) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setMipFilter:"),
                                  (mtlc_uinteger)f);
}

void mtlc_sampler_descriptor_set_address_mode_s(
    struct mtlc_sampler_descriptor self, enum mtlc_sampler_address_mode m) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setSAddressMode:"),
                                  (mtlc_uinteger)m);
}

void mtlc_sampler_descriptor_set_address_mode_t(
    struct mtlc_sampler_descriptor self, enum mtlc_sampler_address_mode m) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setTAddressMode:"),
                                  (mtlc_uinteger)m);
}

void mtlc_sampler_descriptor_set_address_mode_r(
    struct mtlc_sampler_descriptor self, enum mtlc_sampler_address_mode m) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setRAddressMode:"),
                                  (mtlc_uinteger)m);
}

void mtlc_sampler_descriptor_set_max_anisotropy(
    struct mtlc_sampler_descriptor self, mtlc_uinteger value) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj, __MTLC_SEL("setMaxAnisotropy:"),
                                  value);
}

void mtlc_sampler_descriptor_set_support_argument_buffers(
    struct mtlc_sampler_descriptor self, bool value) {
  __MTLC_MSG(void, __mtlc_bool)((id)self.obj,
                                __MTLC_SEL("setSupportArgumentBuffers:"),
                                __MTLC_TO_BOOL(value));
}

void mtlc_sampler_descriptor_set_compare_function(
    struct mtlc_sampler_descriptor self, enum mtlc_compare_function func) {
  __MTLC_MSG(void, mtlc_uinteger)((id)self.obj,
                                  __MTLC_SEL("setCompareFunction:"),
                                  (mtlc_uinteger)func);
}

void mtlc_sampler_descriptor_set_lod_max_clamp(
    struct mtlc_sampler_descriptor self, float value) {
  __MTLC_MSG(void, float)((id)self.obj, __MTLC_SEL("setLodMaxClamp:"), value);
}

// -- Argument buffers -------------------------------------------------------

struct mtlc_argument_encoder
mtlc_function_new_argument_encoder(struct mtlc_function self,
                                   mtlc_uinteger buffer_index) {
  id r = __MTLC_MSG(id, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("newArgumentEncoderWithBufferIndex:"),
      buffer_index);
  return mtlc_argument_encoder_from_id((void *)r);
}

mtlc_uinteger
mtlc_argument_encoder_encoded_length(struct mtlc_argument_encoder self) {
  return __MTLC_MSG(mtlc_uinteger)((id)self.obj,
                                   __MTLC_SEL("encodedLength"));
}

mtlc_uinteger
mtlc_argument_encoder_alignment(struct mtlc_argument_encoder self) {
  return __MTLC_MSG(mtlc_uinteger)((id)self.obj, __MTLC_SEL("alignment"));
}

void mtlc_argument_encoder_set_argument_buffer(
    struct mtlc_argument_encoder self, struct mtlc_buffer buffer,
    mtlc_uinteger offset) {
  __MTLC_MSG(void, id, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("setArgumentBuffer:offset:"), (id)buffer.obj,
      offset);
}

void mtlc_argument_encoder_set_texture(struct mtlc_argument_encoder self,
                                       struct mtlc_texture texture,
                                       mtlc_uinteger index) {
  __MTLC_MSG(void, id, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("setTexture:atIndex:"), (id)texture.obj, index);
}

void mtlc_argument_encoder_set_sampler_state(struct mtlc_argument_encoder self,
                                             struct mtlc_sampler_state sampler,
                                             mtlc_uinteger index) {
  __MTLC_MSG(void, id, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("setSamplerState:atIndex:"), (id)sampler.obj,
      index);
}

void mtlc_argument_encoder_set_buffer(struct mtlc_argument_encoder self,
                                      struct mtlc_buffer buffer,
                                      mtlc_uinteger offset,
                                      mtlc_uinteger index) {
  __MTLC_MSG(void, id, mtlc_uinteger, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("setBuffer:offset:atIndex:"), (id)buffer.obj,
      offset, index);
}

void mtlc_argument_encoder_set_label(struct mtlc_argument_encoder self,
                                     struct ns_string label) {
  __MTLC_MSG(void, id)((id)self.obj, __MTLC_SEL("setLabel:"), (id)label.obj);
}

// -- Residency --------------------------------------------------------------

void mtlc_render_command_encoder_use_resource(
    struct mtlc_render_command_encoder self, void *resource,
    mtlc_uinteger usage, mtlc_uinteger stages) {
  __MTLC_MSG(void, id, mtlc_uinteger, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("useResource:usage:stages:"), (id)resource,
      usage, stages);
}

void mtlc_render_command_encoder_use_resources(
    struct mtlc_render_command_encoder self, void *const *resources,
    mtlc_uinteger count, mtlc_uinteger usage, mtlc_uinteger stages) {
  __MTLC_MSG(void, const void *, mtlc_uinteger, mtlc_uinteger, mtlc_uinteger)(
      (id)self.obj, __MTLC_SEL("useResources:count:usage:stages:"), resources,
      count, usage, stages);
}
