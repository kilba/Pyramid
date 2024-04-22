#ifndef BS_TEXTURES_H
#define BS_TEXTURES_H

#include <bs_types.h>

void bs_saveTexture32(const char *name, unsigned char *data, int w, int h);

void bs_pushImageBuffers();
void bs_pushImages();
bs_Image* bs_image(char* path, bs_U32 num_frames);
bs_Image* bs_imageTtfAtlas(int dim, bs_U8* data);
bs_Image* bs_getLastImage();
void bs_pushImages();

void bs_texture(bs_Texture* texture, bs_vec2 dim, int type);
bs_U32 bs_textureDataFile(const char *path, bool update_dimensions);
void bs_pushTexture(int internal_format, int format, int type);
void bs_textureMinMag(int min_filter, int mag_filter);

void bs_selectTexture(bs_Texture* texture, bs_U32 tex_unit);

void bs_textureArray(bs_Texture* tex, bs_vec2 max_dim, int num_textures);
bs_U32 bs_textureArrayAppendPNG(const char *path);

void bs_linearFiltering();
void bs_nearestFiltering();

void bs_depthStencil(bs_Texture* texture, bs_vec2 dim, bs_U8* data);
void bs_depth(bs_Texture* texture, bs_vec2 dim, bs_U8* data);
void bs_textureR(bs_Texture* texture, bs_vec2 dim, bs_U8* data);
void bs_textureRG(bs_Texture* texture, bs_vec2 dim, bs_U8* data);
void bs_textureRGB(bs_Texture* texture, bs_vec2 dim, bs_U8* data);
void bs_textureRGB16f(bs_Texture* texture, bs_vec2 dim, bs_U8* data);
void bs_textureRGB32f(bs_Texture* texture, bs_vec2 dim, bs_U8* data);
void bs_textureRGBA(bs_Texture* texture, bs_vec2 dim, bs_U8* data);
void bs_textureRGBA16f(bs_Texture* texture, bs_vec2 dim, bs_U8* data);
void bs_textureRGBA32f(bs_Texture* texture, bs_vec2 dim, bs_U8* data);
void bs_texture_11_11_10(bs_Texture* texture, bs_vec2 dim, bs_U8* data);
void bs_textureR16U(bs_Texture* texture, bs_vec2 dim, bs_U8* data);

#endif // BS_TEXTURES_H