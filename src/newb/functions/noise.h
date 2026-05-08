#ifndef NOISE_H
#define NOISE_H

#include "constants.h"
SAMPLER2D_AUTOREG(s_NoiseTexture);

// functions under [1] are from https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83

// [1] hash function for noise (for highp only)
float rand(highp vec2 n) {
  return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

// 1D noise - used in plants,lantern wave
float noise1D(highp float x) {
  return texture2DLod(s_NoiseTexture, vec2_splat(x)*0.0001, 0).g;
}

// simpler rand for disp, puddles
float fastRand(vec2 n){
  return fract(37.45*sin(dot(n, vec2(4.36, 8.28))));
}

// water displacement map (also used by caustic)
float disp(vec3 pos, float t) {
  float n = sin(8.0*PI_HALF*(pos.x+pos.y*pos.z) + 0.7*t);
  pos.y += t + 0.8*n;
  float p = floor(pos.y);
  return (0.8+0.2*n) * mix(fastRand(pos.xz+p), fastRand(pos.xz+p+1.0), pos.y - p);
}

// [1]
float noise2D(vec2 u) {
  return texture2DLod(s_NoiseTexture, u*0.01, 0).b;
}

vec4 mod289(vec4 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 perm(vec4 x) {
  return mod289(((x * 34.0) + 1.0) * x);
}

// [1] used by galaxy
float noise3D(vec3 p){
  vec3 w = abs(p);
  w = w / (w.x + w.y + w.z);

  vec3 n = vec3(
    texture2DLod(s_NoiseTexture, p.yz*0.01, 0).b*w.x, 
    texture2DLod(s_NoiseTexture, p.xz*0.01, 0).b*w.y, 
    texture2DLod(s_NoiseTexture, p.xy*0.01, 0).b*w.z
    );

  return n.x + n.y + n.z;
}

float fastVoronoi2(vec2 pos, float f) {
  return 1.0-f*texture2DLod(s_NoiseTexture, pos.xy*0.1, 0).r;
}

#endif
