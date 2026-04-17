#ifndef WATERS_H
#define WATERS_H

#define PI 3.141592

#include "newb/functions/sky.h"
#include "newb/config.h"
#include "newb/functions/galaxy.h"
#include "pbr.h"
#include "newb/functions/clouds.h"
#include "firmament.h"
#include "atmosphere.h"
#include "cloud.h"

#define NL_CLOUD_PARAMS(x) NL_CLOUD2##x##STEPS, NL_CLOUD2##x##THICKNESS, NL_CLOUD2##x##RAIN_THICKNESS, NL_CLOUD2##x##VELOCITY, NL_CLOUD2##x##SCALE, NL_CLOUD2##x##DENSITY, NL_CLOUD2##x##SHAPE


float booltofloat(bool factor){
return float(factor);
}
float fogtime(vec4 fogcol) {
    //三次多项式拟合，四次多项式拟合曲线存在明显突出故不使用
    // return fogcol.g > 0.213101 ? 1.0 : (((349.305545 * fogcol.g - 159.858192) * fogcol.g + 30.557216) * fogcol.g - 1.628452);
    return clamp(((349.305545 * fogcol.g - 159.858192) * fogcol.g + 30.557216) * fogcol.g - 1.628452, -1.0, 1.0);
}
mat2 rotMat(float a){
 return mat2(cos(a), sin(a), -sin(a), cos(a));
}
float randW(vec2 co)
{
 return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float noiseW(vec2 p)
{
    vec2 ip = floor(p);
    vec2 fp = fract(p);
    fp = fp * fp * (3.0 - 2.0 * fp);

    float res = mix(
    mix(randW(ip),randW(ip+vec2(1.0,0.0)),fp.x),
    mix(randW(ip+vec2(0.0,1.0)),randW(ip+vec2(1.0,1.0)),fp.x),fp.y);

    return res;
}

highp float getWave(highp vec2 uv, float time){
    float t = -time*1.0;

    uv *= 1.5;
    uv = mul(uv, rotMat(80.0));

    float A = sin(noiseW(t+uv-sin(uv.y*0.2)+uv.x)) * 0.5;
    float B = cos(noiseW(-t+uv+cos(uv.y*0.2)+uv.x)) * 0.5;
    float C = sin(noiseW(uv * 3.0 + t * 0.6)) * 0.5;
    return saturate(A + B);
}

float DistributionGGX(float NoH, float rough){
    float denom = (NoH * rough - NoH) * NoH + 1.0;
    return rough / (PI * pow(denom, 2.0));
}

// Another water wave function from code cave 
float ranD( vec2 p){
 return fract(cos(p.x +p.y * 13.0) * 335.552);
}

float snoise(vec2 p) {
   vec2 i = floor(p);
   vec2 f = fract(p);
   vec2 u = pow(f,vec2(2.0, 2.0))*(2. - 1.*f);

   return mix(mix(ranD(i+vec2(0.,0.)), ranD(i+vec2(1.,0.)), u.x), mix(ranD(i+vec2(0.,1.)), ranD(i+vec2(1.,1.)), u.x), u.y);
}

float Wave(vec2 p, float t){
  vec2 p1 = p + vec2(-t*0.9,t*0.2)*0.5;
  p = p - vec2(-t*0.9,t*0.2)*0.1;
  float w1 = snoise(vec2(p1.x*6.0,p1.y*1.5));
  float w2 = sin(snoise(vec2(p.x*3.0+p.y,p.y)));
  return clamp(1.5-(w1*(1.0-w2)+0.5),0.0,1.0);
}

// code cave water wave function ends 

float getWaterHeight(vec2 uv, float time) {
    return 0.03*getWave(uv,time); // your wave function or brightness of your texture (tex.r + tex.g + tex.b)/3.0
    // return 0.05*Wave(uv, time);
}

// another normal map function from code cave
vec3 getWaterNormal(vec2 uv, float t) {
    float eps = 0.005;
    float h  = getWaterHeight(uv, t);
    float hx = getWaterHeight(uv + vec2(eps, 0.0), t);
    float hy = getWaterHeight(uv + vec2(0.0, eps), t);

    float dx = (hx - h) / eps;
    float dy = (hy - h) / eps;

    return normalize(vec3(-dx, -dy, 1.0));
}

vec4 timedetection(vec4 FogColor,vec4 FogAndDistanceControl){
  float day1 = pow(max(min(1.0 - FogColor.r * 1.2, 1.0), 0.0), 0.4);
  float night = pow(max(min(1.0 - FogColor.r * 1.5, 1.0), 0.0), 1.2);
  float dusk1 = max(FogColor.r - FogColor.b, 0.0);
  float rain1 = mix(smoothstep(0.66, 0.3, FogAndDistanceControl.x), 0.0, step(FogAndDistanceControl.x, 0.0));
  
  return vec4(day1 ,night ,dusk1 ,rain1);
}

vec4 applyWaterEffect(
    vec3 v_pos, vec3 v_wpos, vec3 viewDir, vec3 V, vec3 L, vec3 texcol,
    vec4 diffuse, vec4 reflectionColor, 
    nl_skycolor skycol, nl_environment  env, vec3 FogColor,
    float time, float night, float dusk, float dawn, float rain, float nolight,
    bool isCave, bool water, float FogAndDistanceControl, float camDist, vec3 sunDir, vec3 N, float day, sampler2D NOISE_0
) {
    if (!water) return diffuse;

    float endDist = FogAndDistanceControl*0.8;
    bool doEffect = (camDist < endDist);

    vec3 Wnormal = getWaterNormal(v_pos.xz, time).xyz;
    vec3 normal = mul(Wnormal, getTBN(N));
    vec3 reflDir = reflect(viewDir, normal);

    float glossstrength = 0.5;

    vec3 F0 = mix(vec3(0.04, 0.04, 0.04), texcol.rgb, glossstrength);
    vec3 specular = brdf(L, V, 0.22, normal, diffuse.rgb, 0.0, F0, vec3(1.0, 1.0, 1.0));

    // Sun & Moon 
    float sunA = clamp(((349.305545 * FogColor.g - 159.858192) * FogColor.g + 30.557216) * FogColor.g - 1.628452, -1.0, 1.0);
    vec3 moonPos = vec3(cos(sunA), sin(sunA), 0.7);
    vec3 SunMoonDir = mix(normalize(sunDir), -moonPos, night);

    vec3 sun = sunS(normalize(sunDir), normalize(reflDir), dusk, dawn);
    sun *= (1.0-night);

    vec3 moon = getMoon(normalize(-moonPos), normalize(reflDir), night);
    moon *= night;

    // Clouds & Aurora
    vec2 cloudPos = 3.0 * reflDir.xz / max(reflDir.y, 0.05);
    vec3 roundPos;
    roundPos.xz = 48.0 * reflDir.xz/max(reflDir.y, 0.05);
    roundPos.y = 1.0;
    
    vec4 clouds = renderClouds(cloudPos, 0.1 * time, rain, skycol.horizonEdge, skycol.zenith,
                               NL_CLOUD3_SCALE, NL_CLOUD3_SPEED, NL_CLOUD3_SHADOW);

    vec4 v_color1 = vec4(skycol.zenith, rain);
    vec4 v_color2 = vec4(skycol.horizonEdge, time);
    vec4 roundedC = renderCloudsRounded(reflDir, roundPos, v_color1.w, v_color2.w, v_color2.rgb, v_color1.rgb, NL_CLOUD_PARAMS(_));

    vec2 uvC = reflDir.xz/reflDir.y;
    vec3 cirrusCol = vec3(1.0, 0.8, 0.75)*day + vec3(1.0, 0.35, 0.05)*saturate(dawn+dusk) + vec3(0.5765, 0.584, 0.98)*night;
    vec4 Cirrus = cirrus(uvC, cirrusCol, SunMoonDir, reflDir);

    vec4 aurora = rdAurora(reflect(v_wpos, normal) * 0.0001, reflDir, env, time, vec3(0.0,0.0,0.0), 0.0);

    // Stars
    vec3 stars = vec3(0.0, 0.0, 0.0);

    #ifdef FALLING_STARS
    if(!env.underwater) {
        vec2 starUV = reflDir.xz / (0.5 + reflDir.y);
        float starValue = star(starUV * NL_FALLING_STARS_SCALE, NL_FALLING_STARS_VELOCITY, NL_FALLING_STARS_DENSITY, time);
        float starFactor = smoothstep(0.5, 1.0, night)*(1.0-rain);
        stars = pow(vec3(starValue, starValue, starValue) * 1.1, vec3(16.0, 6.0, 4.0));
        stars *= starFactor;
    }
    #endif

    #ifdef NL_GALAXY_STARS
        vec3 GalaxyStars = nlGalaxy(reflDir, FogColor, env, time);
        stars += NL_GALAXY_STARS * GalaxyStars;
    #endif

    float NdotV = dot(normal, V);
    float fresnel = calculateFresnel(NdotV, 1.2);
    float blend = mix(0.04, 1.0, fresnel);

    vec3 skyReflection = getAtmosphere(NOISE_0, normalize(reflDir), normalize(sunDir), SunMoonDir, day, night, dusk, dawn, 0.0);
    diffuse.rgb = mix(diffuse.rgb, skyReflection, 1.0);

    reflectionColor.rgb = mix(vec3(0.02, 0.03, 0.04), reflectionColor.rgb, blend);
    vec3 reflections;
    #if NL_CLOUD_TYPE == 2
        reflections = mix(diffuse.rgb, roundedC.rgb * 0.4, 0.688 * roundedC.a * (1.0 - nolight));
    #else 
        //reflections = mix(diffuse.rgb, clouds.rgb * 0.4, 0.688 * clouds.a * (1.0 - nolight));
        reflections = mix(diffuse.rgb, Cirrus.rgb * mix(1.0, 0.6, night), 0.588 * Cirrus.a * (1.0 - nolight));

        // reflections = diffuse.rgb;
    #endif
    
    reflections += stars;

    float luma = dot(reflectionColor.rgb, vec3(0.299, 0.587, 0.114));
    float brightness = pow(clamp(luma * 1.8, 0.0, 1.0), mix(1.0, 2.5, 1.0 - FogColor.b));

    bool flatWater = v_wpos.y < 0.0;
    // bool flatWater = water;

    if (!env.end && flatWater) {
        diffuse.rgb = reflections * fresnel;
        diffuse.a = mix(diffuse.a * 0.75, 1.0, pow(1.0 - NdotV, 2.0));
        if(doEffect){
            #ifdef LYNX_AURORA
                    diffuse.rgb += aurora.rgb * aurora.a * smoothstep(0.5, 1.0, night) * (1.0-rain);
            #endif
        }
    }
    if(!env.end && !env.nether){
        diffuse.rgb += sun;
        diffuse.rgb += moon;
    }
    return diffuse;
}

#endif