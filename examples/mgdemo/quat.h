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
    float xs, xc, ys, yc, zs, zc;

    fm_sincosf(x * 0.5f, &xs, &xc);
    fm_sincosf(y * 0.5f, &ys, &yc);
    fm_sincosf(z * 0.5f, &zs, &zc);

    d->v[0] =  xs * yc * zc - xc * ys * zs;
    d->v[1] =  xc * ys * zc + xs * yc * zs;
    d->v[2] = -xs * ys * zc + xc * yc * zs;
    d->v[3] =  xc * yc * zc + xs * ys * zs;
}

inline void quat_inverse(quat_t *d, const quat_t *q)
{
    float inv_mag2 = 1.0f / (q->v[0]*q->v[0] + q->v[1]*q->v[1] + q->v[2]*q->v[2] + q->v[3]*q->v[3]);

    d->v[0] = -q->v[0] * inv_mag2;
    d->v[1] = -q->v[1] * inv_mag2;
    d->v[2] = -q->v[2] * inv_mag2;
    d->v[3] =  q->v[3] * inv_mag2;
}

inline void quat_inverse_in_place(quat_t *d)
{
    quat_inverse(d, d);
}

#ifdef __cplusplus
}
#endif

#endif
