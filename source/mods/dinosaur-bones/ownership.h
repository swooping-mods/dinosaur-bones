#ifndef MOON_BONES_OWNERSHIP_H
#define MOON_BONES_OWNERSHIP_H
/* Every native Unity allocation first has a reserved slot in a rooted Object[].
 * Per-object handle failures cannot lose it, including detached SDK children.
 * Groups 1..4 are sites, 5 the journal, 6 materials, 7 skull resources. */
#define FB_OWNED_CAPACITY 192
static struct {
    SURoot pool;unsigned group,latest;int release_materials;
    unsigned char owner[FB_OWNED_CAPACITY],retiring[FB_OWNED_CAPACITY],gameobject[FB_OWNED_CAPACITY];
    void (*barrier)(PTR,PTR *,PTR);
} fbo;
static inline PTR *fb_slot(unsigned i){PTR pool=su_get(fbo.pool);return pool?(PTR *)su.array_address(pool,sizeof(PTR),i):0;}
static inline int fb_pool(void){
    if(fbo.pool)return su_get(fbo.pool)!=0;
    PTR klass=find_class("UnityEngine","Object");if(!klass||!fbo.barrier)return 0;
    PTR pool=su.new_array(klass,FB_OWNED_CAPACITY);fbo.pool=pool?su.root(pool,1):0;return fbo.pool!=0;
}
static inline int fb_pending(unsigned group){
    for(unsigned i=0;i<FB_OWNED_CAPACITY;i++)if(fbo.owner[i]==group&&fbo.retiring[i])return 1;return 0;
}
typedef struct {int index;PTR pool;PTR *address;} FBReservation;
static inline int fb_reserve(FBReservation *slot){
    if(!fbo.group||!fb_pool()||fb_pending(fbo.group))return 0;
    slot->pool=su_get(fbo.pool);if(!slot->pool)return 0;
    for(unsigned i=0;i<FB_OWNED_CAPACITY;i++)if(!fbo.owner[i]){
        slot->index=(int)i;slot->address=(PTR *)su.array_address(slot->pool,sizeof(PTR),i);return slot->address!=0;
    }return 0;
}
static inline PTR fb_record(FBReservation slot,PTR object,int gameobject){
    if(!object)return 0;
    /* The pool has a pinned GC handle: this prevalidated array address remains
     * stable through the allocation. No fallible lookup/allocation follows the
     * returned Unity reference before the native write barrier retains it. */
    fbo.barrier(slot.pool,slot.address,object);
    fbo.latest=(unsigned)slot.index;fbo.owner[slot.index]=(unsigned char)fbo.group;fbo.gameobject[slot.index]=(unsigned char)gameobject;return object;
}
static inline PTR fb_new(const char *ns,const char *name){
    FBReservation slot;if(!fb_reserve(&slot))return 0;return fb_record(slot,su_new(ns,name),su_eq(name,"GameObject"));
}
/* SDK teardown may request removal, but only observed native death releases a
 * slot. Unknown/borrowed objects are never destroyed by this provider. */
static inline void fb_destroy(PTR object){
    if(!object||!fbo.pool)return;
    for(unsigned i=0;i<FB_OWNED_CAPACITY;i++)if(fbo.owner[i]){PTR *slot=fb_slot(i);if(slot&&*slot==object){fbo.retiring[i]=1;return;}}
}
static inline void fb_retire(unsigned group){
    for(unsigned i=0;i<FB_OWNED_CAPACITY;i++)if(fbo.owner[i]==group){
        fbo.retiring[i]=1;
        PTR *slot=fb_slot(i);if(fbo.gameobject[i]&&slot&&*slot){int ok=1;unsigned char active=0;void *a[]={&active};
            call(su_method("UnityEngine","GameObject","SetActive","System.Boolean"),*slot,a,&ok);
        }
    }
}
static inline int fb_empty(unsigned group){
    for(unsigned i=0;i<FB_OWNED_CAPACITY;i++)if(fbo.owner[i]==group)return 0;return 1;
}
static inline void fb_cleanup(void){
    if(!fbo.pool)return;
    for(unsigned i=0;i<FB_OWNED_CAPACITY;i++)if(fbo.owner[i]&&fbo.retiring[i]){
        PTR *slot=fb_slot(i);if(!slot||!*slot)continue;PTR object=*slot;int ok=1;void *a[]={object};PTR box=call(su.obj_alive,0,a,&ok);
        if(!ok||!box)continue;unsigned char *alive=unbox(box);if(!alive)continue;
        if(!*alive){
            PTR pool=su_get(fbo.pool);if(!pool)continue;fbo.barrier(pool,slot,0);fbo.owner[i]=fbo.retiring[i]=fbo.gameobject[i]=0;continue;
        }
        if(fbo.gameobject[i]){unsigned char active=0;void *v[]={&active};
            call(su_method("UnityEngine","GameObject","SetActive","System.Boolean"),object,v,&ok);
        }
        /* Failure, or accepted deferred Destroy, keeps the exact owned slot. */
        ok=1;call(su.obj_destroy,0,a,&ok);
    }
}
static inline int fb_value(PTR method_id,PTR self,void *value){
    if(!method_id||!self)return 0;int ok=1;void *a[]={value};call(method_id,self,a,&ok);return ok;
}
static inline PTR fb_get(PTR method_id,PTR self){int ok=1;PTR p=call(method_id,self,0,&ok);return ok?p:0;}
static inline int fb_parent(PTR transform,PTR parent){
    if(!transform||!parent)return 0;int ok=1;unsigned char keep_world=0;void *a[]={parent,&keep_world};call(su.tr_parent,transform,a,&ok);return ok;
}
static inline int fb_vector(PTR method_id,PTR transform,SPVec3 value){return fb_value(method_id,transform,&value);}
static inline int fb_read_vector(PTR method_id,PTR transform,SPVec3 *value){
    if(!transform)return 0;PTR box=fb_get(method_id,transform);if(!box)return 0;const void *data=unbox(box);if(!data)return 0;su_copy(value,data,sizeof(*value));return 1;
}
static inline int fb_rotation(PTR method_id,PTR transform,SPVec3 forward,SPVec3 up){
    if(!transform)return 0;int ok=1;void *a[]={&forward,&up};PTR box=call(su.look_rotation,0,a,&ok);if(!ok||!box)return 0;
    const void *data=unbox(box);if(!data)return 0;SUQuat value;su_copy(&value,data,sizeof(value));return fb_value(method_id,transform,&value);
}
static inline PTR fb_object(const char *name,PTR parent){
    PTR go=fb_new("UnityEngine","GameObject");if(!go)return 0;
    PTR text=su.string(name);if(!text)return 0;int ok=1;void *a[]={text};call(su.go_ctor,go,a,&ok);
    PTR tr=ok?fb_get(su.go_transform,go):0;if(!tr||(parent&&!fb_parent(tr,parent)))return 0;return go;
}
static inline PTR fb_component(PTR go,const char *ns,const char *name,int add){
    PTR type=su_type(ns,name);if(!go||!type)return 0;int ok=1;void *a[]={type};PTR p=call(add?su.go_add:su.go_component,go,a,&ok);return ok?p:0;
}
static inline int fb_string(PTR method_id,PTR self,const char *text){PTR value=su.string(text);return value&&fb_value(method_id,self,value);}
#endif
