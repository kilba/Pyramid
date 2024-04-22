#ifndef BS_MATH_H
#define BS_MATH_H

#include <bs_types.h>

#define BS_FLT_MAX             3.40282347E+38F
#define BS_GJK_EPSILON         1.19209290E-07F
#define BS_GJK_MAX_ITERATIONS  20
#define BS_2PI                (3.142857 * 2.0)
#define BS_PI                  3.142857
#define BS_SIN_45              0.70710678

float bs_min(float a, float b);
float bs_max(float a, float b);
float bs_inverse(float v);
float bs_degrees(float rad);
float bs_radians(float degrees);
float bs_clamp(float v, float min, float max);
float bs_lerp(float a, float b, float f);
float bs_abs(float v);
int bs_sign(float x);
float bs_fsign(float x);
int bs_closestDivisible(int val, int div);
bs_quat bs_eul2quat(bs_vec3 eul);
bs_vec3 bs_quat2eul(bs_quat q);

bs_vec2 bs_v2normalize(bs_vec2 v);
bs_vec3 bs_v3normalize(bs_vec3 v);
bs_vec4 bs_v4normalize(bs_vec4 v);
bs_vec3 bs_cross(bs_vec3 v0, bs_vec3 v1);
float bs_v2dot(bs_vec2 v0, bs_vec2 v1);
float bs_v3dot(bs_vec3 v0, bs_vec3 v1);
float bs_v4dot(bs_vec4 v0, bs_vec4 v1);
bool bs_triangleIsCCW(bs_vec3 a, bs_vec3 b, bs_vec3 c, bs_vec3 normal);
float bs_signv3(bs_vec3 p1, bs_vec3 p2, bs_vec3 p3);
bool bs_ptInTriangle(bs_vec3 pt, bs_vec3 v1, bs_vec3 v2, bs_vec3 v3);
bs_vec3 bs_triangleNormal(bs_vec3 v0, bs_vec3 v1, bs_vec3 v2);
bs_vec3 bs_triangleCenter(bs_vec3 v0, bs_vec3 v1, bs_vec3 v2);
bs_vec3 bs_v3mid(bs_vec3 a, bs_vec3 b);
bs_vec2 bs_v2mid(bs_vec2 a, bs_vec2 b);
bs_aabb bs_v3bounds(bs_vec3 *arr, int num_indices);

bs_vec2 bs_v2rot(bs_vec2 v, bs_vec2 origin, float angle);
bs_vec3 bs_v3rot(bs_vec3 v, bs_vec3 axis, float angle);
bs_vec3 bs_v3rotq(bs_vec3 v, bs_quat q);

bs_vec2 bs_v2lerp(bs_vec2 from, bs_vec2 to, float f);
bs_vec3 bs_v3lerp(bs_vec3 from, bs_vec3 to, float f);
bs_vec3 bs_v3abs(bs_vec3 v);
bs_vec3 bs_v3min(bs_vec3 a, bs_vec3 b);
bs_vec3 bs_v3max(bs_vec3 a, bs_vec3 b);

bs_RGBA bs_rgba(bs_U8 r, bs_U8 g, bs_U8 b, bs_U8 a);
bs_RGBA bs_hex(bs_U32 hex);

// Quaternions
bs_mat3 bs_qToMat3(bs_vec4 q);
bs_quat bs_slerp(bs_quat q1, bs_quat q2, float t);
bs_quat bs_qMulq(bs_quat q, bs_quat rhs);
bs_quat bs_qNormalize(bs_quat q);
float bs_qMagnitude(bs_quat q);
bs_quat bs_qIntegrate(bs_vec4 quat, bs_vec3 dv, float dt);
float bs_rotationBetweenPoints(bs_vec3 a, bs_vec3 b);

// Bezier
float bs_sCubicBez(float p0, float p1, float p2, float p3, float t);
void bs_v2CubicBez(bs_vec2 p0, bs_vec2 p1, bs_vec2 p2, bs_vec2 p3, bs_vec2 *arr, int num_elems);
void bs_v2QuadBez(bs_vec2 p0, bs_vec2 p1, bs_vec2 p2, bs_vec2 *arr, int num_elems);
void bs_cubicBezierPts(bs_vec3 p0, bs_vec3 p1, bs_vec3 p2, bs_vec3 p3, bs_vec3 *arr, int num_elems);

// Matrices
bs_mat3 bs_m3(bs_vec3 a, bs_vec3 b, bs_vec3 c);
bs_mat3 bs_m3Outer(bs_vec3 u, bs_vec3 v);
bs_mat3 bs_m3Diag(bs_vec3 diag);
bs_mat3 bs_m3Transpose(bs_mat3 m);
bs_mat3 bs_m3mul(bs_mat3 a, bs_mat3 b);
bs_vec3 bs_m3mulv3(bs_mat3 m, bs_vec3 v);
bs_vec3 bs_m3mulv3inv(bs_mat3 m, bs_vec3 v);
bs_mat3 bs_m3muls(bs_mat3 m, float scalar);
bs_mat3 bs_m3add(bs_mat3 a, bs_mat3 b);
bs_mat3 bs_m3sub(bs_mat3 a, bs_mat3 b);
bs_mat3 bs_m3Inv(bs_mat3 m);

bs_mat4 bs_m4(bs_vec4 a, bs_vec4 b, bs_vec4 c, bs_vec4 d);
bs_vec4 bs_m4mulv4(bs_mat4 m, bs_vec4 v);
bs_mat4 bs_m4mul(bs_mat4 m1, bs_mat4 m2);
bs_mat4 bs_m4mulrot(bs_mat4 m1, bs_mat4 m2);
bs_mat4 bs_translate(bs_vec3 pos, bs_mat4 mat);
bs_mat4 bs_rotate(bs_quat rot, bs_mat4 mat);
bs_mat4 bs_scale(bs_vec3 sca, bs_mat4 mat);
bs_mat4 bs_transform(bs_vec3 pos, bs_quat rot, bs_vec3 sca);

// Vector Initialization
bs_ivec2 bs_iv2(int x, int y);
bs_ivec3 bs_iv3(int x, int y, int z);
bs_ivec4 bs_iv4(int x, int y, int z, int w);

bs_vec2 bs_v2(float x, float y);
bs_vec3 bs_v3(float x, float y, float z);
bs_vec4 bs_v4(float x, float y, float z, float w);
bs_quat bs_q(float x, float y, float z, float w);

bs_vec2 bs_v2s(float v);
bs_vec3 bs_v3s(float v);
bs_vec4 bs_v4s(float v);

bs_vec3 bs_v001();
bs_vec3 bs_v010();
bs_vec3 bs_v100();

bs_vec2 bs_xz(bs_vec3 v);

bs_vec3 bs_v3fromv2(bs_vec2 v, float z);
bs_vec4 bs_v4fromv3(bs_vec3 v, float w);

bs_vec2 bs_v2fromiv2(bs_ivec2 v);
bs_vec3 bs_v3fromiv3(bs_ivec3 v);
bs_vec4 bs_v4fromiv4(bs_ivec4 v);

bs_ivec2 bs_iv2s(int v);
bs_ivec3 bs_iv3s(int v);
bs_ivec4 bs_iv4s(int v);

bs_ivec3 bs_iv3fromv3(bs_vec3 v);
bs_ivec4 bs_iv4fromv3(bs_vec4 v);

// Vector Addition
bs_vec2 bs_v2add(bs_vec2 a, bs_vec2 b);
bs_vec2 bs_v2adds(bs_vec2 a, float s);
bs_vec3 bs_v3add(bs_vec3 a, bs_vec3 b);
bs_vec3 bs_v3adds(bs_vec3 a, float s);
bs_vec3 bs_v3addv2(bs_vec3 a, bs_vec2 b);
bs_vec4 bs_v4add(bs_vec4 a, bs_vec4 b);
bs_vec4 bs_v4adds(bs_vec4 a, float s);
bs_vec4 bs_v4addv3(bs_vec4 a, bs_vec3 b);

bs_vec3 bs_v3addx(bs_vec3 v, float x);
bs_vec3 bs_v3addy(bs_vec3 v, float y);
bs_vec3 bs_v3addxy(bs_vec3 v, bs_vec2 xy);
bs_vec3 bs_v3addxz(bs_vec3 v, bs_vec2 xz);

// Vector Subtraction
bs_vec2 bs_v2sub(bs_vec2 a, bs_vec2 b);
bs_vec2 bs_v2subs(bs_vec2 a, float s);
bs_vec3 bs_v3sub(bs_vec3 a, bs_vec3 b);
bs_vec3 bs_v3subs(bs_vec3 a, float s);
bs_vec3 bs_v3subv2(bs_vec3 a, bs_vec2 b);
bs_vec4 bs_v4sub(bs_vec4 a, bs_vec4 b);
bs_vec4 bs_v4subs(bs_vec4 a, float s);
bs_vec4 bs_v4subv3(bs_vec4 a, bs_vec3 b);

// Vector Multiplication
bs_vec2 bs_v2mul(bs_vec2 a, bs_vec2 b);
bs_vec2 bs_v2muls(bs_vec2 a, float s);
bs_vec2 bs_v2muladds(bs_vec2 a, float s, bs_vec2 dest);
bs_vec3 bs_v3mul(bs_vec3 a, bs_vec3 b);
bs_vec3 bs_v3muls(bs_vec3 a, float s);
bs_vec3 bs_v3muladds(bs_vec3 a, float s, bs_vec3 dest);
bs_vec4 bs_v4mul(bs_vec4 a, bs_vec4 b);
bs_vec4 bs_v4muls(bs_vec4 a, float s);
bs_vec4 bs_v4muladds(bs_vec4 a, float s, bs_vec4 dest);

// Vector Division
bs_vec2 bs_v2div(bs_vec2 a, bs_vec2 b);
bs_vec2 bs_v2divs(bs_vec2 a, float s);
bs_vec3 bs_v3safediv(bs_vec3 a, bs_vec3 b);
bs_vec3 bs_v3safedivs(bs_vec3 v, float s);
bs_vec3 bs_v3div(bs_vec3 a, bs_vec3 b);
bs_vec3 bs_v3divs(bs_vec3 a, float s);
bs_vec4 bs_v4div(bs_vec4 a, bs_vec4 b);
bs_vec4 bs_v4divs(bs_vec4 a, float s);

// Vector Comparison
bool bs_v2cmp(bs_vec2 a, bs_vec2 b);
bool bs_v3cmp(bs_vec3 a, bs_vec3 b);
bool bs_v4cmp(bs_vec4 a, bs_vec4 b);

bs_vec3 bs_orthoBasis(bs_vec3* a, bs_vec3* b, bs_vec3* c);

float bs_sqrt(float v);
float bs_v2lenSqrd(bs_vec2 v);
float bs_v2len(bs_vec2 v);
float bs_v2dist(bs_vec2 a, bs_vec2 b);
float bs_v3magnitudeSqrd(bs_vec3 v);
float bs_v3magnitude(bs_vec3 v);
float bs_v3dist(bs_vec3 a, bs_vec3 b);

void bs_quadTextureCoords(bs_Quad* q, bs_vec2 offset, bs_vec2 coords);
bs_Quad bs_quad(bs_vec3 position, bs_vec2 dimensions);
bs_Quad bs_quadPositions(bs_vec3 a, bs_vec3 b, bs_vec3 c, bs_vec3 d);
bs_Quad bs_quadNdc();
bs_Quad bs_flipQuad(bs_Quad quad);
bs_Quad bs_alignQuad(bs_Quad quad, bs_vec3 origin, bs_vec3 dir);

bs_Plane bs_plane(bs_vec3 point, bs_vec3 normal);
bool bs_pointInPlane(bs_Plane* plane, bs_vec3 point);
bool bs_planeEdgeIntersects(bs_Plane* plane, bs_vec3* start, bs_vec3* end, bs_vec3* out);

int bs_randRangeI(int min, int max);
float bs_randRange(float min, float max);
bs_vec3 bs_randTrianglePt(bs_vec3 p0, bs_vec3 p1, bs_vec3 p2);

#endif // BS_MATH_H