#ifndef MATRIX_H
#define MATRIX_H

#include "fmath.h"

typedef struct
{
    float m[4][4];
} mat4x4_t;

#ifdef __cplusplus
extern "C" {
#endif

inline void mat4x4_make_projection(mat4x4_t *d, float fovy, float aspect, float z_near, float z_far)
{
    assert(fovy != 0.0f);
    assert(aspect != 0.0f);
    assert(z_near != z_far);

	float sine, cosine, cotangent, deltaZ;
	float radians = fovy / 2 * (float)M_PI / 180;
	deltaZ = z_far - z_near;
    fm_sincosf(radians, &sine, &cosine);
	cotangent = cosine / sine;

    d->m[0][0] = cotangent / aspect;
    d->m[0][1] = 0;
    d->m[0][2] = 0;
    d->m[0][3] = 0;

    d->m[1][0] = 0;
    d->m[1][1] = cotangent;
    d->m[1][2] = 0;
    d->m[1][3] = 0;

    d->m[2][0] = 0;
    d->m[2][1] = 0;
    d->m[2][2] = -(z_far + z_near) / deltaZ;
    d->m[2][3] = -1;

    d->m[3][0] = 0;
    d->m[3][1] = 0;
    d->m[3][2] = -2 * z_near * z_far / deltaZ;
    d->m[3][3] = 0;
}

inline void mat4x4_make_rotation_translation(mat4x4_t *d, const float position[3], const float rotation[4])
{
    float tx = position[0];
    float ty = position[1];
    float tz = position[2];

    float qx = rotation[0];
    float qy = rotation[1];
    float qz = rotation[2];
    float qw = rotation[3];

    d->m[0][0] = (1 - 2 * qy*qy - 2 * qz*qz);
    d->m[0][1] = (2 * qx*qy + 2 * qz*qw);
    d->m[0][2] = (2 * qx*qz - 2 * qy*qw);
    d->m[0][3] = 0.f;

    d->m[1][0] = (2 * qx*qy - 2 * qz*qw);
    d->m[1][1] = (1 - 2 * qx*qx - 2 * qz*qz);
    d->m[1][2] = (2 * qy*qz + 2 * qx*qw);
    d->m[1][3] = 0.f;

    d->m[2][0] = (2 * qx*qz + 2 * qy*qw);
    d->m[2][1] = (2 * qy*qz - 2 * qx*qw);
    d->m[2][2] = (1 - 2 * qx*qx - 2 * qy*qy);
    d->m[2][3] = 0.f;

    d->m[3][0] = tx;
    d->m[3][1] = ty;
    d->m[3][2] = tz;
    d->m[3][3] = 1.f;
}

inline void mat4x4_make_translation_rotation(mat4x4_t *d, const float position[3], const float rotation[4])
{
    // TODO
    float tx = position[0];
    float ty = position[1];
    float tz = position[2];

    float qx = rotation[0];
    float qy = rotation[1];
    float qz = rotation[2];
    float qw = rotation[3];

    d->m[0][0] = (1 - 2 * qy*qy - 2 * qz*qz);
    d->m[0][1] = (2 * qx*qy + 2 * qz*qw);
    d->m[0][2] = (2 * qx*qz - 2 * qy*qw);
    d->m[0][3] = 0.f;

    d->m[1][0] = (2 * qx*qy - 2 * qz*qw);
    d->m[1][1] = (1 - 2 * qx*qx - 2 * qz*qz);
    d->m[1][2] = (2 * qy*qz + 2 * qx*qw);
    d->m[1][3] = 0.f;

    d->m[2][0] = (2 * qx*qz + 2 * qy*qw);
    d->m[2][1] = (2 * qy*qz - 2 * qx*qw);
    d->m[2][2] = (1 - 2 * qx*qx - 2 * qy*qy);
    d->m[2][3] = 0.f;

    d->m[3][0] = tx;
    d->m[3][1] = ty;
    d->m[3][2] = tz;
    d->m[3][3] = 1.f;
}

inline void mat4x4_make_scale_rotation_translation(mat4x4_t *d, const float position[3], const float rotation[4], const float scale[3])
{
    float tx = position[0];
    float ty = position[1];
    float tz = position[2];

    float qx = rotation[0];
    float qy = rotation[1];
    float qz = rotation[2];
    float qw = rotation[3];
    
    float sx = scale[0];
    float sy = scale[1];
    float sz = scale[2];

    d->m[0][0] = (1 - 2 * qy*qy - 2 * qz*qz) * sx;
    d->m[0][1] = (2 * qx*qy + 2 * qz*qw) * sx;
    d->m[0][2] = (2 * qx*qz - 2 * qy*qw) * sx;
    d->m[0][3] = 0.f;

    d->m[1][0] = (2 * qx*qy - 2 * qz*qw) * sy;
    d->m[1][1] = (1 - 2 * qx*qx - 2 * qz*qz) * sy;
    d->m[1][2] = (2 * qy*qz + 2 * qx*qw) * sy;
    d->m[1][3] = 0.f;

    d->m[2][0] = (2 * qx*qz + 2 * qy*qw) * sz;
    d->m[2][1] = (2 * qy*qz - 2 * qx*qw) * sz;
    d->m[2][2] = (1 - 2 * qx*qx - 2 * qy*qy) * sz;
    d->m[2][3] = 0.f;

    d->m[3][0] = tx;
    d->m[3][1] = ty;
    d->m[3][2] = tz;
    d->m[3][3] = 1.f;
}

inline void mat4x4_mult_vec(float *d, const mat4x4_t *m, const float *v)
{
    d[0] = m->m[0][0] * v[0] + m->m[1][0] * v[1] + m->m[2][0] * v[2] + m->m[3][0] * v[3];
    d[1] = m->m[0][1] * v[0] + m->m[1][1] * v[1] + m->m[2][1] * v[2] + m->m[3][1] * v[3];
    d[2] = m->m[0][2] * v[0] + m->m[1][2] * v[1] + m->m[2][2] * v[2] + m->m[3][2] * v[3];
    d[3] = m->m[0][3] * v[0] + m->m[1][3] * v[1] + m->m[2][3] * v[2] + m->m[3][3] * v[3];
}

inline void mat4x4_mult(mat4x4_t *d, const mat4x4_t *l, const mat4x4_t *r)
{
    mat4x4_mult_vec(d->m[0], l, r->m[0]);
    mat4x4_mult_vec(d->m[1], l, r->m[1]);
    mat4x4_mult_vec(d->m[2], l, r->m[2]);
    mat4x4_mult_vec(d->m[3], l, r->m[3]);
}

inline void mat4x4_transpose_inverse(mat4x4_t *d, const mat4x4_t *m)
{
    // TODO
}

#ifdef __cplusplus
}
#endif

#endif
