#include "r_local.h"

#ifndef R_CAPTURE_LOCAL_H
#define R_CAPTURE_LOCAL_H

void R_SaveScreenshotBuffer(struct texture_buf_s *pic, const char* path, int image_type );

// Records the backbuffer readback for a pending screenshot into the frame's primary command buffer.
// Call between vkCmdEndRendering and EndRICmd. Returns true if it recorded the copy, in which case it
// has also transitioned the swapchain image to PRESENT_SRC itself and the caller must not emit its own
// present barrier. Returns false when no screenshot is pending (the common case) or the capture was
// abandoned, leaving the transition to the caller.
bool R_CaptureRecordScreenshot( struct RICmd_s *cmd );

// Maps the completed readback and writes it to disk. Only valid once the frame timeline has reached
// rsh.screenshot.single.frameCnt. Always clears the capture state.
void R_CaptureFinishScreenshot( void );

#endif
