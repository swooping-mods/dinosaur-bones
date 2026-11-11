/* M03: four fossil sites; discoveries belong to this world. */
#include "../../runtime/native/gameplay.h"
#include "feature.h"
#include "store.h"
#include "skull_model.h"
int _fltused=0;
static PTR db_log;
static struct {SURoot root,label;SPVec3 point;int made;} db_sites[4];
static struct {unsigned found,loaded,dirty,ticks;int save_ok,journal;SURoot notebook,text;} db;
static const char *db_ids[]={"station/north","station/east","station/south","station/west"};
static const char *db_names[]={"Lunar tyrant femur","Vacuum raptor skull","Dustback rib cage","Moonstrider tail"};
static const char *db_notes[]={"A heavy leg bone. Its owner took giant leaps.","A hollow skull. The dust did not get all of it.","A rib cage curled around a very old dust capsule.","A long tail, fossilized before the sweepers arrived."};
static void db_event(const char *event){char b[256],*p=cat(b,"{\"event\":\"");p=cat(p,event);p=cat(p,"\",\"mod\":\"dinosaur-bones\"}\n");DWORD n=0;WriteFile(db_log,b,(DWORD)(p-b),&n,0);}
static int db_save(void){
    if(!fb_host()||!db.dirty||!db.save_ok)return 0;int ok=1,joined=boolean(fb.joined,0,&ok);U64 host=fb_id(fb.host,&ok);if(!ok||joined!=fb.initial_joined||(joined&&host!=fb.host_id)){fb.suspended=1;return 0;}unsigned bytes[2]={0x314e4244u,db.found};
    if(ss_save(fb.save,(const unsigned char *)bytes,sizeof(bytes))){db.dirty=0;db_event("discoveries-saved");return 1;}db_event("discovery-save-pending");return 0;
}
static void db_drop(void){
    db_save();for(unsigned i=0;i<4;i++){su_destroy(su_get(db_sites[i].root));su_drop(&db_sites[i].root);su_drop(&db_sites[i].label);su_zero(&db_sites[i],sizeof(db_sites[i]));}
    sm_release(&dbm_resources);
    su_destroy(su_get(db.notebook));su_drop(&db.notebook);su_drop(&db.text);su_zero(&db,sizeof(db));
}
static void db_load(void){
    db.loaded=1;db.save_ok=1;db.found=0;if(!fb_host())return;
    unsigned bytes[2]={0},size=0;if(ss_load(fb.save,(unsigned char *)bytes,sizeof(bytes),&size)){
        if(size!=8||bytes[0]!=0x314e4244u||(bytes[1]&~15u)){db.save_ok=0;db_event("discoveries-invalid");}else db.found=bytes[1];
    }else if(ss.last_error!=SS_NOT_FOUND){db.save_ok=0;db_event("discoveries-read-failed");}
}
static void db_piece(PTR root,int shape,SPVec3 position,SPVec3 scale,unsigned mat,int horizontal){
    PTR go=fb_primitive(root,shape,position,scale,mat);if(go&&horizontal)su_set_quat(su.tr_local_rotation,su_transform(go),su_rotation(su_v(0,1,0),su_v(0,0,1)));
}
static int db_make(unsigned n,SPVec3 point,SPVec3 forward,SPVec3 up){
    fb_material(0,(SUColor){.76f,.70f,.51f,1});fb_material(1,(SUColor){.055f,.047f,.035f,1});fb_material(2,(SUColor){.28f,.23f,.16f,1});
    PTR go=su_gameobject("MoonLab dinosaur fossil",0);if(!go)return 0;db_sites[n].root=su_hold(go);if(!db_sites[n].root){su_destroy(go);return 0;}PTR root=su_transform(go);
    su_set_vector(su.tr_position,root,point);su_set_quat(su.tr_rotation,root,su_rotation(forward,up));
    /* Exposed bones are authored geometry, never extracted game assets. */
    if(n==0){
        db_piece(root,1,su_v(0,.20f,0),su_v(.20f,.75f,.20f),0,1);
        for(int i=-1;i<=1;i+=2)db_piece(root,0,su_v(0,.20f,i*.69f),su_v(.36f,.32f,.38f),0,0);
    }else if(n==1){
        if(!dbm_attach(root)){su_destroy(go);su_drop(&db_sites[n].root);db_event("skull-upload-pending");return 0;}
    }else if(n==2){
        db_piece(root,1,su_v(0,.22f,0),su_v(.14f,1.2f,.14f),0,1);
        for(int j=-3;j<=3;j++)for(int side=-1;side<=1;side+=2){db_piece(root,0,su_v(side*.31f,.24f,j*.29f),su_v(.64f,.16f,.15f),0,0);db_piece(root,1,su_v(side*.59f,.35f,j*.29f),su_v(.12f,.28f,.12f),0,0);}
    }else{
        for(unsigned i=0;i<10;i++){float size=.26f-i*.018f;db_piece(root,0,su_v(i*.11f,.12f,i*.31f),su_v(size,size,size*1.4f),0,0);db_piece(root,3,su_v(i*.11f,.24f,i*.31f),su_v(.06f,.22f-i*.013f,.1f),0,0);}
    }
    PTR label=fb_text(root,su_v(-.55f,1.05f,0),.035f,db_names[n]);db_sites[n].label=su_hold(label);if(!db_sites[n].label){su_destroy(go);su_drop(&db_sites[n].root);return 0;}db_sites[n].point=point;db_sites[n].made=1;db_event("fossil-placed");return 1;
}
static void db_place(void){
    int ok=1;PTR list=call(fb.stations,su_get(fb.world),0,&ok);if(!ok||!list)return;PTR type=object_class(list);int count=integer(get_method(type,"get_Count",0),list,&ok);if(!ok||count<1||count>16)return;
    for(int i=0;i<count;i++){
        void *a[]={&i};PTR collector=call(get_method(type,"get_Item",1),list,a,&ok);if(!ok||!su_alive(collector))continue;void *b[]={collector};PTR id=call(fb.station_id,su_get(fb.world),b,&ok);char name[80];if(!ok||!id)continue;su_string(id,name,sizeof(name));
        for(unsigned n=0;n<4;n++)if(!db_sites[n].made&&su_eq(name,db_ids[n])){
            PTR tr=fb_transform(collector);SPVec3 origin=su_vector(su.tr_get_position,tr),right=su_vector(su.tr_right,tr),forward=su_vector(su.tr_forward,tr),up=su_vector(su.tr_up,tr),point;
            /* Stable collector-relative offsets let saves recover the same site.
             * Probe visible terrain each time; no world-Y or dust mesh guesses. */
            SPVec3 candidate=su_add(origin,su_add(su_mul(right,17.f+n*4.f),su_mul(forward,-18.f-n*5.f)));
            if(fb_floor(candidate,up,&point))db_make(n,su_add(point,su_mul(up,n==1?.002f:.025f)),forward,up);
        }
    }
}
static int db_visible(SPVec3 from,SPVec3 to){
    SPVec3 d=su_sub(to,from);float length=su_sqrt(su_dot(d,d));if(length<.3f)return 1;d=su_mul(d,1/length);float distance=length-.25f;int mask=~0,query=1,ok=1;union {U64 align;unsigned char bytes[96];} hit;su_zero(&hit,sizeof(hit));
    void *a[]={&from,&d,hit.bytes,&distance,&mask,&query};PTR b=call(su.physics_ray,0,a,&ok);return ok&&b&&!*(unsigned char *)unbox(b);
}
static void db_journal(void){
    if(!db.notebook){PTR camera=0;field_value(su_get(fb.walker),fb.camera_field,&camera);if(!camera)return;PTR root=su_gameobject("MoonLab fossil journal",fb_transform(camera));db.notebook=su_hold(root);su_set_vector(su.tr_local_position,su_transform(root),su_v(-.6f,.32f,1.6f));db.text=su_hold(fb_text(su_transform(root),su_v(0,0,0),.033f,""));}
    char b[1500],*p=cat(b,"DINOSAUR DISCOVERIES          Ctrl+Shift+F3 close\n\n");unsigned count=0;
    for(unsigned n=0;n<4;n++){int found=(db.found>>n)&1;count+=found;p=cat(p,found?"[FOUND] ":"[  ?  ] ");p=cat(p,found?db_names[n]:"Undiscovered fossil");p=cat(p,"\n");if(found){p=cat(p,db_notes[n]);p=cat(p,"\n");}}
    p=cat(p,"\n");p=num(p,count);p=cat(p," / 4 discoveries\nSearch outside the four cardinal bases.\nAim at a fossil nearby and press E.");if(!fb_host())p=cat(p,"\nDiscovery recording requires the world host.");if(!db.save_ok)p=cat(p,"\nSave unavailable; discovery recording is paused.");else if(db.dirty)p=cat(p,"\nSaving discoveries... ");su_text(su_get(db.text),b);
    su_bool(method("UnityEngine","GameObject","SetActive",1),su_get(db.notebook),db.journal);
}
static int db_before(int id,Context *c){
    (void)id;if(!fb_tick((PTR)c->rcx,db_drop))return 0;if(!db.loaded)db_load();if(++db.ticks%120==1)db_place();if(db.dirty&&db.ticks%180==0)db_save();
    /* Leave Ctrl+F3 to KDE; journal input requires Ctrl+Shift+F3. */
    MoonInputState journal_input=moon_input_action("dinosaur-bones.journal","!Alt+Ctrl+Shift+F3",fb.allow),use_input=moon_input_action("dinosaur-bones.discover","E",fb.allow);
    if(journal_input.pressed)db.journal=!db.journal;if(!fb.allow)db.journal=0;
    PTR walker=su_get(fb.walker),camera=0;field_value(walker,fb.camera_field,&camera);SPVec3 pos=su_vector(su.tr_get_position,fb_transform(walker));SPVec3 eye=su_vector(su.tr_get_position,fb_transform(camera)),facing=su_vector(su.tr_forward,fb_transform(camera));
    int nearest=-1;float best=12.25f;
    for(unsigned n=0;n<4;n++)if(db_sites[n].made){SPVec3 delta=su_sub(db_sites[n].point,pos);float dist=su_dot(delta,delta);PTR label=su_get(db_sites[n].label);PTR go=call(su_method("UnityEngine","Component","get_gameObject",""),label,0,&(int){1});
        su_bool(method("UnityEngine","GameObject","SetActive",1),go,dist<36);
        if(dist<best&&su_dot(su_normal(su_sub(db_sites[n].point,eye)),facing)>.7f){SPVec3 up=su_vector(su.tr_up,fb_transform(walker));if(db_visible(eye,su_add(db_sites[n].point,su_mul(up,.3f)))){best=dist;nearest=(int)n;}}
        char text[200],*p=cat(text,db_names[n]);p=cat(p,(db.found&(1u<<n))?"\nDISCOVERED - Ctrl+Shift+F3 journal":"\nE - examine fossil");su_text(label,text);
    }
    if(fb.allow&&fb_host()&&db.save_ok&&use_input.pressed&&nearest>=0&&!(db.found&(1u<<nearest))){db.found|=1u<<nearest;db.dirty=1;db_save();db.journal=1;db_event(db_names[nearest]);}
    if(db.journal||db.notebook)db_journal();return 0;
}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
    char path[1100];DWORD n=GetEnvironmentVariableA("MOONLAB_EVIDENCE",path,1000);if(!n||n>=1000)return 0;cat(path+n,"\\dinosaur-bones.jsonl");db_log=CreateFileA(path,0x40000000,1,0,1,0x80,0);if(db_log==(PTR)(U64)-1)return 0;
    if(!bind_runtime(domain,game)){db_event("runtime-bind-failed");return 0;}
    if(!fb_bind(game)){db_event(su.failed?su.failed:"feature-bind-failed");return 0;}
    if(!ss_bind()){db_event("store-bind-failed");return 0;}
    if(!prepare_hook(method("MoonCleaners","MoonWalker","Update",0),100671733,0,game)){db_event("update-prepare-failed");return 0;}before_hook=db_before;return install_hooks();
}
