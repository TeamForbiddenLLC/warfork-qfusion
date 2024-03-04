
#include "file_format.h"

#include "./q_shared.h"
#include "bsp_format.h"
#include "q_arch.h"


bool FF_GuessFormat( const char *buffer, size_t len, struct header_format_def_s *def)
{
	memset( def, 0, sizeof( struct header_format_def_s ) );
	if( !strncmp( buffer, IDMD3HEADER_MAGIC, strlen( IDMD3HEADER_MAGIC ) ) ) {
		def->format = FILE_IDMD3;
		def->flags = FILE_FLAG_MESH;
		return true;
	}
	if( !strncmp( buffer, IQMHEADER_MAGIC, strlen( IQMHEADER_MAGIC ) ) ) {
		def->format = FILE_IQM;
		def->flags = FILE_FLAG_SKINNED_MESH;
		return true;
	}
	{
		const struct q3q2bsp_header_s *q3 = (struct q3q2bsp_header_s *)buffer;
		static const struct {
			const char* magic;
			enum header_format_type_e format;
			int version;
			size_t versionOffset;
		} test[] = {
			{IDBSPHEADER_MAGIC, FILE_QUAKE_IBSP_V38,  Q2_VERSION_IBSP_V38, offsetof(struct q3q2bsp_header_s, version) },
			{IDBSPHEADER_MAGIC, FILE_QUAKE_IBSP_V46,  Q3_VERSION_IBSP_V46, offsetof(struct q3q2bsp_header_s, version) },
			{IDBSPHEADER_MAGIC, FILE_QUAKE_IBSP_V47,  Q3_VERSION_IBSP_V47, offsetof(struct q3q2bsp_header_s, version) },
			{RBSPHEADER_MAGIC,  FILE_QUAKE_RBSP_V1,   Q3_VERSION_RBSP_V1, offsetof(struct q3q2bsp_header_s, version) },
			{QFBSPHEADER_MAGIC, FILE_QUAKE_FBSP_V1,   Q3_VERSION_FBSP_V1 ,offsetof(struct q3q2bsp_header_s, version)},
			{"",                FILE_QUAKE_BSP_V29,   Q1_VERSION_V29 , offsetof(struct q1bsp_header_s, version) },
		};
		for( size_t i = 0; i < Q_ARRAY_COUNT( test ); i++ ) {
			const int version = LittleLong( *( (int *)( q3->version + test[i].versionOffset ) ) );
			if( test[i].version == version ) {
				if(q3->magic[0] == '\0' || 
					!strncmp( q3->magic, test[i].magic, strlen( test[i].magic ) )
				) {
					def->format = test[i].format;
					def->flags = FILE_FLAG_BSP;
					return true;
				}
			}
		}
	}
	return false;
}
