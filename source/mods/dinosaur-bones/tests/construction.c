/* Actual provider setup/cleanup functions, with a deferred Unity-like adapter.
 * Fault sweeps are authored for explicit native execution, never a game claim. */
#define MOON_NATIVE_UNITY_H
#include <assert.h>
#include <string.h>
#include <stdio.h>
typedef unsigned long long U64;
typedef unsigned long long SURoot;
typedef unsigned DWORD;
typedef struct Object Object;
typedef Object *PTR;
typedef struct {float x,y,z;} SPVec3;
typedef struct {float x,y,z,w;} SUQuat;
typedef struct {float r,g,b,a;} SUColor;
struct Object {int alive,kind,active,enabled,destroy_requested;PTR parent;PTR items[192];union {unsigned char boolean;SUQuat quat;} value;};
static Object objects[2048],box;
static PTR handles[1024];
static unsigned used,handle_calls,fail_handle,call_count,fail_call,allocations,destroy_calls,disable_calls,new_calls,fail_new_at;
static int deferred,fail_destroy,fail_alive,fail_array,fail_new,fail_string,missing_method,null_unbox,null_pool_target,null_array_slot;
static PTR camera,foreign;
enum {M_CTOR=1,M_TRANSFORM,M_PARENT,M_POSITION,M_SCALE,M_ROTATION,M_LOOK,M_ALIVE,M_DESTROY,M_PRIMITIVE,M_COMPONENT,M_ADD,M_DISABLE,M_SHADER,M_MATERIAL,M_COLOR,M_TEXT,M_ACTIVE,M_FONT,M_OTHER,M_LOAD};
#define MP(n) ((PTR)(U64)(n))
static struct {
    PTR go_ctor,go_transform,tr_parent,tr_position,tr_local_position,tr_scale,tr_rotation,tr_local_rotation;
    PTR look_rotation,obj_alive,obj_destroy,primitive,go_component,go_add,collider_enabled,shader_find,material_ctor,material_color,text_text;
    PTR mesh_ctor,mesh_vertices,mesh_normals,mesh_uv,mesh_indices,mesh_bounds,material_float,material_keyword,meshfilter_set,renderer_materials;
    PTR byte_class,vector_class,uv_class,int_class,material_class;
    SURoot (*root)(PTR,int);PTR (*new_array)(PTR,U64),(*string)(const char *);
    PTR (*array_address)(PTR,int,U64);
} su;
static PTR allocate(int kind){assert(used+1<2048);PTR p=&objects[++used];memset(p,0,sizeof(*p));p->kind=kind;p->active=p->enabled=1;allocations++;return p;}
static PTR find_class(const char *ns,const char *name){(void)ns;(void)name;return MP(100);}
static PTR array_new(PTR klass,U64 count){(void)klass;assert(count==192);if(fail_array)return 0;return allocate(10);}
static PTR string_new(const char *text){assert(text);return fail_string?0:MP(101);}
static PTR su_array_address(PTR p,int stride,U64 index){if(null_array_slot)return 0;assert(p&&stride==(int)sizeof(PTR)&&index<192);return (PTR)(void *)&p->items[index];}
static void barrier(PTR owner,PTR *slot,PTR value){assert(owner&&slot>=owner->items&&slot<owner->items+192);*slot=value;}
static PTR su_new(const char *ns,const char *name){(void)ns;if(++new_calls==fail_new_at||fail_new)return 0;return allocate(strcmp(name,"GameObject")==0?1:2);}
static SURoot su_hold(PTR p){if(!p)return 0;if(++handle_calls==fail_handle)return 0;for(unsigned i=1;i<1024;i++)if(!handles[i]){handles[i]=p;return i;}assert(0);return 0;}
static SURoot root(PTR p,int pinned){assert(pinned);return su_hold(p);}
static PTR su_get(SURoot h){return h&&!(null_pool_target&&handles[h]&&handles[h]->kind==10)?handles[h]:0;}
static void su_drop(SURoot *h){if(*h){assert(handles[*h]);handles[*h]=0;*h=0;}}
static void su_copy(void *to,const void *from,U64 bytes){memcpy(to,from,(size_t)bytes);}
static void su_zero(void *to,U64 bytes){memset(to,0,(size_t)bytes);}
static int su_eq(const char *a,const char *b){return strcmp(a,b)==0;}
static SPVec3 su_v(float x,float y,float z){return (SPVec3){x,y,z};}
static PTR su_type(const char *ns,const char *name){(void)ns;(void)name;return MP(102);}
static PTR su_array(PTR klass,U64 count,const void *data,int stride){(void)klass;(void)count;(void)data;(void)stride;return allocate(10);}
static PTR call(PTR method,PTR self,void **args,int *ok);
static PTR su_add_component(PTR go,const char *name){(void)name;int ok=1;void *a[]={MP(102)};PTR p=call(su.go_add,go,a,&ok);return ok?p:0;}
static PTR su_method(const char *ns,const char *type,const char *name,const char *signature){
    (void)ns;(void)type;(void)signature;if(missing_method)return 0;
    if(strcmp(name,".ctor")==0)return MP(M_CTOR);
    if(strcmp(name,"LoadImage")==0)return MP(M_LOAD);
    if(strcmp(name,"SetActive")==0)return MP(M_ACTIVE);
    if(strcmp(name,"GetBuiltinResource")==0)return MP(M_FONT);
    return MP(M_OTHER);
}
static void kill(PTR p){if(!p)return;p->alive=0;for(unsigned i=1;i<=used;i++)if(objects[i].parent==p&&objects[i].alive)kill(&objects[i]);}
static void settle(void){for(unsigned i=1;i<=used;i++)if(objects[i].destroy_requested)kill(&objects[i]);}
static PTR call(PTR method,PTR self,void **args,int *ok){
    if(!method||++call_count==fail_call){*ok=0;return 0;}
    switch((unsigned)(U64)method){
    case M_CTOR:case M_MATERIAL:assert(self);self->alive=1;return 0;
    case M_TRANSFORM:assert(self);return self;
    case M_PARENT:assert(self&&args[0]);self->parent=args[0];return 0;
    case M_LOOK:box.value.quat=(SUQuat){0,0,0,1};return &box;
    case M_ALIVE:if(fail_alive){*ok=0;return 0;}box.value.boolean=(unsigned char)(args[0]&&((PTR)args[0])->alive);return &box;
    case M_DESTROY:destroy_calls++;if(fail_destroy){*ok=0;return 0;}assert(args[0]!=foreign);((PTR)args[0])->destroy_requested=1;if(!deferred)kill(args[0]);return 0;
    case M_PRIMITIVE:{PTR go=allocate(1);go->alive=1;return go;}
    case M_COMPONENT:case M_ADD:{assert(self);PTR p=allocate(3);p->alive=1;p->parent=self;return p;}
    case M_DISABLE:assert(self);self->enabled=*(unsigned char *)args[0];disable_calls++;return 0;
    case M_SHADER:case M_FONT:return camera;
    case M_LOAD:box.value.boolean=1;return &box;
    case M_ACTIVE:assert(self);self->active=*(unsigned char *)args[0];return 0;
    default:assert(self);return 0;
    }
}
static void *unbox(PTR p){return null_unbox?0:&p->value;}
static int su_alive(PTR p){int ok=1;void *a[]={p};PTR result=call(su.obj_alive,0,a,&ok);return ok&&result&&*(unsigned char *)unbox(result);}
/* Production ownership table and checked scene/material/widget helpers. */
#include "bones-construction-runtime.h"
static struct {SURoot root,label;SPVec3 point;int made;} db_sites[4];
static struct {unsigned found,loaded,dirty,ticks;int save_ok,journal;SURoot notebook,text;} db;
/* Production SDK wrappers and upload/attachment path, with a tiny original
 * triangle fixture in place of the proprietary-byte-free retained asset. */
#include "bones-model-runtime.h"
static const char *db_names[]={"A","B","C","D"},*db_notes[]={"a","b","c","d"};
static void db_event(const char *text){(void)text;}
static void field_value(PTR p,PTR f,void *out){(void)p;(void)f;*(PTR *)out=camera;}
static char *cat(char *p,const char *text){while(*text)*p++=*text++;*p=0;return p;}
static char *num(char *p,unsigned n){assert(n<=4);*p++=(char)('0'+n);*p=0;return p;}
static void db_site_clear(unsigned n);
#include "bones-scene-runtime.h"
static unsigned owned(unsigned group){unsigned n=0;for(unsigned i=0;i<192;i++)n+=fbo.owner[i]==group;return n;}
static unsigned live_handles(void){unsigned n=0;for(unsigned i=1;i<1024;i++)n+=handles[i]!=0;return n;}
static void retire_all(void){sm_release(&dbm_resources);fb_materials_drop();for(unsigned i=1;i<=7;i++)fb_retire(i);for(unsigned i=0;i<4;i++)db_site_clear(i);db_notebook_clear();}
static void drain(void){retire_all();fb_cleanup();settle();fb_cleanup();assert(live_handles()==1);for(unsigned i=1;i<=7;i++)assert(fb_empty(i));assert(foreign->alive&&camera->alive);}
static void reset(void){
    memset(objects,0,sizeof(objects));memset(handles,0,sizeof(handles));memset(&fb,0,sizeof(fb));memset(&fbo,0,sizeof(fbo));memset(&db,0,sizeof(db));memset(db_sites,0,sizeof(db_sites));
    used=handle_calls=fail_handle=call_count=fail_call=allocations=destroy_calls=disable_calls=new_calls=fail_new_at=0;
    deferred=1;fail_destroy=fail_alive=fail_array=fail_new=fail_string=missing_method=null_unbox=null_pool_target=null_array_slot=0;
    camera=allocate(1);camera->alive=1;foreign=allocate(1);foreign->alive=1;
    su.go_ctor=MP(M_CTOR);su.go_transform=MP(M_TRANSFORM);su.tr_parent=MP(M_PARENT);su.tr_position=su.tr_local_position=MP(M_POSITION);su.tr_scale=MP(M_SCALE);
    su.tr_rotation=su.tr_local_rotation=MP(M_ROTATION);su.look_rotation=MP(M_LOOK);su.obj_alive=MP(M_ALIVE);su.obj_destroy=MP(M_DESTROY);su.primitive=MP(M_PRIMITIVE);
    su.go_component=MP(M_COMPONENT);su.go_add=MP(M_ADD);su.collider_enabled=MP(M_DISABLE);su.shader_find=MP(M_SHADER);su.material_ctor=MP(M_MATERIAL);su.material_color=MP(M_COLOR);su.text_text=MP(M_TEXT);
    su.mesh_ctor=MP(M_CTOR);su.mesh_vertices=su.mesh_normals=su.mesh_uv=su.mesh_indices=su.mesh_bounds=su.material_float=su.material_keyword=su.meshfilter_set=su.renderer_materials=MP(M_OTHER);
    su.byte_class=su.vector_class=su.uv_class=su.int_class=su.material_class=MP(100);
    memset(&dbm_resources,0,sizeof(dbm_resources));
    su.root=root;su.new_array=array_new;su.string=string_new;su.array_address=su_array_address;fbo.barrier=barrier;fb.transform=MP(M_TRANSFORM);fb.text_fontsize=fb.text_anchor=fb.text_font=fb.text_color=MP(M_OTHER);
    assert(fb_pool());
}
static int make(unsigned site){return db_make(site,su_v(1,2,3),su_v(0,0,1),su_v(0,1,0));}
int main(void){
    reset();su_drop(&fbo.pool);fail_array=1;unsigned before=allocations;assert(!make(0)&&allocations==before);fail_array=0;fail_handle=handle_calls+1;assert(!make(0)&&allocations==before+1);assert(!fbo.pool);
    reset();assert(make(2)&&db_sites[2].made&&owned(3)>29&&disable_calls==29);unsigned construction_calls=call_count;drain();
    /* Inject every method exception through real construction helpers. Null or
     * stale managed results never publish a partially configured site. */
    for(unsigned fault=1;fault<=construction_calls;fault++){
        reset();fail_call=fault;(void)make(2);assert(!db_sites[2].made);fail_call=0;
        drain();
    }
    reset();assert(make(2));unsigned holds=handle_calls;drain();
    for(unsigned fault=2;fault<=holds;fault++){
        reset();fail_handle=fault;(void)make(2);assert(!db_sites[2].made);fail_handle=0;drain();
    }
    /* A failed parent keeps the detached primitive in its exact pool slot. */
    reset();fbo.group=1;PTR root=fb_object("root",0);assert(root);fail_call=call_count+3;
    assert(!fb_primitive(root,1,su_v(0,0,0),su_v(1,1,1),0));assert(owned(1)==2);fail_call=0;
    fb_retire(1);fail_destroy=1;before=allocations;fb_cleanup();assert(owned(1)==2&&!make(0)&&allocations==before);
    fail_destroy=0;fb_cleanup();assert(owned(1)==2);settle();fb_cleanup();assert(fb_empty(1));drain();
    /* Accepted deferred destruction is not release, including liveness errors. */
    reset();assert(make(0));db_site_clear(0);before=allocations;fail_alive=1;fb_cleanup();assert(!make(0)&&allocations==before&&owned(1));
    fail_alive=0;fb_cleanup();assert(owned(1)&&!make(0)&&allocations==before);settle();fb_cleanup();assert(make(0));drain();
    reset();db.journal=1;db_journal();assert(db.notebook&&db.text&&owned(5)==3);unsigned journal_calls=call_count;drain();
    for(unsigned fault=1;fault<=journal_calls;fault++){
        reset();db.journal=1;fail_call=fault;db_journal();assert(!db.notebook&&!db.text);fail_call=0;drain();
    }
    reset();db_journal();unsigned journal_holds=handle_calls;drain();
    for(unsigned fault=2;fault<=journal_holds;fault++){
        reset();fail_handle=fault;db_journal();assert(!db.notebook&&!db.text);fail_handle=0;drain();
    }
    reset();db_journal();kill(su_get(db.text));before=allocations;db_journal();assert(!db.notebook&&!db.text&&allocations==before);
    fb_cleanup();settle();fb_cleanup();db_journal();assert(db.notebook&&db.text);kill(su_get(db.notebook));db_journal();fb_cleanup();db_journal();assert(db.notebook&&db.text);drain();
    reset();missing_method=1;db_journal();assert(!db.notebook&&!db.text);missing_method=0;drain();
    reset();fail_string=1;assert(!make(0));fail_string=0;drain();
    reset();fail_new=1;assert(!make(0));fail_new=0;drain();
    /* Capacity refusal does not allocate or evict an existing owner. */
    reset();fbo.group=1;for(unsigned i=0;i<192;i++)assert(fb_new("UnityEngine","Material"));before=allocations;assert(!fb_new("UnityEngine","Material")&&allocations==before);drain();
    /* Actual provider-wrapped static uploader: every failed call/root retains
     * SDK-created resources and detached children despite SDK handle release. */
    reset();assert(make(1));unsigned skull_calls=call_count,skull_holds=handle_calls,skull_news=new_calls;assert(owned(7)==5);drain();
    for(unsigned fault=1;fault<=skull_calls;fault++){
        reset();fail_call=fault;(void)make(1);assert(!db_sites[1].made);fail_call=0;fail_destroy=1;
        before=allocations;db_cleanup();if(owned(2)||fb_pending(6)||fb_pending(7))assert(!make(1)&&allocations==before);fail_destroy=0;drain();
    }
    for(unsigned fault=2;fault<=skull_holds;fault++){
        reset();fail_handle=fault;(void)make(1);assert(!db_sites[1].made);fail_handle=0;drain();
    }
    for(unsigned fault=1;fault<=skull_news;fault++){
        reset();fail_new_at=fault;assert(!make(1)&&!db_sites[1].made);fail_new_at=0;drain();
    }
    reset();assert(make(1));db_site_clear(1);fb_materials_drop();db_cleanup();assert(dbm_resources.ready);
    settle();db_cleanup();assert(!dbm_resources.ready&&owned(7));drain();
    reset();fbo.group=1;null_pool_target=1;before=allocations;assert(!fb_new("UnityEngine","GameObject")&&allocations==before);null_pool_target=0;
    null_array_slot=1;assert(!fb_new("UnityEngine","GameObject")&&allocations==before);null_array_slot=0;
    assert(fb_object("retained",0));fb_retire(1);null_unbox=1;fb_cleanup();assert(owned(1)==1);null_unbox=0;
    null_array_slot=1;fb_cleanup();assert(owned(1)==1);null_array_slot=0;drain();
    reset();null_unbox=1;assert(!make(0));null_unbox=0;drain();
    reset();db_cleanup();db_cleanup();assert(!destroy_calls);drain();
    puts("Bones production construction/ownership fixture passed");return 0;
}
