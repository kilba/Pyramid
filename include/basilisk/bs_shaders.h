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

/// @brief Creates a pipeline from a vertex shader and fragment shader.
/// @param vs Vertex shader pointer
/// @param fs Fragment shader pointer
/// @return 
bs_Pipeline bs_pipeline(bs_VertexShader* vs, bs_FragmentShader* fs);

/// @brief Creates a SPIR-V vertex shader.
/// @param path Path to the compiled .spv
/// @return 
bs_VertexShader
bs_vertexShader(
	const char* path
);

/// @brief Creates a SPIR-V fragment shader.
/// @param path Path to the compiled .spv
/// @return 
bs_FragmentShader
bs_fragmentShader(
	const char* path
);

#endif // BS_SHADERS_H