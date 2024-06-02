#ifndef UTILITY_H
#define UTILITY_H

#include <math.h>

#define ARRAY_COUNT(a) (sizeof(a)/sizeof(a[0]))

inline float wrap_angle(float angle)
{
    float a = fmodf(angle, 360.0f);
    return a < 0.0f ? a + 360.0f : a;
}

inline float vec3_dot(const float *va, const float *vb)
{
    return va[0]*vb[0] + va[1]*vb[1] + va[2]*vb[2];
}

inline void vec3_normalize(float *dst, const float *vec)
{
    float magnitude = sqrtf(vec3_dot(vec, vec));
    dst[0] = vec[0] / magnitude;
    dst[1] = vec[1] / magnitude;
    dst[2] = vec[2] / magnitude;
}

inline void vec3_cross(float *dst, const float *va, const float *vb)
{
    dst[0] = (va[1] * vb[2] - va[2] * vb[1]);
    dst[1] = (va[2] * vb[0] - va[0] * vb[2]);
    dst[2] = (va[0] * vb[1] - va[1] * vb[0]);
}

#endif
