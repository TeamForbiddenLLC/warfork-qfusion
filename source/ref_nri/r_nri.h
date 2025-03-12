#ifndef R_NRI_IMP_H
#define R_NRI_IMP_H

#include "r_defines.h"

#include "r_texture_format.h"
#include "r_vattribs.h"

#include "r_graphics.h"

// a wrapper to hold onto the hash + cookie
struct nri_descriptor_s {
	//NriDescriptor *descriptor;
	uint32_t cookie;
};
#endif
