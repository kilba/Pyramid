// GL
#include <glad/glad.h>
#include <cglm/cglm.h>

// Basilisk
#include <basilisk.h>

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <lodepng.h>
#include <time.h>
#include <stdarg.h>

#include <assert.h>

bs_Texture def_texture;
bs_Batch *curr_batch = NULL;
bs_Framebuf *curr_framebuf = NULL;

bs_U32 curr_FBO = 0;

float def_scale = 1.0;
int curr_program = -1;

const char* bs_version() {
    return BS_VERSION;
}

bs_Texture *bs_defTexture() {
    return &def_texture;
}

void bs_setDefScale(float scale) {
    def_scale = scale;
}

float bs_defScale() {
    return def_scale;
}

// Cameras
bs_mat4 bs_persp(float aspect, float fovy, float nearZ, float farZ) {
    bs_mat4 m;
    glm_perspective(glm_rad(fovy), aspect, nearZ, farZ, m.a);
    return m;
}

bs_mat4 bs_ortho(int left, int right, int bottom, int top, float nearZ, float farZ) {
    bs_mat4 m;
    glm_ortho(left, right, bottom, top, nearZ, farZ, m.a);
    return m;
}

bs_mat4 bs_lookat(bs_vec3 eye, bs_vec3 center, bs_vec3 up) {
    bs_mat4 m;
    glm_lookat(eye.a, center.a, up.a, m.a);
    return m;
}

bs_mat4 bs_look(bs_vec3 eye, bs_vec3 dir, bs_vec3 up) {
    bs_mat4 m;
    glm_look(eye.a, dir.a, up.a, m.a);
    return m;
}

// Rendering
inline bs_U8* bs_batchData(bs_Batch* batch, int index, int offset) {
    return batch->vertex_buf.data + (batch->vertex_buf.unit_size * index) + offset;
}

void bs_pushIndex(int idx) {
    idx += curr_batch->vertex_buf.size;
    memcpy(
	    (int *)(curr_batch->index_buf.data) + curr_batch->index_buf.size++, 
	    &idx, 
	    sizeof(bs_U32)
    );
}

void bs_pushIndices(int *idxs, int num_elems) {
    for (int i = 0; i < num_elems; i++) {
        bs_pushIndex(idxs[i]);
    }
}

void bs_pushIndexVa(int num_elems, ...) {
    va_list ptr;
    va_start(ptr, num_elems);

    for (int i = 0; i < num_elems; i++) {
        bs_pushIndex(va_arg(ptr, int));
    }

    va_end(ptr);
}

inline void bs_pushAttrib(uint8_t **data_ptr, void *data, uint8_t size) {
    memcpy(*data_ptr, data, size);
    *data_ptr += size;
}

void bs_pushVertex(
    bs_vec3  position,
    bs_vec2  texture,
    bs_vec3  normal,
    bs_RGBA  color,
    bs_ivec4 bone_id,
    bs_vec4  weight,
    bs_U32   entity,
    bs_U32   image
) {
    uint8_t* data_ptr = bs_bufferData(&curr_batch->vertex_buf, curr_batch->vertex_buf.size);
    bs_AttributeSizes* attrib_sizes = &curr_batch->shader.vs->attrib_sizes;

    bs_pushAttrib(&data_ptr, &position, attrib_sizes->position);
    bs_pushAttrib(&data_ptr, &texture, attrib_sizes->texture);
    bs_pushAttrib(&data_ptr, &color, attrib_sizes->color);
    bs_pushAttrib(&data_ptr, &normal, attrib_sizes->normal);
    bs_pushAttrib(&data_ptr, &bone_id, attrib_sizes->bone_id);
    bs_pushAttrib(&data_ptr, &weight, attrib_sizes->weight);
    bs_pushAttrib(&data_ptr, &entity, attrib_sizes->entity);
    bs_pushAttrib(&data_ptr, &image, attrib_sizes->image);

    curr_batch->vertex_buf.size++;
}

bs_BatchPart bs_pushVertices(float* vertices, int num_vertices) {
    bs_batchResizeCheck(0, num_vertices);
    uint8_t* data_ptr = bs_bufferData(&curr_batch->vertex_buf, curr_batch->vertex_buf.size);
    memcpy(data_ptr, vertices, num_vertices * curr_batch->shader.vs->attrib_size_bytes);
    curr_batch->vertex_buf.size += num_vertices;

    return (bs_BatchPart) { curr_batch->index_buf.size, curr_batch->index_buf.size };
}

bs_BatchPart bs_pushQuad(bs_Quad quad, bs_RGBA color, bs_Image* image, bs_ShaderEntity* shader_entity) {
    bs_U32 ent = shader_entity == NULL ? 0 : shader_entity->buffer_location;
    bs_U32 img = image         == NULL ? 0 : image->buffer_location;

    bs_batchResizeCheck(6, 4);
    bs_pushIndexVa(6, 0, 1, 2, 2, 1, 3);

    bs_pushVertex(quad.a, quad.ca, bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0), ent, img);
    bs_pushVertex(quad.b, quad.cb, bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0), ent, img);
    bs_pushVertex(quad.c, quad.cc, bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0), ent, img);
    bs_pushVertex(quad.d, quad.cd, bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0), ent, img);

    return (bs_BatchPart) { curr_batch->index_buf.size, curr_batch->index_buf.size };
}

bs_BatchPart bs_pushTriangle(bs_vec3 a, bs_vec3 b, bs_vec3 c, bs_RGBA color, bs_ShaderEntity* shader_entity) {
    bs_U32 entity = shader_entity == NULL ? 0 : shader_entity->buffer_location;

    bs_batchResizeCheck(3, 3);
    bs_pushIndexVa(3, 0, 1, 2),

    bs_pushVertex(a, bs_v2(0.0, 0.0), bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0.0), entity, 0);
    bs_pushVertex(b, bs_v2(1.0, 0.0), bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0.0), entity, 0);
    bs_pushVertex(c, bs_v2(0.0, 1.0), bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0.0), entity, 0);

    return (bs_BatchPart){ curr_batch->index_buf.size, curr_batch->index_buf.size };
}

bs_BatchPart bs_pushLine(bs_vec3 start, bs_vec3 end, bs_RGBA color, bs_ShaderEntity* shader_entity) {
    bs_U32 entity = shader_entity == NULL ? 0 : shader_entity->buffer_location;

    bs_batchResizeCheck(2, 2);
    bs_pushIndexVa(2, 0, 1),

    bs_pushVertex(start, bs_v2(0.0, 0.0), bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0.0), entity, 0);
    bs_pushVertex(end, bs_v2(1.0, 0.0), bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0.0), entity, 0);

    return (bs_BatchPart){ curr_batch->index_buf.size, curr_batch->index_buf.size };
}

bs_BatchPart bs_pushPoint(bs_vec3 pos, bs_RGBA color, bs_ShaderEntity* shader_entity) {
    bs_U32 entity = shader_entity == NULL ? 0 : shader_entity->buffer_location;

    bs_batchResizeCheck(1, 1);
    bs_pushIndexVa(1, 0);
    bs_pushVertex(pos, bs_v2s(0.0), bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0.0), entity, 0);

    return (bs_BatchPart){ curr_batch->index_buf.size, curr_batch->index_buf.size };
}

bs_BatchPart bs_pushPrimitive(bs_Primitive* primitive, int num_vertices, int num_indices, bs_ShaderEntity* shader_entity) {
    bs_U32 entity = shader_entity == NULL ? 0 : shader_entity->buffer_location;

    bs_batchResizeCheck(bs_max(primitive->num_indices, num_indices), bs_max(primitive->num_vertices, num_vertices));
    bs_pushIndices(primitive->indices, primitive->num_indices);

    float *vertex = primitive->vertices;
    for(int i = 0; i < primitive->num_vertices; i++, vertex += primitive->vertex_size) {
        bs_pushVertex(
            *(bs_vec3  *)(vertex),
	        *(bs_vec2  *)(vertex + primitive->offset_tex),
	        *(bs_vec3  *)(vertex + primitive->offset_nor),
	         (bs_RGBA)BS_WHITE,
	        *(bs_ivec4 *)(vertex + primitive->offset_bid),
	        *(bs_vec4  *)(vertex + primitive->offset_wei),
            entity,
            0
        );
    }

    return (bs_BatchPart){ curr_batch->index_buf.size, curr_batch->index_buf.size };
}

bs_BatchPart bs_pushMesh(bs_Mesh* mesh, int num_vertices, int num_indices, bs_ShaderEntity* shader_entity) {
    bs_BatchPart batch_part = (bs_BatchPart){ curr_batch->index_buf.size, 0 };
    for(int i = 0; i < mesh->num_primitives; i++) {
	    batch_part.num += bs_pushPrimitive(mesh->primitives + i, num_vertices, num_indices, shader_entity).num;
    }

    return batch_part;
}

bs_BatchPart bs_pushModel(bs_Model* model, bs_ShaderEntity* shader_entity) {
    bs_BatchPart batch_part = (bs_BatchPart){ curr_batch->index_buf.size, 0 };

    for(int i = 0; i < model->num_meshes; i++) {
        batch_part.num += bs_pushMesh(model->meshes + i, model->num_vertices, model->num_indices, shader_entity).num;
    }

    return batch_part;
}

bs_BatchPart bs_pushGlyph(bs_Font* font, bs_Glyph* glyph, bs_vec3 pos, bs_RGBA col, float scale) {
    bs_BatchPart batch_part = (bs_BatchPart){ curr_batch->index_buf.size, 0 };

    pos.y += (float)glyph->y_min * font->scale;

    bs_Quad quad = bs_quad(pos, bs_v2muls(bs_v2(glyph->width, BS_TTF_DIM), scale));
    bs_quadTextureCoords(&quad, glyph->tex_offset, glyph->tex_coord);

    return bs_pushQuad(quad, col, NULL, NULL);
}

bs_BatchPart bs_pushText(bs_Font* font, bs_vec3 pos, bs_RGBA col, float scale, const char* text, va_list args) {
    bs_BatchPart batch_part = (bs_BatchPart){ curr_batch->index_buf.size, 0 };

    char buf[512];
    int len = vsprintf(buf, text, args);

    float original_x = pos.x;
    for (int i = 0; i < len; i++) {
        bs_Glyph* glyph = bs_glyph(font, buf[i]);

        switch (buf[i]) {
        case ' ': pos.x += BS_TTF_DIM * scale / 3.0; break;
        case '\n': {
            pos.y -= BS_TTF_DIM * scale;
            pos.x = original_x;
        } break;
        default: {
            if (buf[i] != ' ') {
                batch_part.num += bs_pushGlyph(font, glyph, pos, col, scale).num;
                pos.x += (float)glyph->long_hor_metric.advance_width * font->scale * scale;
            }
        } break;
        }
    }

    return batch_part;
}

void bs_batchResizeCheck(int index_count, int vertex_count) {
    bs_bufferResizeCheck(&curr_batch->vertex_buf, vertex_count);
    bs_bufferResizeCheck(&curr_batch->index_buf, index_count);
}

void bs_batchShader(bs_Batch* batch, bs_Shader* shader) {
    batch->shader = *shader;
}

void bs_batch(bs_Batch* out, bs_Shader* shader) {
    memset(out, 0, sizeof(bs_Batch));

    // Default values
    out->shader = *shader;
    out->use_indices = true;

    // Create buffer/array objects
    glGenVertexArrays(1, &out->VAO);
    glGenBuffers(1, &out->VBO);
    glGenBuffers(1, &out->EBO);

    bs_selectBatch(out);

    // Attribute setup
    struct AttribData {
        int type;
        int count;
        int size;
        bool normalized;
    } attrib_data[] = {
        { GL_FLOAT, 3, sizeof(bs_vec3) , false }, // Position
        { GL_FLOAT, 2, sizeof(bs_vec2) , false }, // Texture Coordinates
        { GL_UBYTE, 4, sizeof(bs_RGBA) , true  }, // Color
        { GL_FLOAT, 3, sizeof(bs_vec3) , false }, // Normal
        { GL_INT  , 4, sizeof(bs_ivec4), false }, // Bone Ids
        { GL_FLOAT, 4, sizeof(bs_vec4) , false }, // Weights
        { GL_UINT , 1, sizeof(bs_uint) , false }  // Entity
    };
    int total_attrib_count = sizeof(attrib_data) / sizeof(struct AttribData);

    // Calculate attrib sizes
    int attrib_size = 0;
    for (int i = 0, j = 1; i < total_attrib_count; i++, j *= 2) {
        if ((out->shader.vs->attribs & j) != j) continue;

        attrib_size += attrib_data[i].size;
    }

    // Add attributes
    for(int i = 0, j = 1; i < total_attrib_count; i++, j *= 2) {
        if ((out->shader.vs->attribs & j) != j) continue;

        struct AttribData* data = &attrib_data[i];
        bs_attrib(data->type, data->count, data->size, attrib_size, data->normalized);
    }
    
    out->vertex_buf = bs_buffer(GL_ARRAY_BUFFER, attrib_size, BS_BATCH_INCR_BY, 0, 0);
    out->index_buf  = bs_buffer(GL_ELEMENT_ARRAY_BUFFER, sizeof(bs_U32), BS_BATCH_INCR_BY, 0, 0);
}

bs_Batch* bs_getBatch() {
    return curr_batch;
}

void bs_selectBatch(bs_Batch* batch) {
    curr_batch = batch;

    if (batch->shader.id == 0) return;

    glBindVertexArray(batch->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, batch->VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch->EBO);

    bs_errorData(GL_SHADER, batch->shader.id);
    glUseProgram(batch->shader.id);
}

void bs_bindBatch(bs_Batch* batch, int vao_binding, int ebo_binding) {
    if (vao_binding != -1) glBindBufferBase(GL_SHADER_STORAGE_BUFFER, vao_binding, batch->VBO);
    if (ebo_binding != -1) glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ebo_binding, batch->EBO);
}

void bs_attribI(const int type, unsigned int amount, size_t size_per_type, size_t attrib_size) {
    bs_Batch *batch = curr_batch;

    glEnableVertexAttribArray(batch->attrib_count);
    glVertexAttribIPointer(batch->attrib_count++, amount, type, attrib_size, (void*)batch->attrib_offset);

    batch->attrib_offset += size_per_type;
}

void bs_attrib(const int type, unsigned int amount, size_t size_per_type, size_t attrib_size, bool normalized) {
    bs_Batch* batch = curr_batch;

    if((type >= GL_SHORT) && (type <= GL_UINT)) {
        bs_attribI(type, amount, size_per_type, attrib_size);
        return;
    }

    glEnableVertexAttribArray(batch->attrib_count);
    glVertexAttribPointer(batch->attrib_count++, amount, type, normalized, attrib_size, (void*)batch->attrib_offset);

    batch->attrib_offset += size_per_type;
}

// Pushes all vertex_buf.data to VRAM
void bs_pushBatch() {
    bs_Batch *batch = curr_batch;
    glBufferSubData(GL_ARRAY_BUFFER, 0, batch->vertex_buf.size * batch->vertex_buf.unit_size, batch->vertex_buf.data);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, batch->index_buf.size * sizeof(int), batch->index_buf.data);
}

void bs_freeBatchData() {
    free(curr_batch->vertex_buf.data);
    free(curr_batch->index_buf.data);

    curr_batch->vertex_buf.data = NULL;
    curr_batch->index_buf.data = NULL;
}

bs_BatchPart bs_batchRange(bs_U32 offset, bs_U32 num) {
    return (bs_BatchPart){ offset, num };
}

void bs_render(bs_BatchPart range, bs_RenderType render_type) {
    if (curr_batch->shader.id == 0) return;

    if (curr_program != curr_batch->shader.id) {
        glUseProgram(curr_batch->shader.id);
        curr_program = curr_batch->shader.id;
    }

    int mode = GL_TRIANGLES;
    switch (render_type) {
        case BS_TRIANGLES: mode = GL_TRIANGLES; break;
        case BS_LINES: mode = GL_LINES; break;
        case BS_POINTS: mode = GL_POINTS; break;
    }

    if(curr_batch->use_indices) glDrawElements(mode, range.num, GL_UINT, (void*)(range.offset * sizeof(GLuint)));
    else glDrawArrays(mode, range.offset, range.num);
}

void bs_renderTriangles(bs_BatchPart range) {
    bs_render(range, BS_TRIANGLES);
}

void bs_renderLines(bs_BatchPart range) {
    bs_render(range, BS_LINES);
}

void bs_renderPoints(bs_BatchPart range) {
    bs_render(range, BS_POINTS);
}

void bs_renderBatchAsText(bs_BatchPart range, bs_Font* font) {
    bs_selectTexture(&font->texture, 0);
    bs_render(range, BS_TRIANGLES);
}

void bs_clearBatch() {
    curr_batch->vertex_buf.size = 0;
    curr_batch->index_buf.size = 0;
}

int bs_batchSize() {
    return (curr_batch->use_indices) ? curr_batch->index_buf.size : curr_batch->vertex_buf.size;
}

// Framebufs
void bs_framebuf(bs_Framebuf* out, bs_vec2 dim) {
    curr_framebuf = out;

    out->dim = dim;
    out->clear = GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    out->buf = bs_buffer(0, sizeof(bs_Texture), 4, 4, 0);

    glGenFramebuffers(1, &out->FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, out->FBO);
}

void bs_setBuf(bs_Texture* buf, int type, int idx) {
    bs_Framebuf *framebuf = curr_framebuf;

    glFramebufferTexture2D(GL_FRAMEBUFFER, type, GL_TEXTURE_2D, buf->id, 0);
    memcpy((bs_Texture *)(framebuf->buf.data) + idx, buf, sizeof(bs_Texture));
}

void bs_attachExisting(bs_Texture* buf, int type) {
    bs_Framebuf *framebuf = curr_framebuf;

    bs_bufferResizeCheck(&framebuf->buf, 1);
    bs_setBuf(buf, type, framebuf->buf.size);

    framebuf->buf.size++;
}

void bs_attachFromFramebuf(bs_Framebuf *framebuf, int buf) {
    bs_Texture* texture = bs_bufferData(&framebuf->buf, buf);
    bs_attachExisting(texture, texture->attachment);
}

void bs_attach(void (*tex_func)(bs_Texture* texture, bs_vec2 dim, bs_U8* data)) {
    bs_Framebuf* framebuf = curr_framebuf;
    bs_bufferResizeCheck(&framebuf->buf, 1);

    bs_Texture* buf = (bs_Texture*)(framebuf->buf.data) + framebuf->buf.size;
    tex_func(buf, framebuf->dim, NULL);

    // Add attachment offset only to GL_COLOR_ATTACHMENT0
    int attachment = buf->attachment + ((buf->attachment == GL_COLOR_ATTACHMENT0) ? framebuf->buf.size : 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, buf->id, 0);

    framebuf->buf.size++;
}

void bs_attachRenderbuf() {
    bs_Framebuf *framebuf = curr_framebuf;

    glGenRenderbuffers(1, &framebuf->RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, framebuf->RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, framebuf->dim.x, framebuf->dim.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, framebuf->RBO); 
}

void bs_setDrawBufs(int n, ...) {
    if (n > BS_MAX_ATTACHMENTS) return; // todo log

    GLenum values[8];
    va_list ptr;

    va_start(ptr, n);
    for(int i = 0; i < n; i++) {
        values[i] = GL_COLOR_ATTACHMENT0 + va_arg(ptr, int);
    }
    va_end(ptr);

    glDrawBuffers(n, values);
}

void bs_selectFramebuf(bs_Framebuf *framebuf) {
    if(framebuf == NULL) {
	    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	    return;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, framebuf->dim.x, framebuf->dim.y);

    if(curr_FBO != framebuf->FBO) {
	    glBindFramebuffer(GL_FRAMEBUFFER, framebuf->FBO);
	    curr_FBO = framebuf->FBO;
    }
}

void bs_pushFramebuf() {
    glDisable(GL_DEPTH_TEST);

    if(curr_FBO != 0) {
	    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	    curr_FBO = 0;
    }

    bs_vec2 res = bs_resolution();
    glViewport(0, 0, res.x, res.y);
}

bs_Texture* bs_framebufTexture(bs_Framebuf* framebuf, bs_U32 binding) {
    assert(binding < framebuf->buf.size);
    return ((bs_Texture*)framebuf->buf.data) + binding;
}

void* bs_framebufPixels(bs_U32 x, bs_U32 y, bs_U32 w, bs_U32 h, bs_U32 buf, bs_U32 channel, bs_U32 datatype, size_t size) {
    void* data = bs_alloc(w * h * size);
    glReadBuffer(GL_COLOR_ATTACHMENT0 + buf);
    glReadPixels(x, y, w, h, channel, datatype, data);
    return data;
}

bs_U16 *bs_u16FramebufData(bs_U32 x, bs_U32 y, bs_U32 w, bs_U32 h, bs_U32 buf) {
    return bs_framebufPixels(x, y, w, h, buf, GL_RED_INTEGER, GL_USHORT, sizeof(bs_U16));
}

bs_U32 *bs_uFramebufData(bs_U32 x, bs_U32 y, bs_U32 w, bs_U32 h, bs_U32 buf) {
    return bs_framebufPixels(x, y, w, h, buf, GL_RED_INTEGER, GL_UINT, sizeof(bs_U32));
}

unsigned char *bs_framebufData(bs_U32 x, bs_U32 y, bs_U32 w, bs_U32 h, bs_U32 buf) {
    return bs_framebufPixels(x, y, w, h, buf, GL_RGB, GL_UBYTE, 3);
}

unsigned char *bs_screenshot() {
    bs_vec2 res = curr_framebuf->dim;
    return bs_framebufData(0, 0, res.x, res.y, GL_FRONT);
}

void bs_saveScreenshot(const char *file_name) {
    unsigned char *data = bs_screenshot();
    bs_vec2 res = curr_framebuf->dim;
    lodepng_encode24_file(file_name, data, res.x, res.y);
    free(data);
}

void bs_clear(int bit_field) {
    glClear(bit_field);
}

void bs_viewport(bs_vec2 dim) {
    glViewport(0, 0, dim.x, dim.y);
}