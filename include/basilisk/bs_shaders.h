#ifndef BS_SHADERS_H
#define BS_SHADERS_H

#include <stdbool.h>
#include <bs_types.h>

#define BS_SPACE_ARMATURES 0
#define BS_SPACE_ENTITIES 1
#define BS_SPACE_GLOBALS 2
#define BS_SPACE_IMAGES 3
#define BS_SPACE_RESERVED_04 4
#define BS_SPACE_RESERVED_05 5
#define BS_SPACE_RESERVED_06 6
#define BS_SPACE_RESERVED_07 7
#define BS_SPACE_RESERVED_08 8

typedef enum bs_SpaceType bs_SpaceType;

enum bs_SpaceType {
	BS_STD140,
	BS_STD430
};

void bs_pushShaderBuffers();
void bs_pushShaderSpaces();

/// <summary>
/// Used to efficiently update data of 3D models. <para/>
/// Example: When pushing a model to the shader with bs_pushModel() and using a shader entity,
/// the transformation of that model can then be updated by changing bs_ShaderEntityData::transform, 
/// setting bs_ShaderEntityData::changed to true and calling bs_pushEntityShaderDataBuffer().
/// </summary>
/// <param name="transformation">= BS_MAT4_IDENTITY</param>
/// <returns>A shader entity storage object.</returns>
bs_ShaderEntity* bs_pushShaderEntity(bs_mat4 transformation);

/// <summary>
/// Pushes all shader entities with bs_ShaderEntity::changed == true to VRAM and sets them to false, avoid calling often.
/// </summary>
void bs_pushEntityShaderDataBuffer();

// Shader Spaces
/// <summary>
/// Copies data into a new buffer in VRAM, this is the GPU equivalent of malloc().<para/>
/// The following sample shows how a shader space is declared in the shader:<para/>
/// layout(std430, binding = 4) buffer { vec3 pos, vec4 rot, ... }
/// </summary>
/// <param name="buffer">- The buffer created with bs_buffer().</param>
/// <param name="binding">- The unique bind point of the shader space.</param>
/// <param name="type">- BS_STD140: Max 16kb, faster. BS_STD430: Any size, slower.</param>
/// <returns>A new shader space object.</returns>
bs_Space 
bs_shaderSpace(
	bs_Buffer buffer, 
	bs_U32 binding, 
	bs_SpaceType type
);

/// <summary>
/// Copies data into an existing buffer in VRAM.
/// </summary>
/// <param name="shader_space">- The shader space created with bs_shaderSpace()</param>
/// <param name="data"></param>
/// <param name="offset"></param>
/// <param name="num_units"></param>
void 
bs_updateShaderSpace(
	bs_Space* shader_space, 
	void* data, 
	bs_U32 index, 
	bs_U32 num_units
);

/// <summary>
/// Fetches data from a buffer in VRAM
/// </summary>
/// <param name="shader_space"></param>
/// <param name="offset"></param>
/// <param name="size"></param>
/// <param name="out"></param>
void
bs_fetchShaderSpace(
	bs_Space* shader_space,
	bs_U32 offset,
	bs_U32 size,
	void* out
);

/// <summary>
/// Creates a shader from a vertex shader and fragment shader.
/// </summary>
/// <param name="vs"></param>
/// <param name="fs"></param>
/// <param name="gs">= NULL</param>
/// <param name="name">= NULL</param>
/// <returns>A new shader object.</returns>
bs_Shader 
bs_shader(
	bs_VertexShader* vs, 
	bs_FragmentShader* fs, 
	bs_GeometryShader* gs,
	const char* name
);

/// <summary>
/// Compiles a vertex shader from a file path.
/// </summary>
/// <param name="path">- Path to the vertex shader.</param>
/// <param name="error">- Compilation error log, NULL on success.</param>
/// <returns>A new vertex shader object.</returns>
bs_VertexShader
bs_vertexShader(
	const char* path
);

/// <summary>
/// Compiles a fragment shader from a file path.
/// </summary>
/// <param name="path">- Path to the fragment shader.</param>
/// <param name="error">- Compilation error log, NULL on success.</param>
/// <returns>A new fragment shader id.</returns>
bs_FragmentShader
bs_fragmentShader(
	const char* path
);

/// <summary>
/// Compiles a geometry shader from a file path.
/// </summary>
/// <param name="path">- Path to the geometry shader.</param>
/// <param name="error">- Compilation error log, NULL on success.</param>
/// <returns>A new geometry shader id.</returns>
bs_GeometryShader
bs_geometryShader(
	const char* path
);

bs_ComputeShader bs_computeShader(const char* path);
void bs_compute(bs_ComputeShader* cs, bs_Texture* output);
void bs_computeSize(bs_ComputeShader* cs, bs_Texture* output, bs_U32 x, bs_U32 y, bs_U32 z);

#endif // BS_SHADERS_H