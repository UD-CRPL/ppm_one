#define BENCHMARK

#define ENABLE_ACC_COMPARE
#define ENABLE_PGI_COMPARE

#ifdef USE_NVTX
#include "nvToolsExt.h"

const uint32_t ProfilerColors[] = { 0xff00ff00, 0xff0000ff, 0xffffff00, 0xffff00ff, 0xff00ffff, 0xffff0000, 0xffffffff };

#define NVPROF_PUSH_RANGE(name,cid) { \
    int color_id = cid; \
    nvtxEventAttributes_t eventAttrib = {0}; \
    eventAttrib.version = NVTX_VERSION; \
    eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE; \
    eventAttrib.colorType = NVTX_COLOR_ARGB; \
    eventAttrib.color = ProfilerColors[color_id]; \
    eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII; \
    eventAttrib.message.ascii = name; \
    nvtxRangePushEx(&eventAttrib); \
}
#define NVPROF_POP_RANGE nvtxRangePop();
#else
#define NVPROF_PUSH_RANGE(name,cid)
#define NVPROF_POP_RANGE
#endif

#if defined(ENABLE_PGI_COMPARE) && defined(USE_PGI_COMPARE) && defined(_OPENACC) && defined(__PGI)
#include <openacc.h>
#define PGI_COMPARE(ptr,type,size,name,file,func,line) {\
    type* pgi_compare_ptr = (type*)ptr;\
    pgi_compare(ptr, "type", size, \
    name, file, func, line);\
}
#else
#define PGI_COMPARE(ptr,size,name,file,func,line)
#endif

#if defined(ENABLE_ACC_COMPARE) && defined(USE_ACC_COMPARE) && defined(_OPENACC) && defined(__PGI)
#include <openacc.h>
#define ACC_COMPARE(ptr,type,size) {\
    type* acc_compare_ptr = (type*) ptr;\
    acc_compare(acc_compare_ptr, size);\
}
#else
#define ACC_COMPARE(ptr,type,size)
#endif

