/*
 * Emscripten JS GL imports cannot be stored in a wasm function-pointer
 * table. Calling through qgl* pointers yields "indirect call to null".
 * Alias qgl* to the real gl* symbols so calls are direct imports.
 */
#ifndef QGL_WASM_ALIASES_H
#define QGL_WASM_ALIASES_H

#if defined( __EMSCRIPTEN__ ) || defined( __WASM__ )

#define qglBindTexture glBindTexture
#define qglClear glClear
#define qglClearColor glClearColor
#define qglClearStencil glClearStencil
#define qglColorMask glColorMask
#define qglCullFace glCullFace
#define qglGenTextures glGenTextures
#define qglDeleteTextures glDeleteTextures
#define qglDepthFunc glDepthFunc
#define qglDepthMask glDepthMask
#define qglDisable glDisable
#define qglDrawElements glDrawElements
#define qglEnable glEnable
#define qglFinish glFinish
#define qglFlush glFlush
#define qglFrontFace glFrontFace
#define qglGetError glGetError
#define qglGetIntegerv glGetIntegerv
#define qglGetString glGetString
#define qglPixelStorei glPixelStorei
#define qglPolygonOffset glPolygonOffset
#define qglReadPixels glReadPixels
#define qglScissor glScissor
#define qglStencilFunc glStencilFunc
#define qglStencilMask glStencilMask
#define qglStencilOp glStencilOp
#define qglTexImage2D glTexImage2D
#define qglTexParameteri glTexParameteri
#define qglTexSubImage2D glTexSubImage2D
#define qglViewport glViewport
#define qglClearDepthf glClearDepthf
#define qglDepthRangef glDepthRangef
#define qglGetShaderPrecisionFormat glGetShaderPrecisionFormat
#define qglActiveTexture glActiveTexture
#define qglDrawRangeElements glDrawRangeElements
#define qglBindBuffer glBindBuffer
#define qglDeleteBuffers glDeleteBuffers
#define qglGenBuffers glGenBuffers
#define qglBufferData glBufferData
#define qglBufferSubData glBufferSubData
#define qglDeleteProgram glDeleteProgram
#define qglDeleteShader glDeleteShader
#define qglDetachShader glDetachShader
#define qglCreateShader glCreateShader
#define qglShaderSource glShaderSource
#define qglCompileShader glCompileShader
#define qglCreateProgram glCreateProgram
#define qglAttachShader glAttachShader
#define qglLinkProgram glLinkProgram
#define qglUseProgram glUseProgram
#define qglValidateProgram glValidateProgram
#define qglUniform1f glUniform1f
#define qglUniform2f glUniform2f
#define qglUniform3f glUniform3f
#define qglUniform4f glUniform4f
#define qglUniform1i glUniform1i
#define qglUniform2i glUniform2i
#define qglUniform3i glUniform3i
#define qglUniform4i glUniform4i
#define qglUniform1fv glUniform1fv
#define qglUniform2fv glUniform2fv
#define qglUniform3fv glUniform3fv
#define qglUniform4fv glUniform4fv
#define qglUniform1iv glUniform1iv
#define qglUniform2iv glUniform2iv
#define qglUniform3iv glUniform3iv
#define qglUniform4iv glUniform4iv
#define qglUniformMatrix2fv glUniformMatrix2fv
#define qglUniformMatrix3fv glUniformMatrix3fv
#define qglUniformMatrix4fv glUniformMatrix4fv
#define qglGetProgramiv glGetProgramiv
#define qglGetShaderiv glGetShaderiv
#define qglGetProgramInfoLog glGetProgramInfoLog
#define qglGetShaderInfoLog glGetShaderInfoLog
#define qglGetAttachedShaders glGetAttachedShaders
#define qglGetUniformLocation glGetUniformLocation
#define qglGetActiveUniform glGetActiveUniform
#define qglGetUniformfv glGetUniformfv
#define qglGetUniformiv glGetUniformiv
#define qglGetShaderSource glGetShaderSource
#define qglVertexAttribPointer glVertexAttribPointer
#define qglEnableVertexAttribArray glEnableVertexAttribArray
#define qglDisableVertexAttribArray glDisableVertexAttribArray
#define qglBindAttribLocation glBindAttribLocation
#define qglGetActiveAttrib glGetActiveAttrib
#define qglGetAttribLocation glGetAttribLocation
#define qglDrawArraysInstanced glDrawArraysInstanced
#define qglDrawElementsInstanced glDrawElementsInstanced
#define qglVertexAttribDivisor glVertexAttribDivisor
#define qglIsRenderbuffer glIsRenderbuffer
#define qglBindRenderbuffer glBindRenderbuffer
#define qglDeleteRenderbuffers glDeleteRenderbuffers
#define qglGenRenderbuffers glGenRenderbuffers
#define qglRenderbufferStorage glRenderbufferStorage
#define qglGetRenderbufferParameteriv glGetRenderbufferParameteriv
#define qglIsFramebuffer glIsFramebuffer
#define qglBindFramebuffer glBindFramebuffer
#define qglDeleteFramebuffers glDeleteFramebuffers
#define qglGenFramebuffers glGenFramebuffers
#define qglCheckFramebufferStatus glCheckFramebufferStatus
#define qglFramebufferTexture2D glFramebufferTexture2D
#define qglFramebufferRenderbuffer glFramebufferRenderbuffer
#define qglGetFramebufferAttachmentParameteriv glGetFramebufferAttachmentParameteriv
#define qglGenerateMipmap glGenerateMipmap
#define qglBlitFramebuffer glBlitFramebuffer
#define qglProgramParameteri glProgramParameteri
#define qglGetProgramBinary glGetProgramBinary
#define qglProgramBinary glProgramBinary
#define qglCompressedTexImage2D glCompressedTexImage2D
#define qglCompressedTexSubImage2D glCompressedTexSubImage2D
#define qglBlendFuncSeparate glBlendFuncSeparate
#define qglTexImage3D glTexImage3D
#define qglTexSubImage3D glTexSubImage3D

#endif /* __EMSCRIPTEN__ || __WASM__ */

#endif
