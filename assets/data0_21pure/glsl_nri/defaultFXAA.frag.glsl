/**
 * @license
 * Copyright (c) 2011 NVIDIA Corporation. All rights reserved.
 *
 * TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
 * *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
 * OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, NONINFRINGEMENT,IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA 
 * OR ITS SUPPLIERS BE  LIABLE  FOR  ANY  DIRECT, SPECIAL,  INCIDENTAL,  INDIRECT,  OR  
 * CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS 
 * OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY 
 * OTHER PECUNIARY LOSS) ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, 
 * EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 */
//https://github.com/libretro/glsl-shaders/blob/master/anti-aliasing/shaders/fxaa.glsl

layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform texture2D u_BaseTexture;
layout(set = 0, binding = 1) uniform sampler u_BaseSampler;


layout( push_constant ) uniform ConstBlock {
  vec2 screenSize;
} push;

#define FXAA_PC 1
#define FXAA_GLSL_130 1
#define FXAA_GREEN_AS_LUMA 1
#define FXAA_QUALITY_X_PRESET 23
#define FXAA_GATHER4_ALPHA 1
#include "include/Fxaa3_11.h"

void main(void)
{
    // Only used on FXAA Quality.
    // Choose the amount of sub-pixel aliasing removal.
    // This can effect sharpness.
    //   1.00 - upper limit (softer)
    //   0.75 - default amount of filtering
    //   0.50 - lower limit (sharper, less sub-pixel aliasing removal)
    //   0.25 - almost off
    //   0.00 - completely off
    float QualitySubpix = 0.75;

    // The minimum amount of local contrast required to apply algorithm.
    //   0.333 - too little (faster)
    //   0.250 - low quality
    //   0.166 - default
    //   0.125 - high quality 
    //   0.033 - very high quality (slower)
    float QualityEdgeThreshold = 0.166;
    float QualityEdgeThresholdMin = 0.0;

    vec4 ConsolePosPos = vec4(0.0,0.0,0.0,0.0);
    vec4 ConsoleRcpFrameOpt = vec4(0.0,0.0,0.0,0.0);
    vec4 ConsoleRcpFrameOpt2 = vec4(0.0,0.0,0.0,0.0);
    vec4 Console360RcpFrameOpt2 = vec4(0.0,0.0,0.0,0.0);
    float ConsoleEdgeSharpness = 8.0;
    float ConsoleEdgeThreshold = 0.125;
    float ConsoleEdgeThresholdMin = 0.05;
    vec4  Console360ConstDir = vec4(1.0, -1.0, 0.25, -0.25);

    outFragColor = FxaaPixelShader(v_TexCoord, ConsolePosPos, u_BaseTexture, u_BaseTexture, u_BaseTexture, 
        vec2(1.0)/push.screenSize, ConsoleRcpFrameOpt, ConsoleRcpFrameOpt2, Console360RcpFrameOpt2, 
        QualitySubpix, QualityEdgeThreshold, QualityEdgeThresholdMin, ConsoleEdgeSharpness, 
        ConsoleEdgeThreshold, ConsoleEdgeThresholdMin, Console360ConstDir);

}
