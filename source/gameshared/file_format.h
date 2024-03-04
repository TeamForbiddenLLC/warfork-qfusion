#ifndef R_FILE_FORMAT_H
#define R_FILE_FORMAT_H

#include "bsp_format.h"

// magic numbers
#define IDBSPHEADER_MAGIC	  "IBSP"
#define RBSPHEADER_MAGIC	  "RBSP"
#define QFBSPHEADER_MAGIC	  "FBSP"

#define IDMD3HEADER_MAGIC		"IDP3" // MD3 model file
#define IQMHEADER_MAGIC     "INTERQUAKEMODEL"

enum header_format_type_e {
  FILE_UNKNOWN,

	// bsp formats
  FILE_QUAKE_BSP_V29, // does not have a magic number
	FILE_QUAKE_IBSP_V38,
	FILE_QUAKE_IBSP_V46,
	FILE_QUAKE_IBSP_V47,
	FILE_QUAKE_RBSP_V1,
	FILE_QUAKE_FBSP_V1,
	
	// quake mesh formats
	FILE_IDMD3, // md3
	FILE_IQM // internal quake
};

// flags for the type of file
enum format_feature_flags_e {
  FILE_FLAG_MESH = 0x1,
  FILE_FLAG_SKINNED_MESH = 0x2,
  FILE_FLAG_BSP = 0x4
};

struct header_format_def_s {
	enum header_format_type_e format;
	enum format_feature_flags_e flags;
};

enum format_error_type_e {
	UNKNOWN_FORMAT,
	FILE_UNEXPECTED_VERSION
};

struct format_error_s {
	enum format_error_type_e type;
	union {
		struct {
			enum header_format_type_e format;
			size_t numSupportedVersions;
			const int* supportedVersions;
			int expectedVersion;
		} unexpectedVersion;
	};
};

// guess the format from the header and also validate if it meet expectations
// validates if the file is a valid format supported by the engine 
bool FF_GuessFormat( const char *buffer, size_t len, struct header_format_def_s *def);

#endif
