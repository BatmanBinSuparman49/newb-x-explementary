#define PI 3.141592

#include "sky.h"
#include "newb/config.h"
#include "galaxy.h"
#include "PBR.h"
#include "clouds.h"

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

float noise(vec2 p)
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

    float tiles = 16.0;

    // uv *= tiles;
    uv *= 2.1;
    //uv *= rotMat(80.0);

    float A = sin(noise(t+mod(uv,tiles)-sin(mod(uv,tiles).y*0.2)+mod(uv,tiles).x)) * 0.5;
    float B = cos(noise(-t+mod(uv,tiles)+cos(mod(uv,tiles).y*0.2)+mod(uv,tiles).x)) * 0.5;
    //float A = sin(voronoi2D(t+uv-sin(uv.y*0.2)+uv.x),time,16.0)) * 0.5;
    return saturate(A + B);
}

float DistributionGGX(float NoH, float rough){
    float denom = (NoH * rough - NoH) * NoH + 1.0;
    return rough / (PI * pow(denom, 2.0));
}

float getWaterHeight(vec2 uv, float time) {
    return 0.05*getWave(uv,time); // your wave function or brightness of your texture (tex.r + tex.g + tex.b)/3.0
}
vec4 getWaterNormalMapFromHeight(vec2 uv, vec2 resolution, float scale, float time) {
  vec2 step = 1.0 / resolution;

  float height = getWaterHeight(uv,time);

  vec2 dxy = height - vec2(
      getWaterHeight(uv + vec2(step.x, 0.0), time),
      getWaterHeight(uv + vec2(0.0, step.y), time)
  );
  return vec4(normalize(vec3(dxy * scale / step, 1.0)), height);
}

vec4 timedetection(vec4 FogColor,vec4 FogAndDistanceControl){
  float day1 = pow(max(min(1.0 - FogColor.r * 1.2, 1.0), 0.0), 0.4);
  float night1 = pow(max(min(1.0 - FogColor.r * 1.5, 1.0), 0.0), 1.2);
  float dusk1 = max(FogColor.r - FogColor.b, 0.0);
  float rain1 = mix(smoothstep(0.66, 0.3, FogAndDistanceControl.x), 0.0, step(FogAndDistanceControl.x, 0.0));
  
  return vec4(day1 ,night1 ,dusk1 ,rain1);
}

vec4 applyWaterEffect(
    vec3 v_pos, vec3 v_wpos, vec3 viewDir, vec3 V, vec3 L, vec3 texcol,
    vec4 diffuse, vec4 reflectionColor, 
    nl_skycolor skycol, nl_environment  env, vec3 FogColor,
    float time, float night, float dusk, float dawn, float rain, float nolight,
    bool isCave, bool water, float FogAndDistanceControl, float camDist, vec3 sunDir
) {
    if (!water) return diffuse;

    float endDist = FogAndDistanceControl*0.8;
    bool doEffect = (camDist < endDist);

    vec3 normal = getWaterNormalMapFromHeight(v_pos.xz, vec2(12.0, 12.0), 0.5, 0.5 * time).xzy;
    vec3 reflDir = reflect(viewDir, normal);

    float glossstrength = 0.5;

    vec3 F0 = mix(vec3(0.04, 0.04, 0.04), texcol.rgb, glossstrength);
    vec3 specular = brdf(L, V, 0.22, normal, diffuse.rgb, 0.0, F0, vec3(1.0, 1.0, 1.0));

    vec2 cloudPos = 3.0 * reflDir.xz / max(reflDir.y, 0.05);
    vec3 roundPos;
    roundPos.xz = 48.0 * reflDir.xz/max(reflDir.y, 0.05);
    roundPos.y = 1.0;
    
    vec4 aurora = rdAurora(reflect(v_wpos, normal) * 0.0001, viewDir, env, time, vec3(0.0,0.0,0.0), 0.0);
    vec4 clouds = renderClouds(cloudPos, 0.1 * time, rain, skycol.horizonEdge, skycol.zenith,
                               NL_CLOUD3_SCALE, NL_CLOUD3_SPEED, NL_CLOUD3_SHADOW);

    vec4 v_color1 = vec4(skycol.zenith, rain);
    vec4 v_color2 = vec4(skycol.horizonEdge, time);
    vec4 roundedC = renderCloudsRounded(viewDir, roundPos, v_color1.w, v_color2.w, v_color2.rgb, v_color1.rgb, NL_CLOUD_PARAMS(_));

    vec3 sun = getSun(sunDir, viewDir, night, dusk, dawn);
    sun *= (1.0-night);
    vec3 moon = getMoon(mix(sunDir, normalize(vec3(-0.6, 0.45, -0.7)), night * (1.0 - dawn) * (1.0 - dusk)), viewDir, night);

    vec3 stars = vec3(0.0, 0.0, 0.0);

    #ifdef FALLING_STARS
    if(!env.underwater) {
        vec2 starUV = viewDir.xz / (0.5 + viewDir.y);
        float starValue = star(starUV * NL_FALLING_STARS_SCALE, NL_FALLING_STARS_VELOCITY, NL_FALLING_STARS_DENSITY, time);
        float starFactor = smoothstep(0.5, 1.0, night)*(1.0-rain);
        stars = pow(vec3(starValue, starValue, starValue) * 1.1, vec3(16.0, 6.0, 4.0));
        stars *= starFactor;
    }
    #endif

    #ifdef NL_GALAXY_STARS
        vec3 GalaxyStars = nlGalaxy(viewDir, FogColor, env, time);
        stars += NL_GALAXY_STARS * GalaxyStars;
    #endif

    float NdotV = dot(normal, V);
    float fresnel = calculateFresnel(NdotV, 1.2);
    float blend = mix(0.04, 1.0, fresnel);

    vec3 skyReflection = getSkyRefl(skycol, env, viewDir, FogColor.rgb, time);
    diffuse.rgb = mix(diffuse.rgb, skyReflection, 1.0);


    reflectionColor.rgb = mix(vec3(0.02, 0.03, 0.04), reflectionColor.rgb, blend);
    vec3 reflections;
    #if NL_CLOUD_TYPE == 2
        reflections = mix(diffuse.rgb, roundedC.rgb * 0.4, 0.688 * roundedC.a * (1.0 - nolight));
    #else 
        reflections = mix(diffuse.rgb, clouds.rgb * 0.4, 0.688 * clouds.a * (1.0 - nolight));
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