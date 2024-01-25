#ifdef CGAMEGETLIGHTORIGIN
#define SHADOW_MAPPING			2
#else
#define SHADOW_MAPPING			1
#endif

#define SUBDIVISIONS_MIN		3
#define SUBDIVISIONS_MAX		16
#define SUBDIVISIONS_DEFAULT	5

#define MAX_PORTAL_SURFACES		32
#define MAX_PORTAL_TEXTURES		64

#define BACKFACE_EPSILON		4

#define	ON_EPSILON				0.1         // point on plane side epsilon

#define Z_NEAR					4.0f
#define Z_BIAS					64.0f

#define	SIDE_FRONT				0
#define	SIDE_BACK				1
#define	SIDE_ON					2

#define RF_BIT(x)				(1ULL << (x))

#define RF_NONE					0x0
#define RF_MIRRORVIEW			RF_BIT(0)
#define RF_PORTALVIEW			RF_BIT(1)
#define RF_ENVVIEW				RF_BIT(2)
#define RF_SHADOWMAPVIEW		RF_BIT(3)
#define RF_FLIPFRONTFACE		RF_BIT(4)
#define RF_DRAWFLAT				RF_BIT(5)
#define RF_CLIPPLANE			RF_BIT(6)
#define RF_NOVIS				RF_BIT(7)
#define RF_LIGHTMAP				RF_BIT(8)
#define RF_SOFT_PARTICLES		RF_BIT(9)
#define RF_PORTAL_CAPTURE		RF_BIT(10)

#define RF_CUBEMAPVIEW			( RF_ENVVIEW )
#define RF_NONVIEWERREF			( RF_PORTALVIEW|RF_MIRRORVIEW|RF_ENVVIEW|RF_SHADOWMAPVIEW )

#define MAX_REF_SCENES			32 // max scenes rendered per frame
#define MAX_REF_ENTITIES		( MAX_ENTITIES + 48 ) // must not exceed 2048 because of sort key packing
