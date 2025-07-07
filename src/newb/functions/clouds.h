#ifndef CLOUDS_H
#define CLOUDS_H

#include "noise.h"
#include "sky.h"
#include "detection.h"


// Constants for shadertoy clouds
const float CLOUD4_SCALE = 1.1;
const float CLOUD4_SPEED = 0.03;
const float CLOUD4_DARK = 0.5;
const float CLOUD4_LIGHT = 0.3;
const float CLOUD4_COVER = 0.2;
const float CLOUD4_ALPHA = 8.0;
const float CLOUD4_SKYTINT = 0.5;
const mat2 CLOUD4_M = mat2( 1.6,  1.2, -1.2,  1.6 );


// simple clouds 2D noise
float cloudNoise2D(vec2 p, highp float t, float rain) {
  t *= NL_CLOUD1_SPEED;
  p += t;
  p.y += 3.0*sin(0.3*p.x + 0.1*t);

  vec2 p0 = floor(p);
  vec2 u = p-p0;
  u *= u*(3.0-2.0*u);
  vec2 v = 1.0-u;

  float n = mix(
    mix(rand(p0),rand(p0+vec2(1.0,0.0)), u.x),
    mix(rand(p0+vec2(0.0,1.0)),rand(p0+vec2(1.0,1.0)), u.x),
    u.y
  );
  n *= 0.5 + 0.5*sin(p.x*0.6 - 0.5*t)*sin(p.y*0.6 + 0.8*t);
  n = min(n*(1.0+rain), 1.0);
  return n*n;
}

// simple clouds
vec4 renderCloudsSimple(nl_skycolor skycol, vec3 pos, highp float t, float rain) {
  pos.xz *= NL_CLOUD1_SCALE;
  float d = cloudNoise2D(pos.xz, t, rain);
  vec4 col = vec4(skycol.horizonEdge + skycol.zenith, smoothstep(0.1,0.6,d));
  col.rgb += 1.5*dot(col.rgb, vec3(0.3,0.4,0.3))*smoothstep(0.6,0.2,d)*col.a;
  col.rgb *= 1.0 - 0.8*rain;
  return col;
}

// rounded clouds

// rounded clouds 3D density map
float cloudDf(vec3 pos, float rain, vec2 boxiness) {
  boxiness *= 0.999;
  vec2 p0 = floor(pos.xz);
  vec2 u = max((pos.xz-p0-boxiness.x)/(1.0-boxiness.x), 0.0);
  u *= u*(3.0 - 2.0*u);

  vec4 r = vec4(rand(p0), rand(p0+vec2(1.0,0.0)), rand(p0+vec2(1.0,1.0)), rand(p0+vec2(0.0,1.0)));
  r = smoothstep(0.1001+0.2*rain, 0.1+0.2*rain*rain, r); // rain transition

  float n = mix(mix(r.x,r.y,u.x), mix(r.w,r.z,u.x), u.y);

  // round y
  n *= 1.0 - 1.5*smoothstep(boxiness.y, 2.0 - boxiness.y, 2.0*abs(pos.y-0.5));

  n = max(1.25*(n-0.2), 0.0); // smoothstep(0.2, 1.0, n)
  n *= n*(3.0 - 2.0*n);
  return n;
}

vec4 renderCloudsRounded(
    vec3 vDir, vec3 vPos, float rain, float time, vec3 horizonCol, vec3 zenithCol,
    const int steps, const float thickness, const float thickness_rain, const float speed,
    const vec2 scale, const float density, const vec2 boxiness
) {
  float height = 7.0*mix(thickness, thickness_rain, rain);
  float stepsf = float(steps);

  // scaled ray offset
  vec3 deltaP;
  deltaP.y = 1.0;
  deltaP.xz = height*scale*vDir.xz/(0.02+0.98*abs(vDir.y));

  // local cloud pos
  vec3 pos;
  pos.y = 0.0;
  pos.xz = scale*(vPos.xz + vec2(1.0,0.5)*(time*speed));
  pos += deltaP;

  deltaP /= -stepsf;

  // alpha, gradient
  vec2 d = vec2(0.0,1.0);
  for (int i=1; i<=steps; i++) {
    float m = cloudDf(pos, rain, boxiness);
    d.x += m;
    d.y = mix(d.y, pos.y, m);
    pos += deltaP;
  }
  d.x *= smoothstep(0.03, 0.1, d.x);
  d.x /= (stepsf/density) + d.x;

  if (vPos.y < 0.0) { // view from top
    d.y = 1.0 - d.y;
  }

  vec4 col = vec4(zenithCol + horizonCol, d.x);
  col.rgb += dot(col.rgb, vec3(0.3,0.4,0.3))*d.y*d.y;
  col.rgb *= 1.0 - 0.8*rain;
  return col;
}

float cloudsNoiseVr(vec2 p, float t) {
  float n = fastVoronoi2(p + t, 1.8);
  n *= fastVoronoi2(3.0*p + t, 1.5);
  n *= fastVoronoi2(9.0*p + t, 0.4);
  n *= fastVoronoi2(27.0*p + t, 0.1);
  //n *= fastVoronoi2(82.0*pos + t, 0.02); // more quality
  return n*n;
}

vec4 renderClouds(vec2 p, float t, float rain, vec3 horizonCol, vec3 zenithCol, const vec2 scale, const float velocity, const float shadow) {
  p *= scale;
  t *= velocity;

  // layer 1
  float a = cloudsNoiseVr(p, t);
  float b = cloudsNoiseVr(p + NL_CLOUD3_SHADOW_OFFSET*scale, t);

  // layer 2
  p = 1.4 * p.yx + vec2(7.8, 9.2);
  t *= 0.5;
  float c = cloudsNoiseVr(p, t);
  float d = cloudsNoiseVr(p + NL_CLOUD3_SHADOW_OFFSET*scale, t);

  // higher = less clouds thickness
  // lower separation betwen x & y = sharper
  vec2 tr = vec2(0.6, 0.7) - 0.12*rain;
  a = smoothstep(tr.x, tr.y, a);
  c = smoothstep(tr.x, tr.y, c);

  // shadow
  b *= smoothstep(0.2, 0.8, b);
  d *= smoothstep(0.2, 0.8, d);

  vec4 col;
  col.a = a + c*(1.0-a);
  col.rgb = horizonCol + horizonCol.ggg;
  col.rgb = mix(col.rgb, 0.5*(zenithCol + zenithCol.ggg), shadow*mix(b, d, c));
  col.rgb *= 1.0-0.7*rain;

  return col;
}

// Custom shadertoy Clouds4 

// Hash function for noise
vec2 cloud4_hash( vec2 p ) {
    p = vec2(dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)));
    return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

// Noise function
float cloud4_noise( in vec2 p ) {
    const float K1 = 0.366025404; // (sqrt(3)-1)/2;
    const float K2 = 0.211324865; // (3-sqrt(3))/6;
    vec2 i = floor(p + (p.x+p.y)*K1);    
    vec2 a = p - i + (i.x+i.y)*K2;
    vec2 o = (a.x>a.y) ? vec2(1.0,0.0) : vec2(0.0,1.0);
    vec2 b = a - o + K2;
    vec2 c = a - 1.0 + 2.0*K2;
    vec3 h = max(0.5-vec3(dot(a,a), dot(b,b), dot(c,c)), 0.0 );
    vec3 n = h*h*h*h*vec3( dot(a,cloud4_hash(i+0.0)), dot(b,cloud4_hash(i+o)), dot(c,cloud4_hash(i+1.0)));
    return dot(n, vec3(70.0, 70.0, 70.0));    
}

// Fractional Brownian Motion
float cloud4_fbm(vec2 n) {
    float total = 0.0, amplitude = 0.1;
    for (int i = 0; i < 7; i++) {
        total += cloud4_noise(n) * amplitude;
        n = mul(CLOUD4_M, n);
        amplitude *= 0.4;
    }
    return total;
}

// Main cloud rendering function for type 4
vec4 renderCloudsShadertoy(vec3 pos, highp float t, float rain, vec3 horizonCol, vec3 zenithCol) {
    vec2 uv = pos.xz * CLOUD4_SCALE;
    float time = t * CLOUD4_SPEED;
    
    // Initial distortion
    float q = cloud4_fbm(uv * 0.5);
    
    // Ridged noise
    float r = 0.0;
    vec2 ridgedUV = uv;
    ridgedUV -= q - time;
    float weight = 0.8;
    for (int j=0; j<8; j++) {
        r += abs(weight*cloud4_noise(ridgedUV));
        ridgedUV = mul(CLOUD4_M, ridgedUV) + time;
        weight *= 0.7;
    }
    
    // Base noise
    float f = 0.0;
    vec2 noiseUV = uv;
    noiseUV -= q - time;
    weight = 0.7;
    for (int k=0; k<8; k++) {
        f += weight*cloud4_noise(noiseUV);
        noiseUV = mul(CLOUD4_M, noiseUV) + time;
        weight *= 0.6;
    }
    
    f *= r + f;
    
    // Color variation
    float c = 0.0;
    time = t * CLOUD4_SPEED * 2.0;
    vec2 colorUV = uv * 2.0;
    colorUV -= q - time;
    weight = 0.4;
    for (int m=0; m<7; m++) {
        c += weight*cloud4_noise(colorUV);
        colorUV = mul(CLOUD4_M, colorUV) + time;
        weight *= 0.6;
    }
    
    // Ridge highlights
    float c1 = 0.0;
    time = t * CLOUD4_SPEED * 3.0;
    vec2 ridgeUV = uv * 3.0;
    ridgeUV -= q - time;
    weight = 0.4;
    for (int i=0; i<7; i++) {
        c1 += abs(weight*cloud4_noise(ridgeUV));
        ridgeUV = mul(CLOUD4_M, ridgeUV) + time;
        weight *= 0.6;
    }
    
    c += c1;
    
    // Combine with sky colors
    vec3 skycolour = mix(zenithCol, horizonCol, smoothstep(0.0, 0.2, pos.y));
    vec3 cloudcolour = vec3(1.1, 1.1, 0.9) * clamp((CLOUD4_DARK + CLOUD4_LIGHT*c), 0.0, 1.0);
    
    // Final cloud density
    f = CLOUD4_COVER + CLOUD4_ALPHA*f*r;
    
    // Blend sky and clouds
    vec3 result = mix(skycolour, clamp(CLOUD4_SKYTINT * skycolour + cloudcolour, 0.0, 1.0), clamp(f + c, 0.0, 1.0));
    
    // Rain effect (make clouds darker and less visible)
    result *= 1.0 - 0.7*rain;
    
    return vec4(result, 1.0);
}

// aurora is rendered on clouds layer
#ifdef NL_AURORA
vec4 renderAurora(vec3 p, float t, float rain, vec3 FOG_COLOR) {
  // if (rain > 0.5) return vec4(0.0, 0.0, 0.0, 0.0);
  t *= NL_AURORA_VELOCITY;
  p.xz *= NL_AURORA_SCALE;
  p.xz += 0.05*sin(p.x*4.0 + 20.0*t);

  float d0 = sin(p.x*0.1 + t + sin(p.z*0.2));
  float d1 = sin(p.z*0.1 - t + sin(p.x*0.2));
  float d2 = sin(p.z*0.1 + 1.0*sin(d0 + d1*2.0) + d1*2.0 + d0*1.0);
  d0 *= d0; d1 *= d1; d2 *= d2;
  d2 = d0/(1.0 + d2/NL_AURORA_WIDTH);

  float mask = (1.0-0.8*rain)*max(1.0 - 4.0*max(FOG_COLOR.b, FOG_COLOR.g), 0.0);
  return vec4(NL_AURORA*mix(NL_AURORA_COL1,NL_AURORA_COL2,d1),1.0)*d2*mask;
}
#endif

#endif
