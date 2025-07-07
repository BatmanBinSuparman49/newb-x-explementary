#ifndef BLACKHOLE_H
#define BLACKHOLE_H


#include "constants.h"

#ifndef PI
    #define PI 3.141592
#endif

// Unique namespace for all black hole functions
#define BH_FUNC(name) bh_##name

// Black Hole specific noise functions
float BH_FUNC(rand)(vec2 co) {
    return fract(sin(dot(co.xy, vec2(89.42, 37.91))) * 58321.3784); // Different constants than global rand()
}

float BH_FUNC(noise)(vec2 p) {
    vec2 ip = floor(p);
    vec2 fp = fract(p);
    float a = BH_FUNC(rand)(ip);
    float b = BH_FUNC(rand)(ip + vec2(1.0, 0.0));
    float c = BH_FUNC(rand)(ip + vec2(0.0, 1.0));
    float d = BH_FUNC(rand)(ip + vec2(1.0, 1.0));
    vec2 u = fp * fp * (3.0 - 2.0 * fp);
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

// Black Hole SDF functions
float BH_FUNC(sdSphere)(vec3 p, float s) {
    return length(p)-s;
}

float BH_FUNC(sdCappedCylinder)(vec3 p, vec2 h) {
    vec2 d = abs(vec2(length(p.xz), p.y)) - h;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

float BH_FUNC(sdTorus)(vec3 p, vec2 t) {
    vec2 q = vec2(length(p.xz)-t.x, p.y);
    return length(q)-t.y;
}

vec4 renderBlackhole(vec3 viewDir, float time) {
    // Camera setup
    vec3 lookAt = vec3(0.0, -0.1, 0.0);
    
    // Camera orbit parameters
    float eyer = 2.0;
    float eyea = (0.5) * PI * 2.0;
    float eyea2 = (0.24) * PI * 2.0;
    
    // Camera position
    vec3 ro = vec3(
        eyer * cos(eyea) * sin(eyea2),
        eyer * cos(eyea2),
        eyer * sin(eyea) * sin(eyea2));
    
    // Camera orientation
    vec3 front = normalize(lookAt - ro);
    vec3 left = normalize(cross(normalize(vec3(0.0, 1.0, -0.1)), front));
    vec3 up = normalize(cross(front, left));
    vec3 rd = normalize(viewDir);
    
    // Black hole parameters
    vec3 bh = vec3(0.0, 0.0, 0.0);
    float bhr = 0.1;
    float bhmass = 5.0 * 0.001;
    
    // Raymarching variables
    vec3 p = ro;
    vec3 pv = rd;
    float dt = 0.02;
    vec3 col = vec3(0.0, 0.0, 0.0);
    float noncaptured = 1.0;
    
    // Color parameters
    vec3 c1 = vec3(0.5, 0.46, 0.4);
    vec3 c2 = vec3(1.0, 0.8, 0.6);
    
    // Raymarching loop
    for(float t = 0.0; t < 1.0; t += 0.005) {
        p += pv * dt * noncaptured;
        vec3 bhv = bh - p;
        float r = dot(bhv, bhv);
        
        // Gravity effect
        pv += normalize(bhv) * (bhmass / r);
        noncaptured = smoothstep(0.0, 0.666, BH_FUNC(sdSphere)(p-bh, bhr));
        
        // Accretion disk with procedural noise
        float dr = length(bhv.xz);
        float da = atan2(bhv.x, bhv.z);
        vec2 diskUV = vec2(
            log(dr/bhr + 1.0) * 5.0,
            da/(2.0*PI) + time*0.05
        );
        
        // Multi-octave noise for turbulence
        float n = BH_FUNC(noise)(diskUV * 5.0);
        n += 0.5 * BH_FUNC(noise)(diskUV * 10.0 + vec2(time*0.1, 0.0));
        n += 0.25 * BH_FUNC(noise)(diskUV * 20.0 + vec2(0.0, time*0.2));
        n = smoothstep(0.3, 0.7, n);
        
        // Temperature gradient
        float temp = 1.0 - smoothstep(0.0, 1.0, (dr - bhr)/0.5);
        
        // Disk colors with noise
        vec3 dcol = mix(c2, c1, pow(length(bhv)-bhr, 2.0)) * 
                   n * (4.0 / (0.001 + (length(bhv) - bhr)*50.0));
        
        // Disk geometry
        float diskMask = smoothstep(0.0, 1.0, 
            -BH_FUNC(sdTorus)((p * vec3(1.0, 25.0, 1.0)) - bh, vec2(0.8, 0.99)));
        
        col += max(vec3(0.0, 0.0, 0.0), dcol * diskMask * noncaptured);
        
        // Gravitational lensing glow
        col += vec3(1.0, 0.9, 0.85) * (1.0/r) * 0.0033 * noncaptured;
    }
    
    return vec4(col, noncaptured);
}

#undef BH_FUNC // Clean up namespace macro

#endif