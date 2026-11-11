#ifndef MOON_DINOSAUR_SKULL_MODEL_H
#define MOON_DINOSAUR_SKULL_MODEL_H
#include "../../runtime/sdk/scene/static_model_pbr.h"
#include "assets/generated/model_data.h"
static SMResources dbm_resources;
static PTR dbm_attach(PTR parent){
    if(!sm_prepare_pbr(&dbm_model,&dbm_resources,dbm_pbr,1))return 0;
    return sm_attach(&dbm_model,&dbm_resources,parent,2,"Supplied PBR raptor skull");
}
#endif
