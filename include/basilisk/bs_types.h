#ifndef BS_TYPES_H
#define BS_TYPES_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define BS_MAX_ATTACHMENTS 8
#define BS_MAX_TEXTURES 8

// CGLM Alignment
#if defined(_MSC_VER)
#  if _MSC_VER < 1913 // Visual Studio 2017 version 15.6
#    define BS_ALL_UNALIGNED
#    define BS_ALIGN(X) // no alignment
#  else
#    define BS_ALIGN(X) __declspec(align(X))
#  endif
#else
#  define BS_ALIGN(X) __attribute((aligned(X)))
#endif

#ifndef BS_ALL_UNALIGNED
#  define BS_ALIGN_IF(X) BS_ALIGN(X)
#else
#  define BS_ALIGN_IF(X) // no alignment
#endif

#ifdef __AVX__
#  define BS_ALIGN_MAT BS_ALIGN(32)
#else
#  define BS_ALIGN_MAT BS_ALIGN(16)
#endif

#include <stdlib.h>
#include <stdio.h>

// Math
typedef union bs_vec2 bs_vec2;
typedef union bs_vec3 bs_vec3;
typedef union bs_vec4 bs_vec4;
typedef union bs_umat3 bs_umat3;
typedef union bs_umat4 bs_umat4;

typedef union bs_ivec2 bs_ivec2;
typedef union bs_ivec3 bs_ivec3;
typedef union bs_ivec4 bs_ivec4;

typedef struct bs_uvec2 bs_uvec2;
typedef struct bs_uvec3 bs_uvec3;
typedef struct bs_uvec4 bs_uvec4;

// Wnd
typedef struct bs_Error bs_Error;
typedef struct bs_ErrorData bs_ErrorData;
// Mem
typedef struct bs_Buffer bs_Buffer;
// Shaders
typedef struct bs_ShaderMaterial bs_ShaderMaterial;
typedef struct bs_ShaderEntity bs_ShaderEntity;
typedef struct bs_ShaderEntityData bs_ShaderEntityData;
typedef struct bs_Shader bs_Shader;
typedef struct bs_ShaderSpace bs_Space;
typedef struct bs_GeometryShader bs_GeometryShader;
typedef struct bs_FragmentShader bs_FragmentShader;
typedef struct bs_ComputeShader bs_ComputeShader;
typedef struct bs_VertexShader bs_VertexShader;
typedef struct bs_Attribute bs_Attribute;
typedef struct bs_Pipeline bs_Pipeline;
// Textures
typedef struct bs_Texture bs_Texture;
typedef struct bs_TextureFunc bs_TextureFunc;
typedef struct bs_ImageShaderData bs_ImageShaderData;
typedef struct bs_Image bs_Image;
// Core
typedef struct bs_RenderData bs_RenderData;
typedef struct bs_Framebuf bs_Framebuf;
typedef struct bs_Batch bs_Batch;
typedef struct bs_BatchPart bs_BatchPart;
typedef struct bs_Quad bs_Quad;
typedef struct bs_Triangle bs_Triangle;
typedef struct bs_Edge bs_Edge;
// Models
typedef struct bs_Gltf bs_Gltf;
typedef struct bs_Channel bs_Channel;
typedef struct bs_AnimationJoint bs_AnimationJoint;
typedef struct bs_Animation bs_Animation;
typedef struct bs_Joint bs_Joint;
typedef struct bs_ArmatureStorage bs_ArmatureStorage;
typedef struct bs_Armature bs_Armature;
typedef struct bs_Primitive bs_Primitive;
typedef struct bs_Mesh bs_Mesh;
typedef struct bs_Model bs_Model;
typedef struct bs_ShadowVolume bs_ShadowVolume;
typedef struct bs_Material bs_Material;
// Ttf
typedef struct bs_Font bs_Font;
// Audio
typedef struct bs_Sound bs_Sound;
// Json
typedef struct bs_JsonField bs_JsonField;
typedef struct bs_JsonString bs_JsonString;
typedef struct bs_JsonArray bs_JsonArray;
typedef struct bs_JsonToken bs_JsonToken;
typedef struct bs_Json bs_Json;
typedef struct bs_JsonValue bs_JsonValue;

typedef struct bs_Plane bs_Plane;

typedef int64_t bs_I64;
typedef int32_t bs_I32;
typedef int16_t bs_I16;
typedef int8_t  bs_I8;

typedef uint64_t bs_U64;
typedef uint32_t bs_U32;
typedef uint16_t bs_U16;
typedef uint8_t  bs_U8;

typedef double bs_F64;
typedef float bs_F32;

typedef union  bs_RGBA bs_RGBA;
typedef union  bs_RGB bs_RGB;

typedef enum bs_RenderType bs_RenderType;
typedef enum bs_ErrorCode bs_ErrorCode;

typedef struct bs_PerformanceData bs_PerformanceData;

typedef struct bs_aabb bs_aabb;

typedef enum bs_AttributeType bs_AttributeType;
enum bs_AttributeType {
	BS_POSITION = 0,
	BS_TEXTURE,
	BS_COLOR,
	BS_NORMAL,
	BS_BONE_ID,
	BS_WEIGHT,
	BS_ENTITY,
	BS_IMAGE,

	BS_NUM_ATTRIBUTES
};

// Initialized by: 
// bs_v2(float, float), 
// bs_v2s(float)
union bs_vec2 { 
    float a[2];
    struct { float x, y; };

#ifdef __cplusplus
    inline bool operator!=(bs_vec2 a) {
        return (x != a.x) || (y != a.y);
    }

    inline bool operator==(bs_vec2 a) {
        return (x == a.x) && (y == a.y);
    }

    // +
    inline bs_vec2 operator+(bs_vec2 a) {
        return { x + a.x, y + a.y };
    }

    // +=
    inline void operator+=(bs_vec2 a) {
        x += a.x;
        y += a.y;
    }

    inline bs_vec2 operator-(bs_vec2 a) {
        return { x - a.x, y - a.y };
    }

    inline bs_vec2 operator*(bs_vec2 a) {
        return { x * a.x, y * a.y };
    }

    inline bs_vec2 operator/(bs_vec2 a) {
        return { x / a.x, y / a.y };
    }

    inline bs_vec2 operator*(float v) {
        return { x * v, y * v };
    }

    inline bs_vec2 operator/(float v) {
        return { x / v, y / v };
    }
#endif 
};

// Initialized by: 
// bs_v3(float, float, float), 
// bs_v3s(float), 
// bs_v3fromv2(bs_vec3, float)
union bs_vec3 { 
    float a[3];
    struct { float x, y, z; };
    struct { bs_vec2 xy; float p0; };

#ifdef __cplusplus
    inline bool operator!=(bs_vec3 a) {
        return (x != a.x) || (y != a.y) || (y != a.z);
    }

    inline bool operator==(bs_vec3 a) {
        return (x == a.x) && (y == a.y) && (z == a.z);
    }

    // +
    inline bs_vec3 operator+(bs_vec3 a) {
        return { x + a.x, y + a.y, z + a.z };
    }

    // +=
    inline void operator+=(bs_vec3 a) {
        x += a.x;
        y += a.y;
        z += a.z;
    }

    // -=
    inline void operator-=(bs_vec3 a) {
        x -= a.x;
        y -= a.y;
        z -= a.z;
    }

    inline bs_vec3 operator-(bs_vec3 a) {
        return { x - a.x, y - a.y, z - a.z };
    }

    inline bs_vec3 operator*(bs_vec3 a) {
        return { x * a.x, y * a.y, z * a.z };
    }

    inline bs_vec3 operator/(bs_vec3 a) {
        return { x / a.x, y / a.y, z / a.z };
    }

    inline bs_vec3 operator*(float v) {
        return { x * v, y * v, z * v };
    }

    inline bs_vec3 operator/(float v) {
        return { x / v, y / v, z / v };
    }

    bool operator<(const bs_vec3& v) const {
        if (x != v.x) return x < v.x;
        if (y != v.y) return y < v.y;
        return z < v.z;
    }
#endif 
};

// Initialized by: 
// bs_v4(float, float, float, float), 
// bs_v4s(float), 
// bs_v4fromv3(bs_vec3, float)
union bs_vec4 {
    float a[4];
    struct { float x, y, z, w; };
    struct { bs_vec2 xy; float p0, p1; };
    struct { bs_vec3 xyz; float p2; };

#ifdef __cplusplus
    inline bool operator!=(bs_vec4 a) {
        return (x != a.x) || (y != a.y) || (y != a.z) || (w != a.w);
    }

    inline bool operator==(bs_vec4 a) {
        return (x == a.x) && (y == a.y) && (z == a.z) && (w == a.w);
    }

    inline bs_vec4 operator+(bs_vec4 a) {
        return { x + a.x, y + a.y, z + a.z, w + a.w };
    }

    inline bs_vec4 operator-(bs_vec4 a) {
        return { x - a.x, y - a.y, z - a.z, w - a.w };
    }

    inline bs_vec4 operator*(bs_vec4 a) {
        return { x * a.x, y * a.y, z * a.z, w * a.w };
    }

    inline bs_vec4 operator/(bs_vec4 a) {
        return { x / a.x, y / a.y, z / a.z, w / a.w };
    }
#endif 
};

union bs_ivec2 { 
    int a[2];
    struct { int x, y; };

#ifdef __cplusplus
    inline bool operator!=(bs_ivec2 a) {
        return (x != a.x) || (y != a.y);
    }

    inline bool operator==(bs_ivec2 a) {
        return (x == a.x) && (y == a.y);
    }

    inline bs_ivec2 operator+(bs_ivec2 a) {
        return { x + a.x, y + a.y };
    }

    inline bs_ivec2 operator-(bs_ivec2 a) {
        return { x - a.x, y - a.y };
    }

    inline bs_ivec2 operator*(bs_ivec2 a) {
        return { x * a.x, y * a.y };
    }

    inline bs_ivec2 operator/(bs_ivec2 a) {
        return { x / a.x, y / a.y };
    }

    inline bs_ivec2 operator*(int v) {
        return { x * v, y * v };
    }

    inline bs_ivec2 operator/(int v) {
        return { x / v, y / v };
    }
#endif 
};

union bs_ivec3 { 
    int a[3];
    struct { int x, y, z; };
    struct { bs_ivec2 xy; int p0; };

#ifdef __cplusplus
    inline bool operator!=(bs_ivec3 a) {
        return (x != a.x) || (y != a.y) || (y != a.z);
    }

    inline bool operator==(bs_ivec3 a) {
        return (x == a.x) && (y == a.y) && (z == a.z);
    }

    inline bs_ivec3 operator+(bs_ivec3 a) {
        return { x + a.x, y + a.y, z + a.z };
    }

    inline bs_ivec3 operator-(bs_ivec3 a) {
        return { x - a.x, y - a.y, z - a.z };
    }

    inline bs_ivec3 operator*(bs_ivec3 a) {
        return { x * a.x, y * a.y, z * a.z };
    }

    inline bs_ivec3 operator/(bs_ivec3 a) {
        return { x / a.x, y / a.y, z / a.z };
    }

    inline bs_ivec3 operator*(int v) {
        return { x * v, y * v, z * v };
    }

    inline bs_ivec3 operator/(int v) {
        return { x / v, y / v, z / v };
    }
#endif 
};

union bs_ivec4 {
    int a[4];
    struct { int x, y, z, w; };
    struct { bs_ivec2 xy; int p0, p1; };
    struct { bs_ivec3 xyz; int p2; };
};
       
struct bs_uvec2 { bs_U32 x, y; };
struct bs_uvec3 { bs_U32 x, y, z; };
struct bs_uvec4 { bs_U32 x, y, z, w; };

// Initialized by: 
// bs_rgba(U8, U8, U8, U8), 
// bs_hex(bs_U32)
union  bs_RGBA { struct { unsigned char r, g, b, a; }; bs_U32 hex; };
union  bs_RGB   { struct { unsigned char r, g, b;    }; bs_U32 hex : 24; };
union  bs_umat4 {
    bs_vec4 v[4];
    float f[4 * 4];
    float a[4][4];

#ifdef __cplusplus
    inline bool operator!=(bs_umat4 m) {
        return (v[0] != m.v[0]) || (v[1] != m.v[1]) || (v[2] != m.v[2]) || (v[3] != m.v[3]);
    }

    inline bool operator==(bs_umat4 m) {
        return (v[0] == m.v[0]) && (v[1] == m.v[1]) && (v[2] == m.v[2]) && (v[3] == m.v[3]);
    }
#endif 
};

union  bs_umat3 {
    bs_vec3 v[3];
    float f[3 * 3];
    float a[3][3];
};

typedef BS_ALIGN_IF(16) bs_vec2 bs_mat2[2];
typedef BS_ALIGN_IF(16)	bs_umat4 bs_mat4;
typedef bs_umat3 bs_mat3;

typedef bs_vec4 	bs_quat;

struct bs_aabb {
    bs_vec3 min;
    bs_vec3 max;
};

struct bs_ErrorData {
    bs_U32 type;
    bs_U32 id;
};

struct bs_Error {
    bs_U32 severity;
    bs_U32 code;
    const char* description;
};

struct bs_RenderData {
    struct {
        int buf;
        bool value;
    } bufs[BS_MAX_ATTACHMENTS];
    bs_U32 num_bufs;

    struct {
        bs_I8 unit;
        bs_Texture* texture;
    } textures[BS_MAX_TEXTURES];
    bs_U32 num_textures;
};

struct bs_Texture {
    bs_ivec3 frame;
    int num_frames;

    int w, h;
    float texw, texh;

    int type;
    int attachment;

    unsigned int id;
    unsigned int unit;

    unsigned char *data;
};

struct bs_ImageShaderData {
    bs_U64 handle;
    bs_vec2 coords;
};

struct bs_Image {
    bs_ImageShaderData* data;
    bs_U32 buffer_location;
};

struct bs_Buffer {
    bs_U32 num_units;
    bs_U32 unit_size;
    bs_U32 capacity;

    bs_U32 max_units;
    bs_U32 increment;

    bs_U8* data;
};

struct bs_Pipeline {
    bs_VertexShader* vs;

    void* state;
};

struct bs_GeometryShader {
    bs_U32 id;
};

struct bs_FragmentShader {
    bs_U32 id;
    void* module;
};

struct bs_Attribute {
    bs_U8 format;
    bs_U8 size;
};

struct bs_VertexShader {
    bs_U32 id;

    bs_Attribute attributes[BS_NUM_ATTRIBUTES];
    int attrib_size_bytes;
    int attrib_count;
    bs_U32 attribs;

    void* module;
};

struct bs_ComputeShader {
    bs_U32 id;
    bs_U32 program;
};

// Needs to be identical to shader equivalent in basilisk.bsh
struct bs_ShaderEntityData {
    bs_mat4 transform;
};

struct bs_ShaderEntity {
    bs_ShaderEntityData* entity_data;

    bool changed;
    bs_U32 buffer_location;
};

struct bs_ShaderSpace {
    bs_U32 accessor;
    bs_U32 bind_point;

    bs_U32 gl_type;

    bs_Buffer buf;
};

struct bs_Shader {
    bs_VertexShader* vs;

    // OpenGL Variables
    bs_U32 id;
};

struct bs_Framebuf {
    bs_vec2 dim;

    unsigned int FBO, RBO;

    int clear;
    int culling;

    bs_Buffer buf;
};

struct bs_BatchPart {
    bs_U32 offset;
    bs_U32 num;
};

struct bs_Batch {
    bs_Pipeline pipeline;

    bool use_indices;

    int attrib_count;
    size_t attrib_offset;

    bs_Buffer vertex_buf;
    bs_Buffer index_buf;

    bs_U32 VAO, VBO, EBO;

    void* vbuffer, *ibuffer;
};

struct bs_Quad {
    bs_vec3 a, b, c, d; // positions
    bs_vec2 ca, cb, cc, cd; // coords
};

struct bs_Edge {
    bs_vec3 a, b;
};

struct bs_Triangle {
    int edges[3];
};

struct bs_Joint {
    bs_mat4 local_inv;
    bs_mat4 bind_matrix;
    bs_mat4 bind_matrix_inv;

    int parent_idx;
    int loc;

    char* name;
    bs_U32 id;

    int* children;
    int num_children;
};

struct bs_ArmatureStorage {
    int buffer_location;
    bs_Armature* armature;
};

struct bs_Armature {
    bs_mat4* joint_matrices;

    bs_Joint* joints;
    int num_joints;

    char* name;
};

struct bs_ShaderMaterial {
    bs_vec4 color;
};

struct bs_Material {
    bs_ShaderMaterial data;

    bs_U32 shader_material;
    bs_U64 texture_handle;
};

struct bs_Primitive {
    float* vertices;
    int num_vertices;
    int vertex_size;

    int offset_tex;
    int offset_nor;
    int offset_bid;
    int offset_wei;
    int offset_idx;

    int* indices;
    int num_indices;
    
    int material_idx;

    bs_Mesh *parent;
    bs_aabb aabb;
};

struct bs_Mesh {
    char* name;

    bs_Primitive *primitives;
    int num_primitives;

    int num_vertices;
    int num_indices;

    bs_Model *parent;
    bs_aabb aabb;
};

struct bs_Model {
    char* name;
    //char** texture_names;

    bs_U32 animation_offset;
    bs_Armature* armatures;
    bs_Mesh* meshes;

    //int image_count;
    int armature_count;
    int num_meshes;
    int prim_count;
    
    int anim_count;
    int anim_offset;

    int num_vertices;
    int num_indices;

    bs_aabb aabb;
};

struct bs_ShadowVolume {
    struct bs_VolumeTriangle {
        bool ccw;
        struct bs_VolumeEdge {
            bool direction; // left or right
            int counter;
        }* edges;
    }* triangles;
};

#define TRANSLATION_JOINT_SIZE sizeof(bs_vec3) + sizeof(float)
#define ROTATION_JOINT_SIZE sizeof(bs_quat) + sizeof(float)
#define SCALE_JOINT_SIZE sizeof(bs_vec3) + sizeof(float)

struct bs_AnimationJoint {
    struct {
        float time;
        bs_vec3 value;
    }* translations;

    struct {
        float time;
        bs_quat value;
    }* rotations;

    struct {
        float time;
        bs_vec3 value;
    }* scalings;

    bs_U32 num_translations;
    bs_U32 num_rotations;
    bs_U32 num_scalings;

    int id;
};

struct bs_Animation {
    bs_Model *model;

    bs_U8* data;

    bs_AnimationJoint* joints;

    int joint_count;

    float length;

    char* name;
};

struct bs_Sound {
    void* data;
    void* xaudio;
    bs_U32 size;
};

typedef struct {
    bs_U16 advance_width;
    bs_I16 left_side_bearing;
} bs_LongHorMetric;

typedef struct {
    bs_I16 x, y;
    bool on_curve;
} bs_GlyfPt;

typedef struct {
    bs_U16 num_points;
    bs_U16 num_contours;

    bs_I16 x_min, x_max;
    bs_I16 y_min, y_max;

    bs_U16* contours;
    bs_GlyfPt* coords;

    bs_vec2 tex_coord;
    bs_vec2 tex_offset;
    float width;

    bs_LongHorMetric long_hor_metric;
} bs_Glyph;

// tables
typedef struct {
    char* buf;
    int units_per_em;
    bs_I16 index_to_loc_format;
} bs_Head;

typedef struct {
    char* buf;
    bs_U16 num_glyphs;
} bs_Maxp;

typedef struct {
    char* buf;
    bs_U16 num_of_long_hor_metrics;
} bs_Hhea;

typedef struct {
    char* buf;
} bs_Hmtx;

typedef struct {
    char* buf;
} bs_Loca;

typedef struct {
    char* buf;

    bs_Glyph glyphs[256];
} bs_Glyf;

typedef struct {
    char* buf;
    bs_U32 offset;

    int num_subtables;
    char* subtable;

    bs_U16 format;
    bs_U16 length;
    bs_U16 seg_count_x2;

    bs_U16* format_data;
    bs_U16* end_code;
    bs_U16* start_code;
    bs_I16* id_delta;
    bs_U16* id_range_offset;
} bs_Cmap;

struct bs_Font {
    char* buf;
    bs_U32 offset;
    int data_len;

    float scale;
    bs_U16 table_count;

    // tables
    bs_Head head;
    bs_Maxp maxp;
    bs_Hhea hhea;
    bs_Hmtx hmtx;
    bs_Loca loca;
    bs_Cmap cmap;
    bs_Glyf glyf;

    bs_Texture texture;
};

struct bs_JsonString {
    char* value;
    int len;
};

struct bs_JsonArray {
    union {
        double* as_floats;
        bs_I64* as_ints;
        bs_JsonString* as_strings;
        bs_Json* as_objects;
    };

    int size;
};

enum {
    BS_JSON_STRING = 0,
    BS_JSON_NUMBER,
    BS_JSON_BOOL,
    BS_JSON_SYNTAX,
};

struct bs_JsonToken {
    int offset;
    int len;
    bs_U8 type;
};

struct bs_Json {
    char* token_data;
    int token_len;

    bs_JsonToken* tokens;
    int num_tokens;

    char* raw;
    int raw_len;
};

struct bs_JsonValue {
    union {
        bs_JsonArray as_array;
        bs_Json as_object;
        bs_JsonString as_string;
        bs_I64 as_int;
        double as_float;
    };

    bool found;
    // todo add strlen
};

struct bs_JsonField {
    bs_JsonValue value;

    const char* name;
    int name_len;
    bool is_array;
    bool is_object;
    bool is_int, is_float;
    int length;

    bs_U32 num_elements;
};

struct bs_Gltf {
    bs_JsonArray meshes;
    bs_JsonArray skins;
    bs_JsonArray nodes;
    bs_JsonArray accessors;
    bs_JsonArray buffer_views;
    bs_JsonArray buffers;
    bs_JsonArray animations;

    bs_JsonArray samplers;

    const char* buffer;
    int buffer_size;
};

struct bs_Channel {
    float* inputs;
    float* outputs;
    int num_inputs;
    int node;
    bs_U8 type;
};

struct bs_PerformanceData {
    bs_I64 frequency;
    bs_I64 start, end;
};

struct bs_Plane {
    bs_vec3 point;
    bs_vec3 normal;
};

// Matrix Constants
#define BS_MAT4_IDENTITY { { \
    { 1.0, 0.0, 0.0, 0.0 },  \
    { 0.0, 1.0, 0.0, 0.0 },  \
    { 0.0, 0.0, 1.0, 0.0 },  \
    { 0.0, 0.0, 0.0, 1.0 } } }

#define BS_MAT3_IDENTITY { { \
    { 1.0, 0.0, 0.0 },  \
    { 0.0, 1.0, 0.0 },  \
    { 0.0, 0.0, 1.0 } } }

// Quaternion Constants
#define BS_QUAT_IDENTITY { 0.0, 0.0, 0.0, 1.0 }

// Color Constants
#define BS_BLANK   { 0  , 0  , 0  , 0   }
#define BS_BLACK   { 0  , 0  , 0  , 255 }
#define BS_WHITE   { 255, 255, 255, 255 }

#define BS_RED     { 255, 0  , 0  , 255 }
#define BS_GREEN   { 0  , 255, 0  , 255 }
#define BS_BLUE    { 0  , 0  , 255, 255 }
   
#define BS_CYAN    { 0  , 255, 255, 255 }
#define BS_YELLOW  { 255, 255, 0  , 255 }
#define BS_MAGENTA { 255, 0  , 255, 255 }

#define BS_TRANSLATION 1
#define BS_ROTATION 2
#define BS_SCALE 3

enum bs_RenderType {
    BS_POINTS,
    BS_LINES,
    BS_TRIANGLES,
};

// Error Codes
enum bs_ErrorCode {
    BS_ERROR_OPENGL = 1000,
    BS_ERROR_TEMPORARILY_UNSUPPORTED = 1001,

    // Core
    BS_ERROR_CORE = 2000,

    // Mem
    BS_ERROR_MEM = 3000,
    BS_ERROR_MEM_PATH_NOT_FOUND,
    BS_ERROR_MEM_FAILED_TO_READ,
    BS_ERROR_MEM_ALLOCATION,

    // Models
    BS_ERROR_MODELS = 4000,
    BS_ERROR_MODEL_DATA_TOO_SHORT,
    BS_ERROR_MODEL_UNKNOWN_FORMAT,
    BS_ERROR_MODEL_INVALID_JSON,
    BS_ERROR_MODEL_INVALID_GLTF,
    BS_ERROR_MODEL_INVALID_OPTIONS,
    BS_ERROR_MODEL_NOT_FOUND,
    BS_ERROR_MODEL_IO_ERROR,
    BS_ERROR_MODEL_OUT_OF_MEMORY,
    BS_ERROR_MODEL_LEGACY_GLTF,
    BS_ERROR_MODEL_MAX_ENUM,
    BS_ERROR_MODEL_GLTF_BUFFER_SIZE,

    BS_ERROR_MODEL_ANIMATION_NOT_FOUND,
    BS_ERROR_MODEL_ARMATURE_NOT_FOUND,
    BS_ERROR_MODEL_BONE_NOT_FOUND,
    BS_ERROR_MODEL_MESH_NOT_FOUND,
    BS_ERROR_MODEL_INVALID_FILE_FORMAT,
    BS_ERROR_MODEL_NO_INDICES,

    // Shaders
    BS_ERROR_SHADERS = 5000,
    BS_ERROR_SHADER_NOT_FOUND,
    BS_ERROR_SHADER_COMPILATION,
    BS_ERROR_SHADER_PROGRAM_CREATION,
    BS_ERROR_SHADER_PROGRAM_CREATION_FAIL,
    BS_ERROR_SHADER_SPACE_OUT_OF_RANGE,
    BS_ERROR_SHADER_SPACE_INCOMPATIBLE_FORMAT,

    // Textures
    BS_ERROR_TEXTURES = 6000,

    // TTF
    BS_ERROR_TTF = 7000,
    BS_ERROR_TTF_TABLE_NOT_FOUND,
    BS_ERROR_TTF_UNSUPPORTED_ENCODING,
    BS_ERROR_TTF_UNSUPPORTED_FORMAT,
    BS_ERROR_TTF_GLYPH_NOT_FOUND,
    BS_ERROR_TTF_COMPOUND_GLYPH_UNSUPPORTED,

    // Wnd
    BS_ERROR_WND = 8000,

    // Audio
    BS_ERROR_AUDIO_INITIALIZE = 9000,
    BS_ERROR_AUDIO_CREATE,
    BS_ERROR_AUDIO_VOICE,

    // Json
    BS_ERROR_JSON_UNEXPECTED_CHAR = 10000,
    BS_ERROR_JSON_EXPECTED_CHAR,
    BS_ERROR_JSON_UNKNOWN_DATATYPE,
    BS_ERROR_JSON_FIELD_NOT_FOUND,
    BS_ERROR_JSON_PARSE,
    BS_ERROR_JSON_FAILED_CALCULATION
};

#endif // BS_TYPES_H