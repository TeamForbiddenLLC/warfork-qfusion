#include "bsp_header.h"
#include "../gameshared/q_shared.h"

struct bsp_format_def_s guessFormatDef(const char* buffer, size_t len) {
	struct bsp_format_def_s result = {};
  struct q3bsp_header_s* q3 = (struct q3bsp_header_s*)buffer; 
  if(!strncmp(q3->magic, Q3_IDBSPHEADER, sizeof(q3->magic))) {
		const int version = LittleLong( q3->version);
		switch(version) {
		  case Q3_BSPVERSION: 
		  case Q3_RTCWBSPVERSION:
		    result.type = BSP_VERSION_3;
		    result.v3.lightmapWidth = Q3_LIGHTMAP_WIDTH;
		    result.v3.lightmapHeight = Q3_LIGHTMAP_HEIGHT;
		    result.v3.flags = BSP_3_NONE; 
		    return result; 
		}
  }
  if(!strncmp(q3->magic, Q3_RBSPHEADER, sizeof(q3->magic))) {
		const int version = LittleLong( q3->version);

  }
  if(!strncmp(q3->magic, Q3_QFBSPHEADER, sizeof(q3->magic))) {
		const int version = LittleLong( q3->version);

  }

  struct bsp_format_def_s def = {

  };

  return def;
}
