#include <glad/glad.h>
#include <bs_types.h>
#include <bs_cmds.h>

struct {
	bs_U8 stencil_test;
} enabled;

void bs_ccw(bool ccw) {
	glFrontFace(ccw ? GL_CCW : GL_CW);
}

void bs_writeIntoDepth(bool enabled) {
	glDepthMask(enabled);
}

void bs_writeIntoColor(bool r, bool g, bool b, bool a) {
	glColorMask(r, g, b, a);
}

void bs_enable(bs_State state) {
	glEnable(state);
}

void bs_disable(bs_State state) {
	glDisable(state);
}

// blending
void bs_blendFunc(bs_BlendOp sfactor, bs_BlendOp dfactor) {
	glBlendFunc(sfactor, dfactor);
}

void bs_blendEquation(bs_Operation operation) {
	glBlendEquation(operation);
}

void bs_resetBlending() {
	bs_blendFunc(BS_BLEND_ONE, BS_BLEND_ZERO);
	bs_blendEquation(BS_ADD);
}

// stencil testing
void bs_enableStencilTest() {
	if (!enabled.stencil_test) glEnable(GL_STENCIL_TEST);
}

void bs_stencilOpSeparate(bs_Face face, bs_StencilOp on_stencil_fail, bs_StencilOp on_depth_fail, bs_StencilOp on_both_pass) {
	bs_enableStencilTest();
	glStencilOpSeparate(face, on_stencil_fail, on_depth_fail, on_both_pass);
}

void bs_stencilOp(bs_StencilOp on_stencil_fail, bs_StencilOp on_depth_fail, bs_StencilOp on_both_pass) {
	bs_enableStencilTest();
	glStencilOp(on_stencil_fail, on_depth_fail, on_both_pass);
}

void bs_stencilFunc(bs_Comparison function, bs_I32 ref, bs_U32 mask) {
	bs_enableStencilTest();
	glStencilFunc(function, ref, mask);
}

void bs_resetStencil() {
	bs_stencilFunc(BS_LEQUAL_TO, 0, 0xFF);
	bs_stencilOp(BS_KEEP, BS_KEEP, BS_KEEP);
}