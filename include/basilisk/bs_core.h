#ifndef BS_CORE_H
#define BS_CORE_H

#include <bs_types.h>
#include <windows.h>

const char* bs_version();

// Rendering
void bs_pushIndex(int idx);
void bs_pushIndices(int* idxs, int num_elems);
void bs_pushIndexVa(int num_elems, ...);

void bs_pushAttrib(uint8_t **data_ptr, void *data, uint8_t size);
void bs_pushVertex(
    bs_vec3  position,
    bs_vec2  texture,
    bs_vec3  normal,
    bs_RGBA  color,
    bs_ivec4 bone_id,
    bs_vec4  weight,
    bs_U32   entity,
    bs_U32   image
);
bs_BatchPart bs_pushVertices(float* vertices, int num_vertices);
bs_BatchPart bs_pushQuad(bs_Quad quad, bs_RGBA color, bs_Image* image, bs_ShaderEntity* shader_entity);
bs_BatchPart bs_pushTriangle(bs_vec3 a, bs_vec3 b, bs_vec3 c, bs_RGBA color, bs_ShaderEntity* shader_entity);
bs_BatchPart bs_pushLine(bs_vec3 start, bs_vec3 end, bs_RGBA color, bs_ShaderEntity* shader_entity);
bs_BatchPart bs_pushPoint(bs_vec3 pos, bs_RGBA color, bs_ShaderEntity* shader_entity);
bs_BatchPart bs_pushPrimitive(bs_Primitive* prim, int num_vertices, int num_indices, bs_ShaderEntity* shader_entity);
bs_BatchPart bs_pushMesh(bs_Mesh *mesh, int num_vertices, int num_indices, bs_ShaderEntity* shader_entity);
bs_BatchPart bs_pushModel(bs_Model *model, bs_ShaderEntity* shader_entity);
bs_BatchPart bs_pushGlyph(bs_Font* font, bs_Glyph* glyph, bs_vec3 pos, bs_RGBA col, float scale);
bs_BatchPart bs_pushText(bs_Font* font, bs_vec3 pos, bs_RGBA col, float scale, const char* text, va_list args);

// Framebufs
void bs_framebuf(bs_Framebuf* out, bs_vec2 dim);
void bs_setBuf(bs_Texture* buf, int type, int idx);
void bs_attach(void (*tex_func)(bs_Texture* texture, bs_vec2 dim, bs_U8* data));
void bs_attachExisting(bs_Texture* buf, int type);
void bs_attachFromFramebuf(bs_Framebuf *framebuf, int buf);
void bs_attachRenderbuf();
void bs_setDrawBufs(int n, ...);
void bs_selectFramebuf(bs_Framebuf *framebuf);
void bs_pushFramebuf();
bs_Texture* bs_framebufTexture(bs_Framebuf* framebuf, bs_U32 binding);

bs_U16* bs_u16FramebufData(bs_U32 x, bs_U32 y, bs_U32 w, bs_U32 h, bs_U32 buf);
bs_U32* bs_uFramebufData(bs_U32 x, bs_U32 y, bs_U32 w, bs_U32 h, bs_U32 buf);
unsigned char* bs_framebufData(bs_U32 x, bs_U32 y, bs_U32 w, bs_U32 h, bs_U32 buf);
unsigned char* bs_screenshot();
void bs_saveScreenshot(const char *file_name);

// Batches
bs_BatchPart bs_batchRange(bs_U32 offset, bs_U32 num);
void bs_batchShader(bs_Batch* batch, bs_Shader *shader);
void bs_attrib(const int type, unsigned int amount, size_t size_per_type, size_t attrib_size, bool normalized);
void bs_attribI(const int type, unsigned int amount, size_t size_per_type, size_t attrib_size);

bs_Batch bs_batch(bs_Pipeline* pipeline);
bs_Batch* bs_getBatch();
void bs_selectBatch(bs_Batch *batch);
void bs_bindBatch(bs_Batch* batch, int vao_binding, int ebo_binding);
void bs_pushBatch();
void bs_render(bs_BatchPart range, bs_RenderType render_type);
void bs_renderTriangles(bs_BatchPart range);
void bs_renderLines(bs_BatchPart range);
void bs_renderPoints(bs_BatchPart range);
void bs_renderBatchAsText(bs_BatchPart range, bs_Font* font);
void bs_freeBatchData();
void bs_clearBatch();

int bs_batchSize();
void bs_batchResizeCheck(int index_count, int vertex_count);

// Matrices / Cameras
bs_Texture* bs_defTexture();
bs_mat4 bs_persp(float aspect, float fovy, float nearZ, float farZ);
bs_mat4 bs_ortho(int left, int right, int bottom, int top, float nearZ, float farZ);
bs_mat4 bs_lookat(bs_vec3 eye, bs_vec3 center, bs_vec3 up);
bs_mat4 bs_look(bs_vec3 eye, bs_vec3 dir, bs_vec3 up);

void bs_setDefScale(float scale);
float bs_defScale();

void bs_clear(int bit_field);
void bs_viewport(bs_vec2 dim);

#define BS_ADDITIVE 1
#define BS_SUBTRACTIVE 2

#define BS_DEPTH_ALWAYS_SUCCEED 1

#define BS_STENCIL_INCREMENT 0x100
#define BS_STENCIL_FORCE_INCREMENT 0x101
#define BS_STENCIL_DECREMENT 0x102
#define BS_STENCIL_FORCE_DECREMENT 0x103

#define BS_STENCIL_DISCARD_WHERE_ZERO 0x200
#define BS_STENCIL_DISCARD_WHERE_NON_ZERO 0x201

#define BS_STENCIL_SHADOWS 0x300

enum {
    BS_BUFFER_COLOR = 8,
    BS_BUFFER_DEPTH,
    BS_BUFFER_STENCIL
};

#define BS_CULLING 0x0B44
#define BS_DEPTH_CLAMP 0x864F

#define GL_UBYTE GL_UNSIGNED_BYTE
#define GL_UINT GL_UNSIGNED_INT
#define GL_USHORT GL_UNSIGNED_SHORT

// Constants
#define BS_BATCH_INCR_BY 256

// Clear Buffer Codes
#define BS_DEPTH_BUFFER_BIT 0x00000100
#define BS_STENCIL_BUFFER_BIT 0x00000400
#define BS_COLOR_BUFFER_BIT 0x00004000

#endif // BS_CORE_H