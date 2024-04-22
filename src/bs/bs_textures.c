// GL
#include <glad/glad.h>
#include <cglm/cglm.h>

// Basilisk
#include <bs_shaders.h>
#include <bs_core.h>
#include <bs_math.h>
#include <bs_mem.h>
#include <bs_textures.h>

#include <lodepng.h>

// STD
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <windows.h>
#include <winreg.h>

bs_Texture* curr_texture;
int filter = GL_NEAREST;

bs_Buffer image_buf;
bs_Buffer image_data_buf;

bs_Space image_shader_space;

// Space Communication-
void bs_pushImageBuffers() {
    image_buf = bs_buffer(0, sizeof(bs_Image), 16, 32, 0);
    image_data_buf = bs_buffer(0, sizeof(bs_ImageShaderData), 16, 32, 0x4000 / sizeof(bs_ImageShaderData));

    bs_image(NULL, 0);
}

void bs_pushImages() {
    image_shader_space = bs_shaderSpace(image_data_buf, BS_SPACE_IMAGES, BS_STD140);
}

bs_Image* bs_getLastImage() {
    return bs_bufferData(&image_buf, image_buf.size - 1);
}
//-Space Communication

void bs_saveTexture32(const char* name, unsigned char* data, int w, int h) {
    lodepng_encode32_file(name, data, w, h);
}

// Texture Initialization
void bs_texture(bs_Texture *texture, bs_vec2 dim, int type) {
    memset(texture, 0, sizeof(bs_Texture));

    glGenTextures(1, &texture->id);
    glBindTexture(type, texture->id);

    curr_texture = texture;
    curr_texture->num_frames = 1;

    curr_texture->w = dim.x;
    curr_texture->h = dim.y;

    curr_texture->texw = 1.0;
    curr_texture->texh = 1.0;

    curr_texture->type = type;
}

bs_U32 bs_textureDataFile(const char* path, bool update_dimensions) {
    if (path == NULL) return 0;
    unsigned int w, h;
    bs_U32 err = lodepng_decode32_file(&curr_texture->data, &w, &h, path);

    if(err != 0) return err;

    if(update_dimensions) {
        curr_texture->w = w;
        curr_texture->h = h;
    }

    return 0;
}

void bs_textureMinMag(int min_filter, int mag_filter) {
    glTexParameteri(curr_texture->type, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(curr_texture->type, GL_TEXTURE_MAG_FILTER, mag_filter);
}

void bs_pushTexture(int internal_format, int format, int type) {
    glTexImage2D(
        curr_texture->type,
        0,
        internal_format,
        curr_texture->w,
        curr_texture->h,
        0,
        format,
        type,
        curr_texture->data
    );
}

bs_Image* bs_imageTtfAtlas(int dim, bs_U8* data) {
    bs_Image* img = bs_bufferAppend(&image_buf, NULL);
    bs_ImageShaderData* img_data = bs_bufferAppend(&image_data_buf, NULL);

    // create texture
    bs_Texture tex;
    bs_texture(&tex, bs_v2s(dim), GL_TEXTURE_2D);
    bs_textureMinMag(GL_NEAREST, GL_NEAREST);
    tex.data = data;
    bs_pushTexture(GL_RGBA, GL_RED, GL_UBYTE);

    // get handle from texture
    bs_U64 handle = glGetTextureHandleARB(tex.id);
    glMakeTextureHandleResidentARB(handle);

    img_data->handle = handle;
    img_data->coords = bs_v2s(0.0);

    img->data = img_data;
    img->buffer_location = image_buf.size - 1;

    return img;
}

bs_Image* bs_image(char* path, bs_U32 num_frames) {
    bs_Image* img = bs_bufferAppend(&image_buf, NULL);
    bs_ImageShaderData* img_data = bs_bufferAppend(&image_data_buf, NULL);

    // create texture
    bs_Texture tex;
    bs_texture(&tex, bs_v2s(0), GL_TEXTURE_2D);
    bs_textureMinMag(GL_NEAREST, GL_NEAREST);
    if (path != NULL) {
        bs_textureDataFile(path, true);
    }
    else {
        // todo remove malloc
        bs_RGBA color = BS_WHITE;
        tex.data = bs_alloc(sizeof(bs_RGBA));
        memcpy(tex.data, &color, sizeof(bs_RGBA));
        tex.w = tex.h = 1;
    }
    bs_pushTexture(GL_RGBA, GL_RGBA, GL_UBYTE);

    // get handle from texture
    bs_U64 handle = glGetTextureHandleARB(tex.id);
    glMakeTextureHandleResidentARB(handle);

    img_data->handle = handle;
    img_data->coords = bs_v2s(0.0);

    img->data = img_data;
    img->buffer_location = image_buf.size - 1;

    return img;
}

void bs_selectTexture(bs_Texture* texture, bs_U32 tex_unit) {
    if (curr_texture == texture) return;

    curr_texture = texture;
    glActiveTexture(GL_TEXTURE0 + tex_unit);
    glBindTexture(texture->type, texture->id);
}

void bs_textureArray(bs_Texture *tex, bs_vec2 max_dim, int num_textures) {
    bs_texture(tex, max_dim, GL_TEXTURE_2D_ARRAY);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, max_dim.x, max_dim.y, num_textures);
    bs_textureMinMag(GL_NEAREST, GL_NEAREST);
}

bs_U32 bs_textureArrayAppendPNG(const char *path) {
    bs_U32 err = bs_textureDataFile(path, true);
    if (err != 0) return err;

    glTexSubImage3D(
        curr_texture->type,
        0,
        0, 0, curr_texture->frame.z,
        curr_texture->w, curr_texture->h, 1,
        GL_RGBA,
        GL_UBYTE,
        curr_texture->data
    );

    curr_texture->frame.z++;
    free(curr_texture->data);

    return 0;
}

void bs_linearFiltering() {
    filter = GL_LINEAR;
}

void bs_nearestFiltering() {
    filter = GL_NEAREST;
}

static void bs_textureColor(bs_Texture *texture, bs_vec2 dim, int internal_format, int format, int type, bs_U8* data) {
    bs_texture(texture, dim, GL_TEXTURE_2D);
    bs_textureMinMag(filter, filter);
    texture->data = data;
    bs_pushTexture(internal_format, format, type);
    texture->attachment = GL_COLOR_ATTACHMENT0;
}

// Maybe make these macros?
void bs_depthStencil(bs_Texture *texture, bs_vec2 dim, bs_U8* data) {
    bs_textureColor(texture, dim, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, data);
    texture->attachment = GL_DEPTH_STENCIL_ATTACHMENT;
}

void bs_depth(bs_Texture *texture, bs_vec2 dim, bs_U8* data) {
    bs_textureColor(texture, dim, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, data);
    texture->attachment = GL_DEPTH_ATTACHMENT;
}

void bs_textureR(bs_Texture* texture, bs_vec2 dim, bs_U8* data) { bs_textureColor(texture, dim, GL_R8, GL_RED, GL_UNSIGNED_BYTE, data); }
void bs_textureRG(bs_Texture *texture, bs_vec2 dim, bs_U8* data) { bs_textureColor(texture, dim, GL_RG8, GL_RG, GL_UNSIGNED_BYTE, data); }
void bs_textureRGB(bs_Texture *texture, bs_vec2 dim, bs_U8* data) { bs_textureColor(texture, dim, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, data); }
void bs_textureRGB16f(bs_Texture *texture, bs_vec2 dim, bs_U8* data) { bs_textureColor(texture, dim, GL_RGB16F, GL_RGB, GL_FLOAT, data); }
void bs_textureRGB32f(bs_Texture *texture, bs_vec2 dim, bs_U8* data) { bs_textureColor(texture, dim, GL_RGB32F, GL_RGB, GL_FLOAT, data); }
void bs_textureRGBA(bs_Texture *texture, bs_vec2 dim, bs_U8* data) { bs_textureColor(texture, dim, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, data); }
void bs_textureRGBA16f(bs_Texture *texture, bs_vec2 dim, bs_U8* data) { bs_textureColor(texture, dim, GL_RGBA16F, GL_RGBA, GL_FLOAT, data); }
void bs_textureRGBA32f(bs_Texture *texture, bs_vec2 dim, bs_U8* data) { bs_textureColor(texture, dim, GL_RGBA32F, GL_RGBA, GL_FLOAT, data); }
void bs_texture_11_11_10(bs_Texture *texture, bs_vec2 dim, bs_U8* data) { bs_textureColor(texture, dim, GL_R11F_G11F_B10F, GL_RGB, GL_FLOAT, data); }
void bs_textureR16U(bs_Texture *texture, bs_vec2 dim, bs_U8* data) { bs_textureColor(texture, dim, GL_RGB16UI, GL_RED_INTEGER, GL_UNSIGNED_INT, data); }