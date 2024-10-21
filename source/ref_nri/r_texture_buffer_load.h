
#include "r_local.h"
#include "r_texture_buf.h"
#include "r_texture_format.h"


// loads with STBI library png/jpg/tga
bool T_LoadImageSTBI(char *filename, struct texture_buf_s* buffer );
// loads the PCX file with a buffer and pallet. this is aliased in the texture buffer 
bool T_LoadImagePCX(char *filename, struct texture_buf_s* buffer, uint8_t** pallet);
bool T_LoadImageWAL(char *filename, struct texture_buf_s* buffer);

void T_SetPallet(uint32_t p[256]);
uint32_t* T_Pallet();