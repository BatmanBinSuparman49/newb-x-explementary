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
    uv = mul(uv, rotMat(0.05));

    float A = sin(noiseW(t+uv-sin(uv.y*0.2)+uv.x)) * 0.5;
    float B = cos(noiseW(-t+uv+cos(uv.y*0.2)+uv.x)) * 0.5;
    float C = sin(noiseW(uv * 3.0 + t * 0.6)) * 0.5;
    return saturate(A + B);
}

float DistributionGGX(float NoH, float rough){
    float denom = (NoH * rough - NoH) * NoH + 1.0;
    return rough / (PI * pow(denom, 2.0));
}

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


vec3 getSunRefl(vec3 viewDir, vec3 sunDir, float day, float night, float dusk, float dawn, float rain, float nolight) {
    vec3 refl = vec3(0.0, 0.0, 0.0);

    float a = saturate(dawn+dusk+day);

    vec3 sun = sunS(normalize(sunDir), normalize(viewDir), dusk, dawn)*a;
    sun = mix(sun, cSatur(sun, 0.5) * 0.5, rain*rain);

    vec3 mie = getMie(normalize(viewDir), normalize(sunDir))*a;
    float sunsetFactor = saturate(sunDir.y * 2.5); 
    float transition = pow(sunsetFactor, 2.0); 
    vec3 sunRed = vec3(4.0, 0.3, 0.02);
    vec3 sunDay = vec3(1.0, 0.875, 0.688); 
    vec3 currentSunCol = mix(sunRed, sunDay, transition);
    mie *= currentSunCol * 0.5;
    mie = max(pow(mie, 0.55), 0.0) * 0.5;
    mie = mix(mie, cSatur(mie, 0.5) * 0.5, rain*rain);
    vec3 finalSun = sun + mie;
    finalSun *= nolight;

    refl += finalSun;
    return refl;
}

vec4 applyWaterEffect(
    vec3 v_pos, vec3 v_wpos, vec3 viewDir, vec3 V, vec3 L, vec3 texcol,
    vec4 diffuse, vec4 reflectionColor, 
    nl_skycolor skycol, nl_environment  env, vec3 FogColor,
    float time, float night, float dusk, float dawn, float rain, float nolight,
    bool isCave, bool water, float FogAndDistanceControl, float camDist, vec3 sunDir, vec3 N, float day, sampler2D cirrusTex
) {
    if (!water) return diffuse;

    float endDist = FogAndDistanceControl*0.8;
    bool doEffect = (camDist < endDist);

    vec3 Wnormal = getWaterNormal(v_pos.xz, time).xyz;
    vec3 normal = mul(Wnormal, getTBN(N));
    vec3 reflDir = reflect(viewDir, normal);

    // Sun & Moon 
    float sunA = clamp(((349.305545 * FogColor.g - 159.858192) * FogColor.g + 30.557216) * FogColor.g - 1.628452, -1.0, 1.0);
    vec3 moonPos = vec3(cos(sunA), sin(sunA), 0.7);
    vec3 SunMoonDir = normalize(mix(sunDir, -moonPos, smoothstep(0.0, 0.67, night)));   
    float a = saturate(dawn+dusk+day);
    
    vec3 sun = sunS(normalize(sunDir), normalize(reflDir), dusk, dawn)*a;
    sun = mix(sun, cSatur(sun, 0.5) * 0.5, rain*rain);

    vec3 mie = getMie(normalize(reflDir), normalize(sunDir))*a;
    float sunsetFactor = saturate(sunDir.y * 2.5); 
    float transition = pow(sunsetFactor, 2.0); 
    vec3 sunRed = vec3(4.0, 0.3, 0.02);
    vec3 sunDay = vec3(1.0, 0.875, 0.688); 
    vec3 currentSunCol = mix(sunRed, sunDay, transition);
    mie *= currentSunCol * 0.5;
    mie = max(pow(mie, 0.55), 0.0) * 0.5;
    mie = mix(mie, cSatur(mie, 0.5) * 0.5, rain*rain);
    sun += mie;

    vec3 moon = getMoon(normalize(-moonPos), normalize(reflDir), night);
    moon *= night;

    vec2 uvC = reflDir.xz/reflDir.y;
    vec3 cirrusCol = vec3(1.0, 0.8, 0.75)*day + vec3(1.0, 0.35, 0.05)*saturate(dawn+dusk) + vec3(0.5765, 0.584, 0.98)*night;
    vec4 Cirrus = cirrus(cirrusTex, uvC, cirrusCol, SunMoonDir, reflDir);

    float NdotV = dot(normal, V);
    float fresnel = calculateFresnel(NdotV, 1.2);

    vec3 skyReflection = getAtmosphere(cirrusTex, normalize(reflDir), normalize(sunDir), SunMoonDir, day, night, dusk, dawn, rain, 0.0);
    skyReflection = cSatur(skyReflection, 1.2);
    skyReflection = mix(skyReflection, day_zenith(sunDir, rain), mix(0.0, 0.6, day));
    skyReflection = mix(skyReflection, night_zenith(rain), mix(0.0, 0.7, night));
    diffuse.rgb = mix(diffuse.rgb, skyReflection, 1.0);

    vec3 reflections;
    reflections = mix(diffuse.rgb, Cirrus.rgb, 0.788 * Cirrus.a);

    bool flatWater = v_wpos.y < 0.0;

    if (!env.end && flatWater) {
        diffuse.rgb += reflections * fresnel;
        diffuse.a = mix(diffuse.a * 0.8, 1.0, pow(1.0 - NdotV, 2.0));
    }
    if(!env.end && !env.nether){
        diffuse.rgb += sun; //*(nolight);
        diffuse.rgb += moon*(nolight);
    }
    return diffuse;
}

float fSchlick(float f0, float nd){
    return f0 + (1.0 - f0) * pow(1.0 - nd, 5.0);
}

vec4 waterfunction(
    sampler2D cirrusTex, vec3 viewDir, vec3 sunDir, vec3 SunMoonDir, vec3 FogColor, vec4 FogAndDistanceControl, float day, 
    float night, float dusk, float dawn, float rain, float nolight,
    vec4 diffuse, vec3 N, vec3 v_pos, vec3 v_wpos, vec2 lightmapUV, nl_environment env, bool water, float time
){
    if(!water) return diffuse;
    vec3 wN = getWaterNormal(v_pos.xz, time).xyz;
    vec3 normal = normalize(mul(normalize(wN), getTBN(N)));


    vec3 V = normalize(reflect(viewDir, normal));

    vec3 skyRefl = getAtmosphereVertex(env, viewDir, sunDir, day, night, rain);

    diffuse.rgb = mix(diffuse.rgb, skyRefl, 1.0);

    vec3 sun = getSunRefl(V, sunDir, day, night, dusk, dawn, rain, nolight);
    float sunA = clamp(((349.305545 * FogColor.g - 159.858192) * FogColor.g + 30.557216) * FogColor.g - 1.628452, -1.0, 1.0);
    vec3 moonPos = vec3(cos(sunA), sin(sunA), 0.7);
    vec3 moon = getMoon(normalize(-moonPos), V, night)*night;

    vec2 uvC = V.xz/V.y;
    vec3 cirrusCol = vec3(1.0, 0.8, 0.75)*day + vec3(1.0, 0.35, 0.05)*saturate(dawn+dusk) + vec3(0.5765, 0.584, 0.98)*night;
    vec4 Cirrus = cirrus(cirrusTex, uvC, cirrusCol, SunMoonDir, V);

    vec3 refl = skyRefl;
         refl = mix(refl, Cirrus.rgb, Cirrus.a * (nolight));

    vec3 vDir = normalize(-viewDir);
    float fresnel = fSchlick(0.2, normalize(dot(normal, vDir))) * lightmapUV.y;

    bool flatWater = v_wpos.y < 0.0;
    
    if(flatWater && !env.end ){
        diffuse.rgb += refl * fresnel; 
        diffuse.a = mix(diffuse.a * 0.8, 1.0, 1.0 - pow(normalize(dot(normal, vDir)), 2.0) );
    }

    if(!env.end){
        diffuse.rgb += sun;
        diffuse.rgb += moon*(nolight);
    }
    return diffuse;
}

#endif