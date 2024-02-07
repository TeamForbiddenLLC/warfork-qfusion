#include "../gameshared/q_arch.h"
#include "r_texture_format.h"

struct ktx_image_s {
	// updates internal data for consistency 
	// - swaps endianness for platform
	bool update;
	
	uint16_t width;
	uint16_t height;
	uint32_t rowPitch; 

	// this is the start of the data
	size_t byteSize;
	size_t byteOffset; // ktx_context_data_s::data + byteOffset
};

struct ktx_context_s {
	bool swapEndianess;
	int type;
	int typeSize;
	int format;
	int internalFormat;
	int baseInternalFormat;
	int pixelWidth;
	int pixelHeight;
	int pixelDepth;
	int numberOfArrayElements;
	int numberOfFaces;
	int numberOfMipmapLevels;
	int bytesOfKeyValueData;

	enum texture_format_e textureFormat; 
	uint8_t* data;
	struct ktx_image_s* dataContext;
};

