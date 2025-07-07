#ifndef CUSTOM_H
#define CUSTOM_H
#include "noise.h"

/* Define */
#define pi radians(180.0)
#define hpi (pi / 2.0)
#define tau (pi * 2.0)
#define phi (0.5*sqrt(5.0) + 0.5)
#define rlog2 (1.0 / log(2.0))
#define batre 0.96
#define charge 9.2

//rain drop effect starts
/* // Screen Effects
float stickyRaindrop(vec2 uv, vec2 center, float baseSize, float stretch) {
    vec2 p = (uv - center);
    p.y /= baseSize * stretch;
    p.x /= baseSize;

    float d = length(p * vec2(1.0, 0.6));  // horizontal squash
    float y = p.y;

    float shape = smoothstep(0.5, 0.45, d) *
                  smoothstep(0.0, 1.0, y + 0.5) *
                  smoothstep(1.0, -0.3, y);

    return shape;
}

vec3 RainDrop(vec4 diffuse, float time, vec2 uv) {
    vec3 baseColor = diffuse.rgb;
    vec3 kol = baseColor;
    const int drops = DROP_NUMBER;

    for (int i = 0; i < drops; i++) {
        float fi = float(i);
        vec2 dropPos = vec2(
            fract(sin(fi * 12.9898) * 43758.5453),
            fract(sin(fi * 78.233) * 12345.678 + time * 0.1)
        );
        dropPos.y = mod(dropPos.y - time * 0.12, 1.0);

        float baseSize = 0.05 + 0.02 * fract(sin(fi * 5.21) * 1000.0);
        float stretch = mix(1.0, 2.5, smoothstep(0.1, 0.9, dropPos.y));
        float dropMask = stickyRaindrop(uv, dropPos, baseSize, stretch);
        vec3 dropColor = vec3(0.4, 0.6, 0.9);
        kol = mix(kol, dropColor, dropMask * 0.6);
    }
    return kol;
} */

// Color/Lighting Calculations
float lumaGrayscale(vec3 d) {
    return dot(d, vec3(0.299, 0.587, 0.114));
}


float ggx(vec3 N, vec3 V, vec3 L, float roughness, float F0) {
    vec3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    float a = roughness * roughness;
    float a2 = a * a;
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    float D = a2 / (3.141592 * denom * denom);

    float k = (roughness + 1.0);
    k = (k * k) / 8.0;
    float G_V = NdotV / (NdotV * (1.0 - k) + k);
    float G_L = NdotL / (NdotL * (1.0 - k) + k);
    float G = G_V * G_L;

    float F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
    return (D * G * F) / max(4.0 * NdotL * NdotV, 0.001);
}

// World Calculation
vec3 getNormal(sampler2D TEXTURE_0, vec2 uv) {
    vec3 base0 = texture2D(TEXTURE_0, uv).rgb;
    highp float base = lumaGrayscale(base0);
    highp float w1 = base - lumaGrayscale(texture2D(TEXTURE_0, vec2(uv.x-0.00018, uv.y)).rgb)
    highp float w2 = (base - lumaGrayscale(texture2D(TEXTURE_0, vec2(uv.x, uv.y-0.00018)).rgb));
    return normalize(vec3(w1, w2, 1.0)) * 0.5 + 0.5;
}

mat3 getTBN(vec3 normal) {
    vec3 T = vec3(abs(normal.y) + normal.z, 0.0, normal.x);
    vec3 B = vec3(0.0, -abs(normal).x - abs(normal).z, abs(normal).y);
    return transpose(mat3(T, B, normal));
}

// PBR Functions
float DistributionGGX(vec3 n, vec3 h, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(n, h), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (3.141592 * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}



/* Metal Blocks Reflection 
   or fake pbr and ray tracing in other 
   words 
   -- Real Deal */
/* #if !defined(TRANSPARENT) && !defined(ALPHA_TEST)
float metallic = metal ? 1.0 : 0.0;
if(metal && !tree){
    vec3 sky = nlRenderSky(skycol, env, -reflectNormal, FogColor.rgb, v_underwaterRainTime.z,L);
     vec4 clouds = renderClouds(reflectNormal.xz, 0.1*ViewPositionAndTime.w, v_underwaterRainTime.y, skycol.horizonEdge, skycol.zenith, NL_CLOUD3_SCALE, NL_CLOUD3_SPEED, NL_CLOUD3_SHADOW);
    vec3 refl = sky;
    
    vec3 baseColor = texcol.rgb;
  vec3 dielectricF0 = vec3(0.4,0.4,0.4); // typical for non-metals
  vec3 f0 = mix(diffuse.rgb,dielectricF0, metallic);
    refl = refl * clouds.a*9.0 + clouds.rgb;
    refl = mix(diffuse.rgb, refl, f0);
    
    float fresnel = pow(1.0 - dot(V, worldNormal), 0.4);
    refl = (refl * (dirlight(dir,rain,night) + nDotL));
    refl = refl  * fresnel + specular * sunLight ;
 diffuse.rgb = refl;
 
 }
 #endif*/

    
#endif