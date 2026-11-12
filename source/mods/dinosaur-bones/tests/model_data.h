/* Original minimal bytes exercise production upload and PBR paths; no retained
 * skull asset bytes are needed by this source/host fixture. */
static const float dbm_positions[]={0,0,0,1,0,0,0,1,0},dbm_normals[]={0,0,1,0,0,1,0,0,1},dbm_uvs[]={0,0,1,0,0,1};
static const unsigned dbm_indices[]={0,1,2};
static const unsigned char dbm_png[]={1};
static const SMImage dbm_images[]={{dbm_png,1},{dbm_png,1},{dbm_png,1}};
static const SMPart dbm_parts[]={
    {"fixture skull",3,3,(const unsigned char *)dbm_positions,(const unsigned char *)dbm_normals,(const unsigned char *)dbm_uvs,(const unsigned char *)dbm_indices,0,{1,1,1,1},{0,0,0},.2f,.6f,0,0}
};
static const SMModel dbm_model={dbm_images,3,dbm_parts,1};
static const SMPbr dbm_pbr[]={{1,2,1}};
