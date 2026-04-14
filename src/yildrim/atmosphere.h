#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

#include "cloud.h"

/* Sun & Moon */
vec3 sunS(vec3 sunDir, vec3 viewDir, float dusk, float dawn) {
  float sunDot = max(0.0, 1.0 - dot(sunDir, viewDir));
  float m = 0.0075 / (0.0001 + sunDot);
  m = pow(m, 1.2) * 0.1;

  float sunsetFactor = saturate(sunDir.y * 2.5); 
  float transition = pow(sunsetFactor, 2.0); 
    
  vec3 sunRed = vec3(4.0, 0.3, 0.02);
  vec3 sunDay = vec3(3.2, 2.8, 2.2); 
  vec3 currentSunCol = mix(sunRed, sunDay, transition);

  vec3 sunCol = vec3(1.0, 0.95, 0.85);
  vec3 dawnCol  = vec3(1.0, 0.35, 0.05); 
  sunCol = mix(sunCol, currentSunCol, saturate(dawn+dusk));

  return sunCol * m;
}

vec3 getMoon(vec3 moonDir, vec3 viewDir, float night){
    float moonDot = saturate(dot(moonDir, viewDir));
    float core =   pow(smoothstep(0.998, 1.0, moonDot), 0.26);
    float corona = pow(moonDot, 32.0) * max(0.8 - 0.7 * night, 0.0);
    float outerGlow = pow(moonDot, 2.0) * max(0.3 - 0.2 * night, 0.0);
    float moon = core + corona + outerGlow;
    vec3 moonCol = vec3(0.8, 0.9, 1.0);

    return moonCol * (core * 20.0 + corona * 2.0 + outerGlow * 1.5);
}

/*
	Non-physical based atmospheric scattering (dynamic)
	Edited by KagzuGraphics
	Based on open-source sky shader concept
	Fun fact: His first (and only till now) personal project on Shadertoy :)
*/

// • Edit by Kagzu Graphic
// ==========================================================
// Global Adjustments
// ==========================================================
#define BRIGHTNESS      1.06
#define COLOR_CONTRAST  1.2
#define SATURATION      1.4

#define pI 3.1415926535897932384626433832795
#define saturate(x) clamp(x, 0.0, 1.0)

#define starCol vec3(1.0, 0.98, 0.97);

// ==========================================================
// Utility Functions
// ==========================================================
float getSunPoint(vec3 p, vec3 lp) {  // not used, sunS is used
    return smoothstep(0.03, 0.026, distance(p, lp)) * 50.0;
} 

float getMie(vec3 p, vec3 lp) {
    float d = distance(p, lp);
    // Exponential falloff mimics the dense core of the atmosphere
    return exp(-d * 5.0) * 2.0; 
}

float hash13S(highp vec3 p) {
    p = fract(p * 0.1031);
    p += dot(p, p.zyx + 31.32);
    return fract((p.x + p.y) * p.z);
}

float getStars(highp vec3 pos) {
    pos = floor((abs(pos) + 16.0) * 265.0);
    return smoothstep(0.998, 1.0, hash13S(pos));
}

float particleThickness(float depth) {
    depth = max(depth + 0.8, 0.01);
    return 1.0 / depth;
}

vec3 calcAbsorption(vec3 x, float y) {
    vec3 absorption = x * -y;
    absorption = exp2(absorption);
    return absorption;
}

// ==========================================================
// Atmospheric Layers
// ==========================================================
vec3 dusk(vec3 rd) {
    float yd = min(rd.y, 0.0);
    rd.y = max(rd.y, 0.0);
    vec3 col = vec3(0.67, 0.4 - exp(-rd.y * 50.0) * 0.15, 0.1) * exp(-rd.y * 15.0);
    col += vec3(1.0, 0.5, 0.5) * (1.0 - exp(-rd.y * 8.0)) * exp(-rd.y * 0.9);
    return mix(col * 1.6, vec3_splat(0.3), 1.0 - exp(yd * 100.0));
}

vec3 day(vec3 rd, vec3 L) {
    float yd = min(rd.y, 0.0);
    rd.y = max(rd.y, 0.0);
    vec3 col = vec3(0.56, 0.57 - exp(-rd.y * 16.0) * 0.06, 1.0) * exp(-rd.y * 4.5);
    col += vec3(0.2, 0.3, 0.6) * 1.1 * (1.0 - exp(-rd.y * 4.0)) * exp(-rd.y * 0.9);
    col = mix(col * 1.1, vec3(0.2, 0.3, 0.55) * 0.55, 1.0 - exp(yd * 6.0) * 0.7);
    col = mix(col * dusk(rd), col, saturate(L.y));
    return col;
}

vec3 night(vec3 rd, vec3 L) {
    float yd = min(rd.y, 0.0);
    rd.y = max(rd.y, 0.0);

    vec3 col = vec3(0.0, 0.0, 0.0);
    col += vec3(0.4, 0.53 - exp(-rd.y * 200.0) * 0.04, 0.6) * 0.8 * exp(-rd.y * 5.0);
    col += vec3(0.0, 0.08, 0.2) * 0.3 * (1.0 - exp(-rd.y * 10.0)) * exp(-rd.y);
    col  = mix(col * 1.2, vec3(0.0, 0.08, 0.15), 1.0 - exp(yd * 15.0));
    return col;
}

// ==========================================================
// Sky Computation
// ==========================================================
vec3 GetSky(sampler2D NOISE_0, vec3 V, vec3 L, vec3 SunMoonDir, float dayFactor, float nightFactor, float dusk, float dawn, float cirrusFactor) {
    vec3 dSky = day(V, L) * 3.0;
    vec3 nSky = night(V, L) * 0.5;

    float a = dayFactor;
    float b = nightFactor;
    float depthView  = max(V.y, 0.0);
    float depthLight = max(L.y * 0.5 + 0.02, 0.01); 
    float VoL = distance(V, L);

    float coeff   = mix(mix(1.0, 0.05, a), 3.0, b); 
    float scatter = exp(-VoL * coeff);
    scatter       = mix(scatter, 0.0, b);

    vec3 abso    = calcAbsorption(mix(nSky, dSky, scatter), depthView);
    vec3 absoSun = calcAbsorption(mix(nSky, dSky, scatter), depthLight);

    float sunsetFactor = saturate(L.y * 2.5); 
    float transition = pow(sunsetFactor, 2.0); 
    
    vec3 sunRed = vec3(4.0, 0.3, 0.02);
    vec3 sunDay = vec3(3.2, 2.8, 2.2); 
    vec3 currentSunCol = mix(sunRed, sunDay, transition);

    vec3 sun = sunS(L, V, dusk, dawn) * currentSunCol * absoSun;
    sun *= exp(min(V.y, 0.0) * 100.0);

    vec3 mie = getMie(V, L) * currentSunCol * 0.5 * absoSun;
    mie *= exp(min(V.y, 0.0) * 50.0);

    float stars = mix(getStars(V), 0.0, a);

    a += saturate(dawn+dusk);
    sun *= a;
    mie *= a;

    vec2 uvC = V.xz/V.y;
    vec3 cirrusCol = vec3(1.0, 0.8, 0.75)*dayFactor + vec3(1.0, 0.35, 0.05)*saturate(dawn+dusk) + vec3(0.5765, 0.584, 0.98)*nightFactor;
    vec4 Cirrus = cirrus(NOISE_0, uvC, cirrusCol, SunMoonDir, V) * cirrusFactor;

    vec3 atmosphere = mix(nSky, dSky, scatter);
    atmosphere = mix(atmosphere, Cirrus.rgb*mix(1.5, 1.0, nightFactor), Cirrus.a);

    atmosphere = mix(atmosphere, sun, sun) + mie;

    vec3 spaceColor = vec3(0.1, 0.15, 0.35); 
    if (V.y > 0.0)
        atmosphere += spaceColor * stars * nightFactor;

    return atmosphere;
}

vec3 getAtmosphere(sampler2D NOISE_0, vec3 V, vec3 L, vec3 SunMoonDir, float day, float night, float dusk, float dawn, float cirrusFactor) {
    vec3 sky = GetSky(NOISE_0, V, L, SunMoonDir, day, night, dusk, dawn, cirrusFactor) * BRIGHTNESS;
    sky = 1.0 - exp(-1.2 * sky);
    return sky;
}

/* Tyro's physically based Atmospheric Scattering | not used */
// Flaws - i) Sky below horizon is black 
//        ii) It is heavy and difficult to configure as it is physically based 

#define SKY_BRIGHTNESS 15.0

vec2 raysi(vec3 r0, vec3 rd, float sr) {
    // ray-sphere intersection that assumes
    // the sphere is centered at the origin.
    // No intersection when result.x > result.y
    float a = dot(rd, rd);
    float b = 1. * dot(rd, r0);
    float c = dot(r0, r0) - (sr * sr);
    float d = (b*b) - 4.0*a*c;
    if (d < 0.0) return vec2(1e2,-1e5);
    return vec2(
        (-b - sqrt(d))/(2.0*a),
        (-b + sqrt(d))/(2.0*a)
    );
}


vec3 atmosphere(vec3 r, vec3 r0,
vec3 pSun, float iSun,
float rPlanet, float rAtmos,
vec3 kRlh, float kMie, float shRlh,
float shMie, float g,out vec3 sunColor) {

    vec2 psi = raysi(r0, r, rAtmos);
    if (psi.x > psi.y) return vec3(0.0, 0.0, 0.0);
    psi.y = min(psi.y, raysi(r0, r, rPlanet).x);
    float iStepSize = (psi.y - psi.x ) / 8.0;
    float iTime = 0.0;
    vec3 totalRlh = vec3_splat(0.0);
    vec3 totalMie = vec3_splat(0.0);
    float iOdRlh = 0.0;
    float iOdMie = 0.0;
    float mu = dot(r, pSun);
    float mumu = mu * mu;
    float gg = g * g;
    float pRlh = clamp(3.0 / (12.0 * PI) * (1.0 + mumu), 0.0, 1.0);
    float pMie = clamp(3.0 / (22.0 * PI) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg)), 0.0, 1.0);
    vec3 iPos = r0 + r * (iTime + iStepSize * 0.5);
    float iHeight = length(iPos) - rPlanet;
    float odStepRlh = exp2(-iHeight / shRlh) * iStepSize;
    float odStepMie = pRlh*exp2(-iHeight / shMie) * iStepSize;
    iOdRlh += odStepRlh;
    iOdMie += odStepMie;
    float jStepSize = raysi(iPos, pSun, rAtmos).y / 5.0;
    float jTime = 0.0;
    float jOdRlh = 0.0;
    float jOdMie = 0.0;
    vec3 jPos = iPos + pSun * (jTime + jStepSize * 0.5);
    float jHeight = length(jPos) - rPlanet;
    jOdRlh += exp(-jHeight / shRlh) * jStepSize;
    jOdMie += exp(-jHeight / shMie) * jStepSize;
    jTime += jStepSize;
    vec3 attn = exp(-(kMie * (iOdMie + jOdMie) + kRlh * (iOdRlh + jOdRlh)));
    totalRlh += odStepRlh * attn;
    totalMie += odStepMie * attn;
    iTime += iStepSize;
    sunColor = attn;

     return iSun * (pRlh * kRlh * totalRlh + pMie * totalRlh*5e-6);
}

vec3 getSkyColor(vec3 pos, vec3 sunDir,out vec3 sunCol){
	vec3 ro = vec3(0,6372.0e3,0);
    float psCat = SKY_BRIGHTNESS; // brighness
    float rPl = 6372e3;
    float rAt = 6471e3;
    vec3 rlh = vec3(5.5e-6, 13e-6, 32.4e-6);
    vec3 rlp = vec3_splat(13e-6);
    float kMi = 37e-6;
    float rshl = 10e3;
    float rlm = 1.25e3;
    float attn = 0.58;
    
	vec3 color = atmosphere(pos, ro, sunDir, psCat, rPl, rAt, rlh, kMi, rshl, rlm, attn, sunCol);

    vec3 SKY_NIGHT_HORIZON = vec3(0.05, 0.08, 0.15);
    vec3 SKY_NIGHT_ZENITH  = vec3(0.01, 0.03, 0.08);

    float sunFactor = smoothstep(-0.1, 0.1, sunDir.y);
	color += mix(SKY_NIGHT_HORIZON,SKY_NIGHT_ZENITH, smoothstep(1.0,-0.5,pos.y));

return color;
}

#endif