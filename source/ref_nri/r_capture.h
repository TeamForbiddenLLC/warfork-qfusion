#include "r_local.h"

#ifndef R_CAPTURE_LOCAL_H
#define R_CAPTURE_LOCAL_H

void R_SaveScreenshotBuffer(struct texture_buf_s *pic, const char* path, int image_type );

// Records the back-buffer readback for a pending screenshot into the frame's primary command buffer.
// Call between vkCmdEndRendering and EndRICmd, with `src` (the mode-res back buffer) already in
// RI_RESOURCE_STATE_COPY_SRC; the image is left in that state, so the caller owns every layout
// transition. Returns true if it recorded the copy, false when no screenshot is pending (the common
// case) or the capture was abandoned.
bool R_CaptureRecordScreenshot( struct RICmd_s *cmd, const struct RITexture_s *src, uint32_t width, uint32_t height );

// Maps the completed readback and writes it to disk. Only valid once the frame timeline has reached
// rsh.screenshot.single.frameCnt. Always clears the capture state.
void R_CaptureFinishScreenshot( void );

#endif
