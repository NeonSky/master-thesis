#pragma once

const float GRAVITY = 9.82f;
const float METERS_TO_UNREAL_UNITS = 100.0f;
const float HORSEPOWER_TO_NEWTON = 750.0f;
const float DENSITY_OF_WATER = 1000.0f;

inline float RECOVER_F32(FFloat16Color c) {

    uint32_t p1  = uint32_t(c.R.GetFloat());
    uint32_t p2  = uint32_t(c.G.GetFloat());
    uint32_t p3  = uint32_t(c.B.GetFloat());
    uint32_t p4  = uint32_t(c.A.GetFloat());
    uint32_t v = (p4 << 24 | p3 << 16 | p2 << 8 | p1);

    float res;
    memcpy(&res, &v, sizeof(float));

    return res;
}