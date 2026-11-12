/* Self-contained subset of the existing slime Unity adapter. No feature-mod
 * dependency: packages contain their own compiled adapter and original assets. */
#ifndef MOON_BACKLOG_FEATURE_H
#define MOON_BACKLOG_FEATURE_H
#include "unity.h"
#include "../../runtime/native/input.h"
#include "ownership.h"
static struct {
    PTR world_field,camera_field,transform,frame,focused,typing,blocked,menu_blocked,set_blocked,restoring,firstperson;
    PTR joined,host,local,save_root,stations,station_id;
    PTR text_value,text_size,text_anchor,text_font,text_fontsize,text_color;
    short (*key)(int);PTR (*foreground)(void);DWORD (*window_pid)(PTR,DWORD *);
    SURoot world,walker,materials[4];DWORD thread;int last_frame,ready_frames,ready,allow,authority,suspended,initial_joined;
    U64 host_id;char save[181];
} fb;
static inline U64 fb_id(PTR m,int *ok){PTR b=call(m,0,0,ok);return b?*(U64 *)unbox(b):0;}
static inline PTR fb_transform(PTR component){int ok=1;return call(fb.transform,component,0,&ok);}
static inline int fb_down(int vk){return moon_input_key((unsigned)vk);}
static inline int fb_host(void){return fb.authority&&!fb.suspended;}
static inline void fb_materials_drop(void){fbo.release_materials=1;for(unsigned i=0;i<4;i++)su_drop(&fb.materials[i]);}
static inline PTR fb_material(unsigned n,SUColor color){
    if(n>=4||fbo.release_materials||fb_pending(6))return 0;if(fb.materials[n])return su_get(fb.materials[n]);fbo.group=6;
    int ok=1;PTR name=su.string("Universal Render Pipeline/Lit");if(!name)return 0;void *q[]={name};PTR shader=call(su.shader_find,0,q,&ok);
    SURoot held=ok?su_hold(shader):0;if(!held)return 0;
    PTR mat=fb_new("UnityEngine","Material");if(!mat){su_drop(&held);return 0;}unsigned slot=fbo.latest;void *args[]={shader};call(su.material_ctor,mat,args,&ok);su_drop(&held);
    PTR property=su.string("_BaseColor");void *a[]={&color},*b[]={property,&color};
    if(ok)call(su.material_color,mat,a,&ok);if(ok&&property)call(su_method("UnityEngine","Material","SetColor","System.String;UnityEngine.Color"),mat,b,&ok);else ok=0;
    if(ok)fb.materials[n]=su_hold(mat);if(!ok||!fb.materials[n]){fbo.retiring[slot]=1;return 0;}return mat;
}
static inline PTR fb_primitive(PTR parent,int shape,SPVec3 position,SPVec3 scale,unsigned material){
    FBReservation slot;if(!fb_reserve(&slot))return 0;int ok=1;void *args[]={&shape};PTR go=call(su.primitive,0,args,&ok);fb_record(slot,go,1);
    if(!ok||!go||material>=4)return 0;PTR tr=fb_get(su.go_transform,go);
    if(!tr||!fb_parent(tr,parent)||!fb_vector(su.tr_local_position,tr,position)||!fb_vector(su.tr_scale,tr,scale))return 0;
    PTR collider=fb_component(go,"UnityEngine","Collider",0);unsigned char disabled=0;
    /* Disable is synchronous. Destroy acceptance alone cannot establish that a
     * deferred collider stopped taking part in this frame's physics queries. */
    if(!collider||!fb_value(su.collider_enabled,collider,&disabled))return 0;
    void *c[]={collider};call(su.obj_destroy,0,c,&ok);if(!ok)return 0;
    PTR renderer=fb_component(go,"UnityEngine","Renderer",0),mat=su_get(fb.materials[material]);
    if(!renderer||!su_alive(mat)||!fb_value(su_method("UnityEngine","Renderer","set_sharedMaterial","UnityEngine.Material"),renderer,mat))return 0;return go;
}
/* TextMesh is stripped. A checked world-space Canvas uses retained uGUI Text. */
static inline PTR fb_text(PTR parent,SPVec3 position,float size,const char *text){
    PTR go=fb_object("MoonLab mod display",parent);if(!go)return 0;
    PTR canvas=fb_component(go,"UnityEngine","Canvas",1);int mode=2;
    PTR tr=fb_get(su.go_transform,go);float scale=size/24.f;
    if(!canvas||!fb_value(su_method("UnityEngine","Canvas","set_renderMode","UnityEngine.RenderMode"),canvas,&mode)||
       !fb_vector(su.tr_local_position,tr,position)||!fb_vector(su.tr_scale,tr,su_v(scale,scale,scale)))return 0;
    PTR child=fb_object("MoonLab mod text",tr);if(!child)return 0;
    PTR mesh=fb_component(child,"UnityEngine.UI","Text",1),rect=fb_component(child,"UnityEngine","RectTransform",0);
    struct {float x,y;} extent={1200,700},pivot={0,1};int fontsize=48,anchor=0;
    if(!mesh||!rect||!fb_value(su_method("UnityEngine","RectTransform","set_sizeDelta","UnityEngine.Vector2"),rect,&extent)||
       !fb_value(su_method("UnityEngine","RectTransform","set_pivot","UnityEngine.Vector2"),rect,&pivot)||
       !fb_value(fb.text_fontsize,mesh,&fontsize)||!fb_value(fb.text_anchor,mesh,&anchor)||!fb_string(su.text_text,mesh,text))return 0;
    PTR type=su_type("UnityEngine","Font"),name=su.string("LegacyRuntime.ttf");if(!type||!name)return 0;
    int ok=1;void *a[]={type,name};PTR font=call(su_method("UnityEngine","Resources","GetBuiltinResource","System.Type;System.String"),0,a,&ok);
    SUColor white={.94f,.95f,.96f,1};if(!ok||!font||!fb_value(fb.text_font,mesh,font)||!fb_value(fb.text_color,mesh,&white))return 0;return mesh;
}
static inline int fb_floor(SPVec3 point,SPVec3 up,SPVec3 *floor){
    union {U64 align;unsigned char bytes[96];} hit;su_zero(&hit,sizeof(hit));SPVec3 origin=su_add(point,su_mul(up,12)),down=su_mul(up,-1);float range=30;int mask=~0,query=1,ok=1;
    void *a[]={&origin,&down,hit.bytes,&range,&mask,&query};PTR b=call(su.physics_ray,0,a,&ok);
    if(!ok||!b||!*(unsigned char *)unbox(b))return 0;SPVec3 normal;su_copy(floor,hit.bytes+su.hit_point,sizeof(*floor));su_copy(&normal,hit.bytes+su.hit_normal,sizeof(normal));return su_dot(normal,up)>.65f;
}
static inline int fb_bind(PTR game){
    if(!su_bind(game))return 0;
    fbo.barrier=(void(*)(PTR,PTR *,PTR))GetProcAddress(game,"il2cpp_gc_wbarrier_set_field");if(!fbo.barrier)return 0;
#define FB(F,NS,T,N,S) su.failed=T "::" N;fb.F=su_method(NS,T,N,S);if(!fb.F)return 0
    FB(transform,"UnityEngine","Component","get_transform","");FB(frame,"UnityEngine","Time","get_frameCount","");FB(focused,"UnityEngine","Application","get_isFocused","");
    FB(typing,"MoonCleaners","MoonInput","get_Typing","");FB(blocked,"MoonCleaners","MoonWalker","get_UIBlocked","");FB(menu_blocked,"MoonCleaners","MoonWalker","get_MenuBlocked","");FB(set_blocked,"MoonCleaners","MoonWalker","set_UIBlocked","System.Boolean");
    FB(restoring,"MoonCleaners","MoonWorld","get_IsRestoringRendering","");FB(firstperson,"MoonCleaners","MoonWalker","get_IsFirstPerson","");FB(save_root,"MoonCleaners","MoonWorld","get_SaveDirectory","");
    FB(joined,"MoonCleaners","MoonCrew","get_Joined","");FB(host,"MoonCleaners","MoonCrew","get_HostId","");FB(local,"Steamworks","SteamUser","GetSteamID","");
    FB(stations,"MoonCleaners","MoonWorld","get_Stations","");FB(station_id,"MoonCleaners","MoonWorld","StationId","MoonCleaners.MoonCollector");
    FB(text_anchor,"UnityEngine.UI","Text","set_alignment","UnityEngine.TextAnchor");
    FB(text_font,"UnityEngine.UI","Text","set_font","UnityEngine.Font");FB(text_fontsize,"UnityEngine.UI","Text","set_fontSize","System.Int32");FB(text_color,"UnityEngine.UI","Graphic","set_color","UnityEngine.Color");
#undef FB
    su.text_text=su_method("UnityEngine.UI","Text","set_text","System.String");if(!su.text_text)return 0;
    fb.world_field=get_field(find_class("MoonCleaners","MoonWalker"),"world");fb.camera_field=get_field(find_class("MoonCleaners","MoonWalker"),"viewCamera");if(!fb.world_field||!fb.camera_field)return 0;
    PTR user=LoadLibraryA("user32.dll");fb.key=(short(*)(int))GetProcAddress(user,"GetAsyncKeyState");fb.foreground=(PTR(*)(void))GetProcAddress(user,"GetForegroundWindow");fb.window_pid=(DWORD(*)(PTR,DWORD *))GetProcAddress(user,"GetWindowThreadProcessId");return fb.key&&fb.foreground&&fb.window_pid;
}
/* The caller releases its scene state before old world handles are dropped.
 * Authority is latched per world so a guest disconnect cannot permit writes. */
static inline int fb_tick(PTR walker,void (*drop)(void),void (*cleanup)(void)){
    if(!fb.thread)fb.thread=GetCurrentThreadId();if(fb.thread!=GetCurrentThreadId())return 0;
    int ok=1,frame=integer(fb.frame,0,&ok);if(!ok||frame==fb.last_frame)return 0;fb.last_frame=frame;cleanup();
    PTR world=0;field_value(walker,fb.world_field,&world);
    if(world!=su_get(fb.world)||walker!=su_get(fb.walker)||!su_alive(world)){
        drop();fb_materials_drop();su_drop(&fb.world);su_drop(&fb.walker);fb.ready=fb.ready_frames=fb.allow=fb.authority=fb.suspended=0;fb.save[0]=0;
        if(!world||!su_alive(world))return 0;fb.world=su_hold(world);fb.walker=su_hold(walker);if(!fb.world||!fb.walker){su_drop(&fb.world);su_drop(&fb.walker);return 0;}
    }
    if(boolean(fb.restoring,world,&ok)||!ok)return 0;
    if(!fb.ready){
        if(fb.ready_frames<90)fb.ready_frames++;if(fb.ready_frames<90)return 0;PTR path=call(fb.save_root,world,0,&ok);if(!ok||!path)return 0;
        int n=su.string_length(path);if(n<1||n>180)return 0;const unsigned short *chars=su.chars(path);for(int i=0;i<n;i++)if(chars[i]<32||chars[i]>126)return 0;
        su_string(path,fb.save,sizeof(fb.save));fb.initial_joined=boolean(fb.joined,0,&ok);fb.host_id=fb_id(fb.host,&ok);U64 local=fb_id(fb.local,&ok);
        fb.authority=ok&&(!fb.initial_joined||(fb.host_id&&local&&fb.host_id==local));fb.ready=1;
    }
    int joined=boolean(fb.joined,0,&ok);U64 host=fb_id(fb.host,&ok);
    if(!ok||joined!=fb.initial_joined||(joined&&host!=fb.host_id))fb.suspended=1;
    /* UIBlocked also includes Held during ordinary equipment use. MenuBlocked
     * supplies the menu gate without disabling examination while holding tools. */
    DWORD pid=0;fb.window_pid(fb.foreground(),&pid);fb.allow=pid==GetCurrentProcessId()&&boolean(fb.focused,0,&ok)&&!boolean(fb.typing,0,&ok)&&!boolean(fb.menu_blocked,walker,&ok)&&boolean(fb.firstperson,walker,&ok)&&ok;
    return 1;
}
#endif
