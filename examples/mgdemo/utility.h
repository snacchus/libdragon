#ifndef UTILITY_H
#define UTILITY_H

#define ARRAY_COUNT(a) (sizeof(a)/sizeof(a[0]))

inline float wrap_angle(float angle)
{
    float a = fmodf(angle, 360.0f);
    return a < 0.0f ? a + 360.0f : a;
}

#endif
