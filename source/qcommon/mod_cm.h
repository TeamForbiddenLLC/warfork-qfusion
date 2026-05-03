#ifndef R_CM_MODULE_H
#define R_CM_MODULE_H

#include "../gameshared/q_arch.h"
#include "../gameshared/q_shared.h"
#include "../gameshared/q_math.h"
#include "../gameshared/q_collision.h"
#include "qfiles.h"

typedef struct cmodel_state_s cmodel_state_t;
struct cmodel_s;

#define DECLARE_TYPEDEF_METHOD( ret, name, ... ) \
	typedef ret(*name##Fn )( __VA_ARGS__ ); \
	ret name (__VA_ARGS__);

// map / inline models
DECLARE_TYPEDEF_METHOD( struct cmodel_s *, CM_LoadMap, cmodel_state_t *cms, const char *name, bool clientload, unsigned *checksum );
DECLARE_TYPEDEF_METHOD( struct cmodel_s *, CM_InlineModel, cmodel_state_t *cms, int num );
DECLARE_TYPEDEF_METHOD( char *, CM_LoadMapMessage, char *name, char *message, int size );

// PVS / PHS data
DECLARE_TYPEDEF_METHOD( dvis_t *, CM_PVSData, cmodel_state_t *cms );
DECLARE_TYPEDEF_METHOD( dvis_t *, CM_PHSData, cmodel_state_t *cms );

// counts / strings
DECLARE_TYPEDEF_METHOD( int, CM_NumClusters, cmodel_state_t *cms );
DECLARE_TYPEDEF_METHOD( int, CM_NumAreas, cmodel_state_t *cms );
DECLARE_TYPEDEF_METHOD( int, CM_NumInlineModels, cmodel_state_t *cms );
DECLARE_TYPEDEF_METHOD( char *, CM_EntityString, cmodel_state_t *cms );
DECLARE_TYPEDEF_METHOD( int, CM_EntityStringLen, cmodel_state_t *cms );
DECLARE_TYPEDEF_METHOD( const char *, CM_ShaderrefName, cmodel_state_t *cms, int ref );

// bbox hulls
DECLARE_TYPEDEF_METHOD( struct cmodel_s *, CM_ModelForBBox, cmodel_state_t *cms, vec3_t mins, vec3_t maxs );
DECLARE_TYPEDEF_METHOD( struct cmodel_s *, CM_OctagonModelForBBox, cmodel_state_t *cms, vec3_t mins, vec3_t maxs );
DECLARE_TYPEDEF_METHOD( void, CM_InlineModelBounds, cmodel_state_t *cms, struct cmodel_s *cmodel, vec3_t mins, vec3_t maxs );

// contents / traces
DECLARE_TYPEDEF_METHOD( int, CM_TransformedPointContents, cmodel_state_t *cms, vec3_t p, struct cmodel_s *cmodel, vec3_t origin, vec3_t angles );
DECLARE_TYPEDEF_METHOD( void, CM_TransformedBoxTrace, cmodel_state_t *cms, trace_t *tr, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, struct cmodel_s *cmodel, int brushmask, vec3_t origin, vec3_t angles );
DECLARE_TYPEDEF_METHOD( void, CM_RoundUpToHullSize, cmodel_state_t *cms, vec3_t mins, vec3_t maxs, struct cmodel_s *cmodel );

// leaves / clusters / areas
DECLARE_TYPEDEF_METHOD( int, CM_ClusterRowSize, cmodel_state_t *cms );
DECLARE_TYPEDEF_METHOD( int, CM_AreaRowSize, cmodel_state_t *cms );
DECLARE_TYPEDEF_METHOD( int, CM_PointLeafnum, cmodel_state_t *cms, const vec3_t p );
DECLARE_TYPEDEF_METHOD( int, CM_BoxLeafnums, cmodel_state_t *cms, vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode );
DECLARE_TYPEDEF_METHOD( int, CM_LeafCluster, cmodel_state_t *cms, int leafnum );
DECLARE_TYPEDEF_METHOD( int, CM_LeafArea, cmodel_state_t *cms, int leafnum );

// portals / area connectivity
DECLARE_TYPEDEF_METHOD( void, CM_SetAreaPortalState, cmodel_state_t *cms, int area1, int area2, bool open );
DECLARE_TYPEDEF_METHOD( bool, CM_AreasConnected, cmodel_state_t *cms, int area1, int area2 );
DECLARE_TYPEDEF_METHOD( int, CM_WriteAreaBits, cmodel_state_t *cms, uint8_t *buffer );
DECLARE_TYPEDEF_METHOD( void, CM_ReadAreaBits, cmodel_state_t *cms, uint8_t *buffer );
DECLARE_TYPEDEF_METHOD( bool, CM_HeadnodeVisible, cmodel_state_t *cms, int headnode, uint8_t *visbits );
DECLARE_TYPEDEF_METHOD( void, CM_WritePortalState, cmodel_state_t *cms, int file );
DECLARE_TYPEDEF_METHOD( void, CM_ReadPortalState, cmodel_state_t *cms, int file );

// vis merging
DECLARE_TYPEDEF_METHOD( void, CM_MergePVS, cmodel_state_t *cms, vec3_t org, uint8_t *out );
DECLARE_TYPEDEF_METHOD( void, CM_MergePHS, cmodel_state_t *cms, int cluster, uint8_t *out );
DECLARE_TYPEDEF_METHOD( int, CM_MergeVisSets, cmodel_state_t *cms, vec3_t org, uint8_t *pvs, uint8_t *areabits );
DECLARE_TYPEDEF_METHOD( bool, CM_InPVS, cmodel_state_t *cms, const vec3_t p1, const vec3_t p2 );

// state lifecycle
DECLARE_TYPEDEF_METHOD( cmodel_state_t *, CM_New, void *mempool );
DECLARE_TYPEDEF_METHOD( void, CM_AddReference, cmodel_state_t *cms );
DECLARE_TYPEDEF_METHOD( void, CM_ReleaseReference, cmodel_state_t *cms );

#undef DECLARE_TYPEDEF_METHOD

struct cm_import_s {
	CM_LoadMapFn CM_LoadMap;
	CM_InlineModelFn CM_InlineModel;
	CM_LoadMapMessageFn CM_LoadMapMessage;
	CM_PVSDataFn CM_PVSData;
	CM_PHSDataFn CM_PHSData;
	CM_NumClustersFn CM_NumClusters;
	CM_NumAreasFn CM_NumAreas;
	CM_NumInlineModelsFn CM_NumInlineModels;
	CM_EntityStringFn CM_EntityString;
	CM_EntityStringLenFn CM_EntityStringLen;
	CM_ShaderrefNameFn CM_ShaderrefName;
	CM_ModelForBBoxFn CM_ModelForBBox;
	CM_OctagonModelForBBoxFn CM_OctagonModelForBBox;
	CM_InlineModelBoundsFn CM_InlineModelBounds;
	CM_TransformedPointContentsFn CM_TransformedPointContents;
	CM_TransformedBoxTraceFn CM_TransformedBoxTrace;
	CM_RoundUpToHullSizeFn CM_RoundUpToHullSize;
	CM_ClusterRowSizeFn CM_ClusterRowSize;
	CM_AreaRowSizeFn CM_AreaRowSize;
	CM_PointLeafnumFn CM_PointLeafnum;
	CM_BoxLeafnumsFn CM_BoxLeafnums;
	CM_LeafClusterFn CM_LeafCluster;
	CM_LeafAreaFn CM_LeafArea;
	CM_SetAreaPortalStateFn CM_SetAreaPortalState;
	CM_AreasConnectedFn CM_AreasConnected;
	CM_WriteAreaBitsFn CM_WriteAreaBits;
	CM_ReadAreaBitsFn CM_ReadAreaBits;
	CM_HeadnodeVisibleFn CM_HeadnodeVisible;
	CM_WritePortalStateFn CM_WritePortalState;
	CM_ReadPortalStateFn CM_ReadPortalState;
	CM_MergePVSFn CM_MergePVS;
	CM_MergePHSFn CM_MergePHS;
	CM_MergeVisSetsFn CM_MergeVisSets;
	CM_InPVSFn CM_InPVS;
	CM_NewFn CM_New;
	CM_AddReferenceFn CM_AddReference;
	CM_ReleaseReferenceFn CM_ReleaseReference;
};

#if CM_DEFINE_INTERFACE_IMPL
static struct cm_import_s cm_import;

struct cmodel_s *CM_LoadMap( cmodel_state_t *cms, const char *name, bool clientload, unsigned *checksum ) { return cm_import.CM_LoadMap( cms, name, clientload, checksum ); }
struct cmodel_s *CM_InlineModel( cmodel_state_t *cms, int num ) { return cm_import.CM_InlineModel( cms, num ); }
char *CM_LoadMapMessage( char *name, char *message, int size ) { return cm_import.CM_LoadMapMessage( name, message, size ); }
dvis_t *CM_PVSData( cmodel_state_t *cms ) { return cm_import.CM_PVSData( cms ); }
dvis_t *CM_PHSData( cmodel_state_t *cms ) { return cm_import.CM_PHSData( cms ); }
int CM_NumClusters( cmodel_state_t *cms ) { return cm_import.CM_NumClusters( cms ); }
int CM_NumAreas( cmodel_state_t *cms ) { return cm_import.CM_NumAreas( cms ); }
int CM_NumInlineModels( cmodel_state_t *cms ) { return cm_import.CM_NumInlineModels( cms ); }
char *CM_EntityString( cmodel_state_t *cms ) { return cm_import.CM_EntityString( cms ); }
int CM_EntityStringLen( cmodel_state_t *cms ) { return cm_import.CM_EntityStringLen( cms ); }
const char *CM_ShaderrefName( cmodel_state_t *cms, int ref ) { return cm_import.CM_ShaderrefName( cms, ref ); }
struct cmodel_s *CM_ModelForBBox( cmodel_state_t *cms, vec3_t mins, vec3_t maxs ) { return cm_import.CM_ModelForBBox( cms, mins, maxs ); }
struct cmodel_s *CM_OctagonModelForBBox( cmodel_state_t *cms, vec3_t mins, vec3_t maxs ) { return cm_import.CM_OctagonModelForBBox( cms, mins, maxs ); }
void CM_InlineModelBounds( cmodel_state_t *cms, struct cmodel_s *cmodel, vec3_t mins, vec3_t maxs ) { cm_import.CM_InlineModelBounds( cms, cmodel, mins, maxs ); }
int CM_TransformedPointContents( cmodel_state_t *cms, vec3_t p, struct cmodel_s *cmodel, vec3_t origin, vec3_t angles ) { return cm_import.CM_TransformedPointContents( cms, p, cmodel, origin, angles ); }
void CM_TransformedBoxTrace( cmodel_state_t *cms, trace_t *tr, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, struct cmodel_s *cmodel, int brushmask, vec3_t origin, vec3_t angles ) { cm_import.CM_TransformedBoxTrace( cms, tr, start, end, mins, maxs, cmodel, brushmask, origin, angles ); }
void CM_RoundUpToHullSize( cmodel_state_t *cms, vec3_t mins, vec3_t maxs, struct cmodel_s *cmodel ) { cm_import.CM_RoundUpToHullSize( cms, mins, maxs, cmodel ); }
int CM_ClusterRowSize( cmodel_state_t *cms ) { return cm_import.CM_ClusterRowSize( cms ); }
int CM_AreaRowSize( cmodel_state_t *cms ) { return cm_import.CM_AreaRowSize( cms ); }
int CM_PointLeafnum( cmodel_state_t *cms, const vec3_t p ) { return cm_import.CM_PointLeafnum( cms, p ); }
int CM_BoxLeafnums( cmodel_state_t *cms, vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode ) { return cm_import.CM_BoxLeafnums( cms, mins, maxs, list, listsize, topnode ); }
int CM_LeafCluster( cmodel_state_t *cms, int leafnum ) { return cm_import.CM_LeafCluster( cms, leafnum ); }
int CM_LeafArea( cmodel_state_t *cms, int leafnum ) { return cm_import.CM_LeafArea( cms, leafnum ); }
void CM_SetAreaPortalState( cmodel_state_t *cms, int area1, int area2, bool open ) { cm_import.CM_SetAreaPortalState( cms, area1, area2, open ); }
bool CM_AreasConnected( cmodel_state_t *cms, int area1, int area2 ) { return cm_import.CM_AreasConnected( cms, area1, area2 ); }
int CM_WriteAreaBits( cmodel_state_t *cms, uint8_t *buffer ) { return cm_import.CM_WriteAreaBits( cms, buffer ); }
void CM_ReadAreaBits( cmodel_state_t *cms, uint8_t *buffer ) { cm_import.CM_ReadAreaBits( cms, buffer ); }
bool CM_HeadnodeVisible( cmodel_state_t *cms, int headnode, uint8_t *visbits ) { return cm_import.CM_HeadnodeVisible( cms, headnode, visbits ); }
void CM_WritePortalState( cmodel_state_t *cms, int file ) { cm_import.CM_WritePortalState( cms, file ); }
void CM_ReadPortalState( cmodel_state_t *cms, int file ) { cm_import.CM_ReadPortalState( cms, file ); }
void CM_MergePVS( cmodel_state_t *cms, vec3_t org, uint8_t *out ) { cm_import.CM_MergePVS( cms, org, out ); }
void CM_MergePHS( cmodel_state_t *cms, int cluster, uint8_t *out ) { cm_import.CM_MergePHS( cms, cluster, out ); }
int CM_MergeVisSets( cmodel_state_t *cms, vec3_t org, uint8_t *pvs, uint8_t *areabits ) { return cm_import.CM_MergeVisSets( cms, org, pvs, areabits ); }
bool CM_InPVS( cmodel_state_t *cms, const vec3_t p1, const vec3_t p2 ) { return cm_import.CM_InPVS( cms, p1, p2 ); }
cmodel_state_t *CM_New( void *mempool ) { return cm_import.CM_New( mempool ); }
void CM_AddReference( cmodel_state_t *cms ) { cm_import.CM_AddReference( cms ); }
void CM_ReleaseReference( cmodel_state_t *cms ) { cm_import.CM_ReleaseReference( cms ); }

static inline void Q_ImportCmModule( struct cm_import_s *mod ) {
	cm_import = *mod;
}

#endif

#endif
