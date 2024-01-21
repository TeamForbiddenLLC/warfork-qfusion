#ifndef R_NRI_IMP_H
#define R_NRI_IMP_H

#define NRI_STATIC_LIBRARY 1
#include "NRI.h"

#include "Extensions/NRIDeviceCreation.h"
#include "Extensions/NRIHelper.h"
#include "Extensions/NRIMeshShader.h"
#include "Extensions/NRIRayTracing.h"
#include "Extensions/NRISwapChain.h"

typedef struct {
} nri_desc_t;

#define NRI_ABORT_ON_FAILURE(result) \
    if (result != NriResult_SUCCESS) { exit(0);}

static const char* NriResultToString[NriResult_MAX_NUM] = {
    [NriResult_SUCCESS]= "SUCESS",
    [NriResult_FAILURE]= "FAILURE",
    [NriResult_INVALID_ARGUMENT]= "INVALID_ARGUMENT",
    [NriResult_OUT_OF_MEMORY]= "OUT_OF_MEMORY",
    [NriResult_UNSUPPORTED]= "UNSUPPORTED",
    [NriResult_DEVICE_LOST] = "DEVICE_LOST",
    [NriResult_OUT_OF_DATE] = "OUT_OF_DATE"
};

typedef struct {
		NriHelperInterface helperI;
		NriCoreInterface coreI;
		NriSwapChainInterface swapChainI;
		NriDevice *device;
} nri_t;

extern nri_t r_nri;

bool NRIimp_Init(nri_desc_t* desc);

#endif
