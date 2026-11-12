#ifndef MOON_DINOSAUR_SKULL_MODEL_H
#define MOON_DINOSAUR_SKULL_MODEL_H
/* Capture the SDK's actual Unity allocations and teardown requests in the
 * provider pool. Components belong to their tracked GameObjects; upload arrays
 * and borrowed shader/font objects retain the SDK's ordinary temporary roots. */
#define su_new fb_new
#define su_destroy fb_destroy
#include "../../runtime/sdk/scene/static_model_pbr.h"
#undef su_destroy
#undef su_new
#include "assets/generated/model_data.h"
static SMResources dbm_resources;
static PTR dbm_attach(PTR parent){
    unsigned group=fbo.group;if(fb_pending(7))return 0;fbo.group=7;
    int ready=sm_prepare_pbr(&dbm_model,&dbm_resources,dbm_pbr,1);fbo.group=group;
    if(!ready){fb_retire(7);return 0;}
    return sm_attach(&dbm_model,&dbm_resources,parent,2,"Supplied PBR raptor skull");
}
#endif
