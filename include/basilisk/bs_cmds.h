// added this just for calling OpenGL rendering functions

#ifndef BS_CMDS_H
#define BS_CMDS_H

#include <bs_types.h>

typedef enum bs_BlendOp bs_BlendOp;
typedef enum bs_StencilOp bs_StencilOp;

typedef enum bs_Face bs_Face;
typedef enum bs_Comparison bs_Comparison;
typedef enum bs_Operation bs_Operation;
typedef enum bs_State bs_State;

void bs_ccw(bool ccw);
void bs_writeIntoDepth(bool enabled);

void bs_writeIntoColor(bool r, bool g, bool b, bool a);
void bs_enable(bs_State state);
void bs_disable(bs_State state);

// blending
void bs_blendFunc(bs_BlendOp sfactor, bs_BlendOp dfactor);
void bs_blendEquation(bs_Operation operation);
void bs_resetBlending();

// stencil testing
void bs_stencilOpSeparate(bs_Face face, bs_StencilOp on_stencil_fail, bs_StencilOp on_depth_fail, bs_StencilOp on_both_pass);
void bs_stencilOp(bs_StencilOp on_stencil_fail, bs_StencilOp on_depth_fail, bs_StencilOp on_both_pass);
void bs_stencilFunc(bs_Comparison function, bs_I32 ref, bs_U32 mask);
void bs_resetStencil();

enum bs_State {
	BS_CULLING_TMP				= 0x0B44,
	BS_DEPTH_CLAMP_TMP			= 0x864F,
};

enum bs_Face {
	BS_FRONT					= 0x0404,
	BS_BACK						= 0x0405,
	BS_FRONT_AND_BACK			= 0x0408
};

enum bs_BlendOp {
	BS_BLEND_ZERO				= 0,
	BS_BLEND_ONE				= 1,
	BS_SRC_COLOR				= 0x0300,
	BS_ONE_MINUS_SRC_COLOR		= 0x0301,
	BS_DST_COLOR				= 0x0306,
	BS_ONE_MINUS_DST_COLOR		= 0x0307,
	BS_SRC_ALPHA				= 0x0302,
	BS_ONE_MINUS_SRC_ALPHA		= 0x0303,
	BS_DST_ALPHA				= 0x0304,
	BS_ONE_MINUS_DST_ALPHA		= 0x0305,
	BS_CONSTANT_COLOR			= 0x8001,
	BS_ONE_MINUS_CONSTANT_COLOR = 0x8002,
	BS_CONSTANT_ALPHA			= 0x8003,
	BS_ONE_MINUS_CONSTANT_ALPHA = 0x8004
};

enum bs_Operation {
	BS_ADD						= 0x8006,
	BS_SUBTRACT					= 0x800A,
	BS_REVERSE_SUBTRACT			= 0x800B,
	BS_MIN						= 0x8007,
	BS_MAX						= 0x8008
};

enum bs_StencilOp {
	BS_KEEP						= 0x1E00,
	BS_REPLACE					= 0x1E01,
	BS_INCREMENT				= 0x1E02,
	BS_DECREMENT				= 0x1E03,
	BS_INCREMENT_WRAP			= 0x8507,
	BS_DECREMENT_WRAP			= 0x8508,
	BS_INVERT					= 0x150A
};

enum bs_Comparison {
	BS_NEVER					= 0x0200,
	BS_ALWAYS					= 0x0207,
	BS_LESSER_THAN				= 0x0201,
	BS_GREATER_THAN				= 0x0204,
	BS_EQUAL_TO					= 0x0202,
	BS_LEQUAL_TO				= 0x0203,
	BS_GEQUAL_TO				= 0x0206,
	BS_NOT_EQUAL_TO				= 0x0205
};

#endif // BS_CMDS_H