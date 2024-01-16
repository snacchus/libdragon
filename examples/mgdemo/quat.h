#ifndef QUAT_H
#define QUAT_H

#include "fmath.h"

typedef struct
{
    float v[4];
} quat_t;

#ifdef __cplusplus
extern "C" {
#endif

inline void quat_make_identity(quat_t *d)
{
    d->v[0] = 0.0f;
    d->v[1] = 0.0f;
    d->v[2] = 0.0f;
    d->v[3] = 1.0f;
}

inline void quat_from_axis_angle(quat_t *d, const float axis[3], float angle)
{
    float sine, cosine;
    fm_sincosf(angle / 2.0f, &sine, &cosine);
    d->v[0] = sine * axis[0];
    d->v[1] = sine * axis[1];
    d->v[2] = sine * axis[2];
    d->v[3] = cosine;
}

inline void quat_from_euler_zyx(quat_t *d, float x, float y, float z)
{
    float sx, cx, sy, cy, sz, cz;

    fm_sincosf(x, &sx, &cx);
    fm_sincosf(y, &sy, &cy);
    fm_sincosf(z, &sz, &cz);

    sx *= 0.5f;
    cx *= 0.5f;
    sy *= 0.5f;
    cy *= 0.5f;
    sz *= 0.5f;
    cz *= 0.5f;

    d->v[0] = cx * sz * cy - sx * cz * sy;
    d->v[1] = cx * cz * sy + sx * sz * cy;
    d->v[2] = sx * cz * cy - cx * sz * sy;
    d->v[3] = cx * cz * cy + sx * sz * sy;
}

inline void quat_inverse(quat_t *d, const quat_t *q)
{
    float inv_mag2 = 1.0f / (q->v[0]*q->v[0] + q->v[1]*q->v[1] + q->v[2]*q->v[2] + q->v[3]*q->v[3]);

    d->v[0] = -q->v[0] * inv_mag2;
    d->v[1] = -q->v[1] * inv_mag2;
    d->v[2] = -q->v[2] * inv_mag2;
    d->v[3] = q->v[3] * inv_mag2;
}

inline void quat_inverse_in_place(quat_t *d)
{
    quat_inverse(d, d);
}

#ifdef __cplusplus
}
#endif

#endif
