#include <bs_core.h>
#include <bs_math.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

float bs_min(float a, float b) {
    if (a < b) return a;
    return b;
}

float bs_max(float a, float b) {
    if (a > b) return a;
    return b;
}

float bs_inverse(float v) {
    return v != 0.0f ? 1.0f / v : 0.0f;
}

float bs_degrees(float rad) {
    return rad * (180.0f / BS_PI);
}

float bs_radians(float degrees) {
    return degrees * BS_PI / 180.0f;
}

float bs_abs(float v) {
    return fabs(v);
}

float bs_lerp(float a, float b, float f) {
    return a * (1.0f - f) + (b * f);
}

int bs_sign(float x) {
    return (x > 0.0f) - (x < 0.0f);
}

float bs_fsign(float x) {
    return (x > 0.0f) - (x < 0.0f);
}

float bs_clamp(float v, float min, float max) {
    if (v < min)
        return min;

    if (v > max)
        return max;

    return v;
}

int bs_closestDivisible(int val, int div) {
    int q = val / div;
     
    int n1 = div * q;
    int n2 = (val * div) > 0 ? (div * (q + 1)) : (div * (q - 1));
     
    if (abs(val - n1) < abs(val - n2)) {
        return n1;
    }
     
    return n2;
}

float bs_rotationBetweenPoints(bs_vec3 a, bs_vec3 b) {
    return acos(bs_v3dot(a, b) / (bs_v3magnitude(a) * bs_v3magnitude(b)));
}

bs_quat bs_slerp(bs_quat q1, bs_quat q2, float t) {
    float cos_half_theta = bs_v4dot(q1, q2);

    if (fabs(cos_half_theta) >= 1.0f) {
        return q1;
    }

    if(cos_half_theta < 0.0f) {
	    q1 = bs_q(-q1.x, -q1.y, -q1.z, -q1.w);
	    cos_half_theta = -cos_half_theta;
    }

    float half_theta = acos(cos_half_theta);
    float sin_half_theta = bs_sqrt(1.0f - cos_half_theta * cos_half_theta);

    if(fabs(sin_half_theta) < 0.001f) {
	    return bs_q(
	        q1.x * 0.5f + q2.x * 0.5f,
	        q1.y * 0.5f + q2.y * 0.5f,
	        q1.z * 0.5f + q2.z * 0.5f,
	        q1.w * 0.5f + q2.w * 0.5f
	    );
    }

    float ratio1 = sin((1.0f - t) * half_theta) / sin_half_theta;
    float ratio2 = sin(t * half_theta) / sin_half_theta;

    return bs_q(
	    q1.x * ratio1 + q2.x * ratio2,
	    q1.y * ratio1 + q2.y * ratio2,
	    q1.z * ratio1 + q2.z * ratio2,
	    q1.w * ratio1 + q2.w * ratio2
    );
}

bs_quat bs_eul2quat(bs_vec3 eul) {
    double cy = cos(eul.x * 0.5f);
    double sy = sin(eul.x * 0.5f);
    double cp = cos(eul.y * 0.5f);
    double sp = sin(eul.y * 0.5f);
    double cr = cos(eul.z * 0.5f);
    double sr = sin(eul.z * 0.5f);

    return bs_q(
        sr * cp * cy - cr * sp * sy,
        cr * sp * cy + sr * cp * sy,
        cr * cp * sy - sr * sp * cy,
        cr * cp * cy + sr * sp * sy
    );
}

bs_vec3 bs_quat2eul(bs_quat q) {
    bs_vec3 eul;

    float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    eul.x = atan2(sinr_cosp, cosr_cosp);

    float sinp = 2.0f * (q.w * q.y - q.z * q.x);
    if (fabs(sinp) >= 1.0f) eul.y = bs_sign(BS_PI / 2.0f, sinp);
    else eul.y = asin(sinp);

    float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    eul.z = atan2(siny_cosp, cosy_cosp);

    return eul;
}


bs_aabb bs_v3bounds(bs_vec3* arr, int num_indices) {
    bs_aabb aabb = { bs_v3s(0.0f), bs_v3s(0.0f) };
    float ffar = -99999.0f;
    float fnear = 99999.0f;

    for(int i = 0; i < num_indices; i++) {
	    float dot = bs_v3dot(bs_v3s(1.0f), arr[i]);
	    if(dot > ffar) {
	        ffar = dot;
	        aabb.min = arr[i];
	    }

	    if(dot < fnear) {
	        fnear = dot;
	        aabb.max = arr[i];
	    }
    }

    return aabb;
}

bs_vec3 bs_v3abs(bs_vec3 v) {
    return bs_v3(bs_abs(v.x), bs_abs(v.y), bs_abs(v.z));
}

bs_vec3 bs_v3min(bs_vec3 a, bs_vec3 b) {
    return bs_v3(bs_min(a.x, b.x), bs_min(a.y, b.y), bs_min(a.z, b.z));
}

bs_vec3 bs_v3max(bs_vec3 a, bs_vec3 b) {
    return bs_v3(bs_max(a.x, b.x), bs_max(a.y, b.y), bs_max(a.z, b.z));
}

bs_vec2 bs_v2normalize(bs_vec2 v) {
    float len = bs_sqrt(bs_v2dot(v, v));
    return len == 0.0f ? bs_v2s(0.0f) : bs_v2divs(v, len);
}

bs_vec3 bs_v3normalize(bs_vec3 v) {
    float len = bs_sqrt(bs_v3dot(v, v));
    return len == 0.0f ? v : bs_v3muls(v, 1.0f / len);
}

bs_vec4 bs_v4normalize(bs_vec4 v) {
    float len = bs_sqrt(bs_v4dot(v, v));
    return len == 0.0f ? bs_v4s(0.0f) : bs_v4divs(v, len);
}

bs_vec3 bs_cross(bs_vec3 v0, bs_vec3 v1) {
    bs_vec3 out;
    out.x = v0.y * v1.z - v0.z * v1.y;
    out.y = v0.z * v1.x - v0.x * v1.z;
    out.z = v0.x * v1.y - v0.y * v1.x;
    return out;
}

float bs_v2dot(bs_vec2 v0, bs_vec2 v1) {
    return v0.x * v1.x + v0.y * v1.y;
}

float bs_v3dot(bs_vec3 v0, bs_vec3 v1) {
    return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}

float bs_v4dot(bs_vec4 v0, bs_vec4 v1) {
    return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z + v0.w * v1.w;
}

bs_vec3 bs_triangleNormal(bs_vec3 v0, bs_vec3 v1, bs_vec3 v2) {
    bs_vec3 a = bs_v3(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z);
    bs_vec3 b = bs_v3(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);
    bs_vec3 c = bs_cross(a, b);
    c = bs_v3normalize(c);

    return c;
}

bs_vec3 bs_triangleCenter(bs_vec3 v0, bs_vec3 v1, bs_vec3 v2) {
    float x, y, z;
    x = v0.x + v1.x + v2.x;
    y = v0.y + v1.y + v2.y;
    z = v0.z + v1.z + v2.z;

    return bs_v3(x / 3.0f, y / 3.0f, z / 3.0f);
}

bool bs_triangleIsCCW(bs_vec3 a, bs_vec3 b, bs_vec3 c, bs_vec3 normal) {
    return bs_cross(
	    bs_v3(b.x - a.x, b.y - a.y, b.z - a.z), 
	    bs_v3(c.x - a.x, c.y - a.y, c.z - a.z)
    ).z > 0.0f;
}

float bs_signv3(bs_vec3 p1, bs_vec3 p2, bs_vec3 p3) {
    return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

bs_vec3 bs_v3mid(bs_vec3 a, bs_vec3 b) {
    return bs_v3((a.x + b.x) / 2.0f, (a.y + b.y) / 2.0f, (a.z + b.z) / 2.0f);
}

bs_vec2 bs_v2mid(bs_vec2 a, bs_vec2 b) {
    return bs_v2((a.x + b.x) / 2.0f, (a.y + b.y) / 2.0f);
}

bool bs_ptInTriangle(bs_vec3 pt, bs_vec3 v1, bs_vec3 v2, bs_vec3 v3) {
    float d1, d2, d3;
    bool has_neg, has_pos;

    d1 = bs_signv3(pt, v1, v2);
    d2 = bs_signv3(pt, v2, v3);
    d3 = bs_signv3(pt, v3, v1);

    has_neg = (d1 < 0.0f) || (d2 < 0.0f) || (d3 < 0.0f);
    has_pos = (d1 > 0.0f) || (d2 > 0.0f) || (d3 > 0.0f);

    return !(has_neg && has_pos);
}

// Quaternions
bs_mat3 bs_qToMat3(bs_vec4 q) {
    float qx2 = q.x + q.x;
    float qy2 = q.y + q.y;
    float qz2 = q.z + q.z;
    float qxqx2 = q.x * qx2;
    float qxqy2 = q.x * qy2;
    float qxqz2 = q.x * qz2;
    float qxqw2 = q.w * qx2;
    float qyqy2 = q.y * qy2;
    float qyqz2 = q.y * qz2;
    float qyqw2 = q.w * qy2;
    float qzqz2 = q.z * qz2;
    float qzqw2 = q.w * qz2;

    return bs_m3(
        bs_v3(1.0f - qyqy2 - qzqz2, qxqy2 + qzqw2, qxqz2 - qyqw2),
        bs_v3(qxqy2 - qzqw2, 1.0f - qxqx2 - qzqz2, qyqz2 + qxqw2),
        bs_v3(qxqz2 + qyqw2, qyqz2 - qxqw2, 1.0f - qxqx2 - qyqy2)
    );
}

bs_quat bs_qMulq(bs_quat q, bs_quat rhs) {
    return bs_q(
	    q.w * rhs.x + q.x * rhs.w + q.y * rhs.z - q.z * rhs.y,
	    q.w * rhs.y + q.y * rhs.w + q.z * rhs.x - q.x * rhs.z,
	    q.w * rhs.z + q.z * rhs.w + q.x * rhs.y - q.y * rhs.x,
	    q.w * rhs.w - q.x * rhs.x - q.y * rhs.y - q.z * rhs.z
	);
}

bs_vec3 bs_applyRotationToV3(bs_quat q, bs_vec3 v) {
    return bs_qMulq(bs_qMulq(q, bs_q(v.x, v.y, v.z, 0.0f)), bs_q(-q.x, -q.y, -q.z, q.w)).xyz;
}

bs_quat bs_qNormalize(bs_quat q) {
    float x = q.x;
    float y = q.y;
    float z = q.z;
    float w = q.w;

    float d = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
    if(d == 0.0f) w = 1.0f;

    d = 1.0f / bs_sqrt(d);
    if(d > ((float)1.0e-8)) {
	    x *= d;
	    y *= d;
	    z *= d;
	    w *= d;
    }

    return bs_q(x, y, z, w);
}

float bs_qMagnitude(bs_quat q) {
    return sqrtf(bs_v4dot(q, q));
}

/*
    q3Quaternion q( dv.x * dt, dv.y * dt, dv.z * dt, r32( 0.0 ) );

    q *= *this;

    x += q.x * r32( 0.5 );
    y += q.y * r32( 0.5 );
    z += q.z * r32( 0.5 );
    w += q.w * r32( 0.5 );

    *this = q3Normalize( *this );

*/

bs_quat bs_qIntegrate(bs_vec4 quat, bs_vec3 dv, float dt) {
    bs_quat q = bs_q(dv.x * dt, dv.y * dt, dv.z * dt, 0.0f);
    q = bs_qMulq(q, quat);

    return bs_qNormalize(bs_v4add(quat, bs_v4muls(q, 0.5f)));
}

// Bezier
float bs_sCubicBez(float p0, float p1, float p2, float p3, float t) {
    double curve;

    curve  =     powf(1.0f - t, 3.0f) * p0;
    curve += 3 * powf(1.0f - t, 2.0f) * t * p1;
    curve += 3 * powf(1.0f - t, 1.0f) * powf(t, 2.0f) * p2;
    curve +=     powf(t, 3.0f) * p3;

    return curve;
}

void bs_v2CubicBez(bs_vec2 p0, bs_vec2 p1, bs_vec2 p2, bs_vec2 p3, bs_vec2 *arr, int num_elems) {
    double t = 0.0f;
    double incr;

    incr = 1.0f / (double)num_elems;

    for(int i = 0; i < num_elems; i++, t += incr) {
	    float x = bs_sCubicBez(p0.x, p1.x, p2.x, p3.x, t);
	    float y = bs_sCubicBez(p0.y, p1.y, p2.y, p3.y, t);

	    arr[i] = bs_v2(x, y);
    }
}

// todo make float
void bs_v2QuadBez(bs_vec2 p0, bs_vec2 p1, bs_vec2 p2, bs_vec2 *arr, int num_elems) {
    double t = 0.0;
    double incr;

    incr = 1.0 / (double)num_elems;

    for(int i = 0; i < num_elems; i++, t += incr) {
	    bs_vec2 v;

        v.x = (1 - t) * (1 - t) * p0.x + 2 * (1 - t) * t * p1.x + t * t * p2.x;
	    v.y = (1 - t) * (1 - t) * p0.y + 2 * (1 - t) * t * p1.y + t * t * p2.y;

	    arr[i] = v;
    }
}

void bs_cubicBezierPts(bs_vec3 p0, bs_vec3 p1, bs_vec3 p2, bs_vec3 p3, bs_vec3 *arr, int num_elems) {
    double t = 0.0;
    double incr;
    int i = 0;

    incr = 1.0 / (double)num_elems;

    for(; i < num_elems; i++, t += incr) {
	    float x = bs_sCubicBez(p0.x, p1.x, p2.x, p3.x, t);
	    float y = bs_sCubicBez(p0.y, p1.y, p2.y, p3.y, t);
	    float z = bs_sCubicBez(p0.z, p1.z, p2.z, p3.z, t);

	    arr[i] = bs_v3(x, y, z);
    }
}

// matrices
bs_mat3 bs_m3(bs_vec3 a, bs_vec3 b, bs_vec3 c) {
    bs_mat3 m;
    m.v[0] = a;
    m.v[1] = b;
    m.v[2] = c;
    return m;
}

bs_mat3 bs_m3Outer(bs_vec3 u, bs_vec3 v) {
    return bs_m3(
        bs_v3muls(v, u.x),
        bs_v3muls(v, u.y),
        bs_v3muls(v, u.z)
    );
}

bs_mat3 bs_m3Diag(bs_vec3 diag) {
    return bs_m3(
        bs_v3(diag.x, 0.0f, 0.0f),
        bs_v3(0.0f, diag.y, 0.0f),
        bs_v3(0.0f, 0.0f, diag.z)
    );
}

bs_mat3 bs_m3Transpose(bs_mat3 m) {
    bs_mat3 dest;

    dest.v[0].a[0] = m.v[0].a[0];
    dest.v[0].a[1] = m.v[1].a[0];
    dest.v[0].a[2] = m.v[2].a[0];

    dest.v[1].a[0] = m.v[0].a[1];
    dest.v[1].a[1] = m.v[1].a[1];
    dest.v[1].a[2] = m.v[2].a[1];

    dest.v[2].a[0] = m.v[0].a[2];
    dest.v[2].a[1] = m.v[1].a[2];
    dest.v[2].a[2] = m.v[2].a[2];

    return dest;
}

bs_mat3 bs_m3Inv(bs_mat3 m) {
    bs_vec3 tmp0 = bs_cross(m.v[1], m.v[2]);
    bs_vec3 tmp1 = bs_cross(m.v[2], m.v[0]);
    bs_vec3 tmp2 = bs_cross(m.v[0], m.v[1]);

    float detinv = 1.0f / bs_v3dot(m.v[2], tmp2);
    return bs_m3(
        bs_v3(tmp0.x * detinv, tmp0.y * detinv, tmp0.z * detinv),
        bs_v3(tmp1.x * detinv, tmp1.y * detinv, tmp1.z * detinv),
        bs_v3(tmp2.x * detinv, tmp2.y * detinv, tmp2.z * detinv)
    );
}

bs_vec3 bs_m3mulv3(bs_mat3 m, bs_vec3 v) {
    return bs_v3(
        m.a[0][0] * v.a[0] + m.a[1][0] * v.a[1] + m.a[2][0] * v.a[2],
        m.a[0][1] * v.a[0] + m.a[1][1] * v.a[1] + m.a[2][1] * v.a[2],
        m.a[0][2] * v.a[0] + m.a[1][2] * v.a[1] + m.a[2][2] * v.a[2]
    );
}

bs_vec3 bs_m3mulv3inv(bs_mat3 m, bs_vec3 v) {
    return bs_v3(
        m.a[0][0] * v.a[0] + m.a[0][1] * v.a[1] + m.a[0][2] * v.a[2],
        m.a[1][0] * v.a[0] + m.a[1][1] * v.a[1] + m.a[1][2] * v.a[2],
        m.a[2][0] * v.a[0] + m.a[2][1] * v.a[1] + m.a[2][2] * v.a[2]
    );
}

bs_mat3 bs_m3mul(bs_mat3 a, bs_mat3 b) {
    return bs_m3(
        bs_m3mulv3(a, b.v[0]),
        bs_m3mulv3(a, b.v[1]),
        bs_m3mulv3(a, b.v[2])
    );
}

bs_mat3 bs_m3muls(bs_mat3 m, float scalar) {
    return bs_m3(
        bs_v3muls(m.v[0], scalar),
        bs_v3muls(m.v[1], scalar),
        bs_v3muls(m.v[2], scalar)
    );
}

bs_mat3 bs_m3add(bs_mat3 a, bs_mat3 b) {
    return bs_m3(
        bs_v3add(a.v[0], b.v[0]),
        bs_v3add(a.v[1], b.v[1]),
        bs_v3add(a.v[2], b.v[2])
    );
}

bs_mat3 bs_m3sub(bs_mat3 a, bs_mat3 b) {
    return bs_m3(
        bs_v3sub(a.v[0], b.v[0]),
        bs_v3sub(a.v[1], b.v[1]),
        bs_v3sub(a.v[2], b.v[2])
    );
}

bs_mat3 bs_rotMatrix(bs_vec3 axis, float angle) {
    bs_vec3 normalized_axis = bs_v3normalize(axis);
    bs_mat3 K = bs_m3(
        bs_v3(0, -normalized_axis.z, normalized_axis.y),
        bs_v3(normalized_axis.z, 0.0f, -normalized_axis.x),
        bs_v3(-normalized_axis.y, normalized_axis.x, 0.0f)
    );

    bs_mat3 K2 = bs_m3mul(K, K);
    bs_mat3 I = BS_MAT3_IDENTITY;
    bs_mat3 R = bs_m3add(bs_m3add(I, bs_m3muls(K, sin(angle))), bs_m3muls(K2, (1.0f - cosf(angle))));

    return R;
}

bs_mat4 bs_m4(bs_vec4 a, bs_vec4 b, bs_vec4 c, bs_vec4 d) {
    bs_mat4 m;
    m.v[0] = a;
    m.v[1] = b;
    m.v[2] = c;
    m.v[3] = d;
    return m;
}

bs_vec4 bs_m4mulv4(bs_mat4 m, bs_vec4 v) {
    bs_vec4 res;

    res.a[0] = m.a[0][0] * v.a[0] + m.a[1][0] * v.a[1] + m.a[2][0] * v.a[2] + m.a[3][0] * v.a[3];
    res.a[1] = m.a[0][1] * v.a[0] + m.a[1][1] * v.a[1] + m.a[2][1] * v.a[2] + m.a[3][1] * v.a[3];
    res.a[2] = m.a[0][2] * v.a[0] + m.a[1][2] * v.a[1] + m.a[2][2] * v.a[2] + m.a[3][2] * v.a[3];
    res.a[3] = m.a[0][3] * v.a[0] + m.a[1][3] * v.a[1] + m.a[2][3] * v.a[2] + m.a[3][3] * v.a[3];

    return res;
}

bs_mat4 bs_m4mul(bs_mat4 m1, bs_mat4 m2) {
    bs_mat4 dest;

    dest.v[0] = bs_m4mulv4(m1, m2.v[0]);
    dest.v[1] = bs_m4mulv4(m1, m2.v[1]);
    dest.v[2] = bs_m4mulv4(m1, m2.v[2]);
    dest.v[3] = bs_m4mulv4(m1, m2.v[3]);

    return dest;
}

bs_mat4 bs_m4mulrot(bs_mat4 m1, bs_mat4 m2) {
    bs_mat4 dest;

    dest.v[0] = bs_m4mulv4(m1, m2.v[0]);
    dest.v[1] = bs_m4mulv4(m1, m2.v[1]);
    dest.v[2] = bs_m4mulv4(m1, m2.v[2]);
    dest.v[3] = bs_v4(m1.a[3][0], m1.a[3][1], m1.a[3][2], m1.a[3][3]);

    return dest;
}

bs_mat4 bs_translate(bs_vec3 pos, bs_mat4 mat) {
    mat.v[3] = bs_v4muladds(mat.v[0], pos.x, mat.v[3]); 
    mat.v[3] = bs_v4muladds(mat.v[1], pos.y, mat.v[3]); 
    mat.v[3] = bs_v4muladds(mat.v[2], pos.z, mat.v[3]); 
    return mat;
}

bs_mat4 bs_rotate(bs_quat rot, bs_mat4 mat) {
    float norm = bs_qMagnitude(rot);
    float s = norm > 0.0f ? 2.0f / norm : 0.0f;

    float xx, yy, zz;
    float xy, yz, xz;
    float wx, wy, wz;

    xx = s * rot.x * rot.x;   xy = s * rot.x * rot.y;   wx = s * rot.w * rot.x;
    yy = s * rot.y * rot.y;   yz = s * rot.y * rot.z;   wy = s * rot.w * rot.y;
    zz = s * rot.z * rot.z;   xz = s * rot.x * rot.z;   wz = s * rot.w * rot.z;

    bs_mat4 rotm4 = {
	    bs_v4(1.0f - yy - zz, xy + wz, xz - wy, 0.0f ),
	    bs_v4(xy - wz , 1.0f - xx - zz, yz + wx, 0.0f ),
	    bs_v4(xz + wy , yz - wx, 1.0f - xx - yy, 0.0f ),
        bs_v4(0.0f, 0.0f, 0.0f, 1.0f)
    };

    return bs_m4mulrot(mat, rotm4);
}

bs_mat4 bs_scale(bs_vec3 sca, bs_mat4 mat) {
    mat.v[0] = bs_v4muls(mat.v[0], sca.x);
    mat.v[1] = bs_v4muls(mat.v[1], sca.y);
    mat.v[2] = bs_v4muls(mat.v[2], sca.z);
    return mat;
}

bs_mat4 bs_transform(bs_vec3 pos, bs_quat rot, bs_vec3 sca) {
    bs_mat4 m = BS_MAT4_IDENTITY;
    m = bs_translate(pos, m);
    m = bs_rotate(rot, m);
    m = bs_scale(sca, m);

    return m;
}

bs_RGBA bs_rgba(bs_U8 r, bs_U8 g, bs_U8 b, bs_U8 a) {
    return (bs_RGBA) { r, g, b, a };
}

bs_RGBA bs_hex(bs_U32 hex) {
    bs_RGBA rgba;
    rgba.hex = hex;
    return rgba;
}

// VECTOR INITIALIZATION
bs_ivec2 bs_iv2(int x, int y) {
    return (bs_ivec2) { x, y };
}

bs_ivec3 bs_iv3(int x, int y, int z) {
    return (bs_ivec3) { x, y, z };
}

bs_ivec4 bs_iv4(int x, int y, int z, int w) {
    return (bs_ivec4) { x, y, z, w };
}

bs_vec2 bs_v2(float x, float y) {
    return (bs_vec2) { x, y };
}

bs_vec3 bs_v3(float x, float y, float z) {
    return (bs_vec3) { x, y, z };
}

bs_vec4 bs_v4(float x, float y, float z, float w) {
    return (bs_vec4) { x, y, z, w };
}

bs_quat bs_q(float x, float y, float z, float w) {
    return (bs_quat) { x, y, z, w };
}

bs_vec2 bs_v2s(float v) {
    return (bs_vec2) { v, v };
}

bs_vec3 bs_v3s(float v) {
    return (bs_vec3) { v, v, v };
}

bs_vec3 bs_v001() {
    return (bs_vec3) { 0.0, 0.0, 1.0 };
}

bs_vec3 bs_v010() {
    return (bs_vec3) { 0.0, 1.0, 0.0 };
}

bs_vec3 bs_v100() {
    return (bs_vec3) { 1.0, 0.0, 0.0 };
}

bs_vec4 bs_v4s(float v) {
    return (bs_vec4) { v, v, v, v };
}

bs_vec2 bs_xz(bs_vec3 v) {
    return bs_v2(v.x, v.z);
}

bs_vec3 bs_v3fromv2(bs_vec2 v, float z) {
    return bs_v3(v.x, v.y, z);
}

bs_vec4 bs_v4fromv3(bs_vec3 v, float w) {
    return bs_v4(v.x, v.y, v.z, w);
}

// vx from ivx
bs_vec2 bs_v2fromiv2(bs_ivec2 v) {
    return bs_v2(v.x, v.y);
}

bs_vec3 bs_v3fromiv3(bs_ivec3 v) {
    return bs_v3(v.x, v.y, v.z);
}

bs_vec4 bs_v4fromiv4(bs_ivec4 v) {
    return bs_v4(v.x, v.y, v.z, v.w);
}

bs_ivec2 bs_iv2s(int v) {
    return (bs_ivec2) { v, v };
}

bs_ivec3 bs_iv3s(int v) {
    return (bs_ivec3) { v, v, v };
}

bs_ivec4 bs_iv4s(int v) {
    return (bs_ivec4) { v, v, v, v };
}

bs_ivec3 bs_iv3fromv3(bs_vec3 v) {
    return bs_iv3(v.x, v.y, v.z);
}

bs_ivec4 bs_iv4fromv3(bs_vec4 v) {
    return bs_iv4(v.x, v.y, v.z, v.w);
}

// Vector Addition
// Vec2
bs_vec2 bs_v2add(bs_vec2 a, bs_vec2 b) {
    return bs_v2(a.x + b.x, a.y + b.y);
}

bs_vec2 bs_v2adds(bs_vec2 a, float s) {
    return bs_v2(a.x + s, a.y + s);
}

// Vec3
bs_vec3 bs_v3add(bs_vec3 a, bs_vec3 b) {
    return bs_v3(a.x + b.x, a.y + b.y, a.z + b.z);
}

bs_vec3 bs_v3adds(bs_vec3 a, float s) {
    return bs_v3(a.x + s, a.y + s, a.z + s);
}

bs_vec3 bs_v3addv2(bs_vec3 a, bs_vec2 b) {
    return bs_v3(a.x + b.x, a.y + b.y, a.z);
}

bs_vec3 bs_v3addx(bs_vec3 v, float x) {
    return bs_v3(v.x + x, v.y, v.z);
}

bs_vec3 bs_v3addy(bs_vec3 v, float y) {
    return bs_v3(v.x, v.y + y, v.z);
}

bs_vec3 bs_v3addxy(bs_vec3 v, bs_vec2 xy) {
    return bs_v3(v.x + xy.x, v.y + xy.y, v.z);
}

bs_vec3 bs_v3addxz(bs_vec3 v, bs_vec2 xz) {
    return bs_v3(v.x + xz.x, v.y, v.z + xz.y);
}

// Vec4
bs_vec4 bs_v4add(bs_vec4 a, bs_vec4 b) {
    return bs_v4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

bs_vec4 bs_v4adds(bs_vec4 a, float s) {
    return bs_v4(a.x + s, a.y + s, a.z + s, a.w + s);
}

bs_vec4 bs_v4addv3(bs_vec4 a, bs_vec3 b) {
    return bs_v4(a.x + b.x, a.y + b.y, a.z + b.z, a.w);
}

// Vector Subtraction
// Vec2
bs_vec2 bs_v2sub(bs_vec2 a, bs_vec2 b) {
    return bs_v2(a.x - b.x, a.y - b.y);
}

bs_vec2 bs_v2subs(bs_vec2 a, float s) {
    return bs_v2(a.x - s, a.y - s);
}

// Vec3
bs_vec3 bs_v3sub(bs_vec3 a, bs_vec3 b) {
    return bs_v3(a.x - b.x, a.y - b.y, a.z - b.z);
}

bs_vec3 bs_v3subs(bs_vec3 a, float s) {
    return bs_v3(a.x - s, a.y - s, a.z - s);
}

bs_vec3 bs_v3subv2(bs_vec3 a, bs_vec2 b) {
    return bs_v3(a.x - b.x, a.y - b.y, a.z);
}

// Vec4
bs_vec4 bs_v4sub(bs_vec4 a, bs_vec4 b) {
    return bs_v4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

bs_vec4 bs_v4subs(bs_vec4 a, float s) {
    return bs_v4(a.x - s, a.y - s, a.z - s, a.w - s);
}

bs_vec4 bs_v4subv3(bs_vec4 a, bs_vec3 b) {
    return bs_v4(a.x - b.x, a.y - b.y, a.z - b.z, a.w);
}

// Vector Multiplication
// Vec2
bs_vec2 bs_v2mul(bs_vec2 a, bs_vec2 b) {
    return bs_v2(a.x * b.x, a.y * b.y);
}

bs_vec2 bs_v2muls(bs_vec2 a, float s) {
    return bs_v2(a.x * s, a.y * s);
}

bs_vec2 bs_v2muladds(bs_vec2 a, float s, bs_vec2 dest) {
    return bs_v2add(dest, bs_v2muls(a, s));
}

// Vec3
bs_vec3 bs_v3mul(bs_vec3 a, bs_vec3 b) {
    return bs_v3(a.x * b.x, a.y * b.y, a.z * b.z);
}

bs_vec3 bs_v3muls(bs_vec3 a, float s) {
    return bs_v3(a.x * s, a.y * s, a.z * s);
}

bs_vec3 bs_v3muladds(bs_vec3 a, float s, bs_vec3 dest) {
    return bs_v3add(dest, bs_v3muls(a, s));
}

// Vec4
bs_vec4 bs_v4mul(bs_vec4 a, bs_vec4 b) {
    return bs_v4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

bs_vec4 bs_v4muls(bs_vec4 a, float s) {
    return bs_v4(a.x * s, a.y * s, a.z * s, a.w * s);
}

bs_vec4 bs_v4muladds(bs_vec4 a, float s, bs_vec4 dest) {
    return bs_v4add(dest, bs_v4muls(a, s));
}

// Vector Division
// Vec2
bs_vec2 bs_v2div(bs_vec2 a, bs_vec2 b) {
    return bs_v2(a.x / b.x, a.y / b.y);
}

bs_vec2 bs_v2divs(bs_vec2 a, float s) {
    return bs_v2(a.x / s, a.y / s);
}

// Vec3
bs_vec3 bs_v3safediv(bs_vec3 a, bs_vec3 b) {
    return bs_v3(
        (b.x == 0.0) ? 0.0 : a.x / b.x,
        (b.y == 0.0) ? 0.0 : a.y / b.y,
        (b.z == 0.0) ? 0.0 : a.z / b.z
    );
}

bs_vec3 bs_v3safedivs(bs_vec3 v, float s) {
    if (s == 0.0) return bs_v3s(0.0);
    return bs_v3(v.x / s, v.y / s, v.z / s);
}

bs_vec3 bs_v3div(bs_vec3 a, bs_vec3 b) {
    return bs_v3(a.x / b.x, a.y / b.y, a.z / b.z);
}

bs_vec3 bs_v3divs(bs_vec3 a, float s) {
    return bs_v3(a.x / s, a.y / s, a.z / s);
}

// Vec4
bs_vec4 bs_v4div(bs_vec4 a, bs_vec4 b) {
    return bs_v4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
}

bs_vec4 bs_v4divs(bs_vec4 a, float s) {
    return bs_v4(a.x / s, a.y / s, a.z / s, a.w / s);
}

// Vector Lerp
bs_vec2 bs_v2lerp(bs_vec2 from, bs_vec2 to, float f) {
    return bs_v2(bs_lerp(from.x, to.x, f), bs_lerp(from.y, to.y, f));
}

bs_vec3 bs_v3lerp(bs_vec3 from, bs_vec3 to, float f) {
    return bs_v3(bs_lerp(from.x, to.x, f), bs_lerp(from.y, to.y, f), bs_lerp(from.z, to.z, f));
}

// Vector Comparison
bool bs_v2cmp(bs_vec2 a, bs_vec2 b) {
    return (a.x == b.x) && (a.y == b.y);
}

bool bs_v3cmp(bs_vec3 a, bs_vec3 b) {
    return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}

bool bs_v4cmp(bs_vec4 a, bs_vec4 b) {
    return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
}

bs_vec3 bs_orthoBasis(bs_vec3* a, bs_vec3* b, bs_vec3* c) {
    if (bs_abs(a->x) >= 0.57735027f) {
        *b = bs_v3(a->y, -a->x, 0.0f);
    }
    else {
        *b = bs_v3(0.0f, a->z, -a->y);
    }

    *b = bs_v3normalize(*b);
    *c = bs_cross(*a, *b);
}

// Vector Distances
// Vec2
float bs_sqrt(float v) {
    return sqrtf(v);
}

float bs_v2lenSqrd(bs_vec2 v) {
    return v.x * v.x + v.y * v.y;
}

float bs_v2len(bs_vec2 v) {
    return bs_sqrt(bs_v2lenSqrd(v));
}

float bs_v2dist(bs_vec2 a, bs_vec2 b) {
    return bs_sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}

// Vec3
float bs_v3magnitudeSqrd(bs_vec3 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

float bs_v3magnitude(bs_vec3 v) {
    return bs_sqrt(bs_v3magnitudeSqrd(v));
}

float bs_v3dist(bs_vec3 a, bs_vec3 b) {
    return bs_sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y) + (b.z - a.z) * (b.z - a.z));
}

// Vector Rotation
bs_vec2 bs_v2rot(bs_vec2 v, bs_vec2 origin, float angle) {
    return bs_v2(
        cos(angle) * (v.x - origin.x) - sin(angle) * (v.y - origin.y) + origin.x,
        sin(angle) * (v.x - origin.x) + cos(angle) * (v.y - origin.y) + origin.y
    );
}

bs_vec3 bs_v3rot(bs_vec3 v, bs_vec3 axis, float angle) {
    axis = bs_v3divs(axis, bs_v3magnitude(axis));

    return bs_v3add(bs_v3add(
            bs_v3muls(v, cosf(angle)),
            bs_v3muls(bs_cross(axis, v), sinf(angle))
        ),
        bs_v3muls(axis, bs_v3dot(axis, v) * (1 - cosf(angle)))
    );
}

bs_vec3 bs_v3rotq(bs_vec3 v, bs_quat q) {
    bs_vec3 u = bs_v3(q.x, q.y, q.z);
    float s = q.w;

    return
        bs_v3add(
	    bs_v3add(
	        bs_v3muls(u, 2.0f * bs_v3dot(u, v)),
	        bs_v3muls(v, s * s - bs_v3dot(u, u))
	    ),
	    bs_v3muls(bs_cross(u, v), 2.0f * s)
    );
}

// Quads
bs_Quad bs_quadPositions(bs_vec3 a, bs_vec3 b, bs_vec3 c, bs_vec3 d) {
    bs_Quad q;

    q.a = a;
    q.b = b;
    q.c = c;
    q.d = d;

    return q;
}

void bs_quadTextureCoords(bs_Quad* q, bs_vec2 offset, bs_vec2 coords) {
    q->ca = bs_v2(offset.x, offset.y);
    q->cb = bs_v2(coords.x, offset.y);
    q->cc = bs_v2(offset.x, coords.y);
    q->cd = bs_v2(coords.x, coords.y);
}

bs_Quad bs_quad(bs_vec3 position, bs_vec2 dimensions) {
    bs_Quad q = bs_quadPositions(
        position,
        bs_v3addx(position, dimensions.x),
        bs_v3addy(position, dimensions.y),
        bs_v3addxy(position, dimensions)
    );
    bs_quadTextureCoords(&q, bs_v2s(0.0), bs_v2s(1.0));
    return q;
}

bs_Quad bs_quadNdc() {
    bs_Quad q = bs_quadPositions(
        bs_v3(-1.0, -1.0, 0.0),
        bs_v3( 1.0, -1.0, 0.0),
        bs_v3(-1.0,  1.0, 0.0),
        bs_v3( 1.0,  1.0, 0.0)
    );
    bs_quadTextureCoords(&q, bs_v2s(0.0), bs_v2s(1.0));
    return q;
}

// todo add offsets
bs_Quad bs_flipQuad(bs_Quad quad) {
    quad.ca = bs_v2(0.0, 1.0);
    quad.cb = bs_v2(1.0, 1.0);
    quad.cc = bs_v2(0.0, 0.0);
    quad.cd = bs_v2(1.0, 0.0);
    return quad;
}

bs_Quad bs_alignQuad(bs_Quad quad, bs_vec3 origin, bs_vec3 dir) {
    dir = bs_v3normalize(dir);
    bs_vec3 axis = bs_v3normalize(bs_cross(bs_v010(), dir));

    float angle = acos(bs_v3dot(bs_v010(), dir));
    bs_mat3 rot = bs_rotMatrix(axis, angle);


    bs_vec3 t = bs_v3sub(quad.a, origin);
    bs_vec3 t2 = bs_m3mulv3(rot, bs_v3sub(quad.a, origin));

    quad.a = bs_v3add(origin, bs_m3mulv3(rot, bs_v3sub(quad.a, origin)));


    quad.b = bs_v3add(origin, bs_m3mulv3(rot, bs_v3sub(quad.b, origin)));
    quad.c = bs_v3add(origin, bs_m3mulv3(rot, bs_v3sub(quad.c, origin)));
    quad.d = bs_v3add(origin, bs_m3mulv3(rot, bs_v3sub(quad.d, origin)));

    return quad;
}

// plane
bs_Plane bs_plane(bs_vec3 point, bs_vec3 normal) {
    bs_Plane plane;
    plane.point = point;
    plane.normal = normal;
    return plane;
}

bool bs_pointInPlane(bs_Plane* plane, bs_vec3 point) {
    float distance = -bs_v3dot(plane->normal, plane->point);
    if ((bs_v3dot(point, plane->normal) + distance) < 0.0) {
        return false;
    }

    return true;
}

bool bs_planeEdgeIntersects(bs_Plane* plane, bs_vec3* start, bs_vec3* end, bs_vec3* out) {
    bs_F64 EPSILON = 0.000001;
    bs_vec3 diff = bs_v3sub(*end, *start);

    float diff_p = bs_v3dot(plane->normal, diff);
    if (bs_abs(diff_p) <= EPSILON) return false;

    float distance = -bs_v3dot(plane->normal, plane->point);
    bs_vec3 projected = bs_v3muls(plane->normal, distance);

    float edge_factor = -bs_v3dot(plane->normal, bs_v3sub(*start, projected)) / diff_p;
    edge_factor = bs_min(bs_max(edge_factor, 0.0), 1.0);

    *out = bs_v3add(*start, bs_v3muls(diff, edge_factor));
    return true;
}

// random
float bs_randRange(float min, float max) {
    float val = ((float)rand() / RAND_MAX) * max + min;
    return val;
}

int bs_randRangeI(int min, int max) {
    int val = (rand() % (max - min + 1)) + max;
    return val;
}

bs_vec3 bs_randTrianglePt(bs_vec3 p0, bs_vec3 p1, bs_vec3 p2) {
    float a = bs_randRange(0, 1);
    float b = bs_randRange(0, 1);

    if((a + b) > 1.0) {
	    a = 1.0 - a;
	    b = 1.0 - b;
    }

    bs_vec3 v = bs_v3muls(bs_v3sub(p1, p0), a);
    bs_vec3 v0 = bs_v3muls(bs_v3sub(p2, p0), b);

    return bs_v3add(bs_v3add(p0, v), v0);
}