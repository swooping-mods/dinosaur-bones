/* Production save/load paths with isolated IO and world/authority boundaries. */
#include <assert.h>
#include <string.h>
typedef void *PTR;
typedef unsigned long long U64;
static int world_object, walker_object, other_world;
static struct {PTR world,walker,world_field,joined,host;int initial_joined,suspended;U64 host_id;char save[181];} fb;
static struct {unsigned found,loaded,dirty;int save_ok;} db;
static struct {int last_error;} ss;
#define SS_NOT_FOUND 2
static PTR current;
static int authority,alive,read_success,write_success,writes,read_error,joined,invoke_ok;
static U64 host;
static unsigned payload[2],payload_size;
static int fb_host(void){return authority&&!fb.suspended;}
static PTR su_get(PTR p){return p;}
static int su_alive(PTR p){return p&&alive;}
static void field_value(PTR p,PTR f,void *out){(void)p;(void)f;*(PTR *)out=current;}
static int boolean(PTR m,PTR p,int *ok){(void)m;(void)p;*ok=invoke_ok;return joined;}
static U64 fb_id(PTR m,int *ok){(void)m;*ok=invoke_ok;return host;}
static void db_event(const char *e){(void)e;}
static int ss_save(const char *root,const unsigned char *bytes,unsigned size){
    (void)root;assert(size==8);const unsigned *p=(const unsigned *)bytes;
    assert(p[0]==0x314e4244u&&p[1]==db.found);writes++;return write_success;
}
static int ss_load(const char *root,unsigned char *bytes,unsigned capacity,unsigned *size){
    (void)root;assert(capacity==8);ss.last_error=read_error;
    if(!read_success)return 0;memcpy(bytes,payload,8);*size=payload_size;return 1;
}
#include "bones-persistence-runtime.h"
static void reset(void){
    memset(&fb,0,sizeof(fb));memset(&db,0,sizeof(db));
    fb.world=&world_object;fb.walker=&walker_object;current=fb.world;
    authority=alive=invoke_ok=write_success=1;writes=joined=0;host=0;
    read_success=1;read_error=0;payload_size=8;payload[0]=0x314e4244u;payload[1]=5;
}
int main(void){
    reset();db_load();assert(db.loaded&&db.save_ok&&db.found==5);
    reset();read_success=0;read_error=SS_NOT_FOUND;db_load();assert(db.save_ok&&!db.found);
    reset();read_success=0;read_error=5;db_load();assert(!db.save_ok&&!db.found);
    reset();payload[0]=0;db_load();assert(!db.save_ok);
    reset();payload[1]=16;db_load();assert(!db.save_ok);
    reset();payload_size=4;db_load();assert(!db.save_ok);
    reset();authority=0;db_load();assert(!db.found);db.dirty=1;assert(!db_save()&&!writes);
    reset();db_load();db.dirty=1;write_success=0;
    assert(!db_save()&&db.dirty&&writes==1);write_success=1;
    assert(db_save()&&!db.dirty&&writes==2);assert(!db_save()&&writes==2);
    reset();db.save_ok=db.dirty=1;current=&other_world;assert(!db_save()&&!writes&&db.dirty);
    current=fb.world;alive=0;assert(!db_save()&&!writes);
    reset();db.save_ok=db.dirty=1;joined=1;assert(!db_save()&&fb.suspended&&!writes);
    reset();db.save_ok=db.dirty=1;fb.initial_joined=joined=1;fb.host_id=7;host=8;
    assert(!db_save()&&fb.suspended&&!writes);
    reset();db.save_ok=db.dirty=1;invoke_ok=0;assert(!db_save()&&fb.suspended&&!writes);
    return 0;
}
