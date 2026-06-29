#define PI 3.141592

#include "sky.h"
#include "newb/config.h"
#include "galaxy.h"
#include "PBR.h"
#include "clouds.h"
#include "water.h"

SAMPLER2D_AUTOREG(s_NoiseW);

#define NL_CLOUD_PARAMS(x) NL_CLOUD2##x##STEPS, NL_CLOUD2##x##THICKNESS, NL_CLOUD2##x##RAIN_THICKNESS, NL_CLOUD2##x##VELOCITY, NL_CLOUD2##x##SCALE, NL_CLOUD2##x##DENSITY, NL_CLOUD2##x##SHAPE

float fogtime(vec4 fogcol) {
    //三次多项式拟合，四次多项式拟合曲线存在明显突出故不使用
    // return fogcol.g > 0.213101 ? 1.0 : (((349.305545 * fogcol.g - 159.858192) * fogcol.g + 30.557216) * fogcol.g - 1.628452);
    return clamp(((349.305545 * fogcol.g - 159.858192) * fogcol.g + 30.557216) * fogcol.g - 1.628452, -1.0, 1.0);
}

// 2D Rotation Matrix function
mat2 rotMat(float a){
 return mat2(cos(a), sin(a), -sin(a), cos(a));
}


// Water Noise & Wave Starts from this Random Function
float randW(vec2 co)
{
 return fract(sin(dot(co ,vec2(12.9898,78.233))) * 43758.5453);
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

    uv *= 1.5;
    uv = mul(uv, mat2(0.173648, 0.984808, -0.984808, 0.173648));

    // float scale = 1.0 / 16.0;

    return saturate(noise(t+uv-(uv.y*0.4)+uv.x)*0.5) * 0.03;
    // float B = (texture2D(s_NoiseW, (-t+uv+(uv.y*0.3)+uv.x) * scale).r) * 0.5;
    // return saturate(A);// + B);
}

float getWaterHeight(vec2 uv, float time) {
    return 0.03*getWave(uv,time); }

// Water Noise & Wave Ends

vec3 getWaterNormal(vec2 uv, float t) {
    float eps = 0.005;
    float h  = getWave(uv, t);
    float hx = getWave(uv + vec2(eps, 0.0), t);
    float hy = getWave(uv + vec2(0.0, eps), t);

    float dx = (hx - h) / eps;
    float dy = (hy - h) / eps;

    return normalize(vec3(-dx, -dy, 1.0));
}

vec4 timedetection(vec4 FogColor,vec4 FogAndDistanceControl){
  float day1 = pow(max(min(1.0 - FogColor.r * 1.2, 1.0), 0.0), 0.4);
  float night1 = pow(max(min(1.0 - FogColor.r * 1.5, 1.0), 0.0), 1.2);
  float dusk1 = max(FogColor.r - FogColor.b, 0.0);
  float rain1 = mix(smoothstep(0.66, 0.3, FogAndDistanceControl.x), 0.0, step(FogAndDistanceControl.x, 0.0));
  
  return vec4(day1 ,night1 ,dusk1 ,rain1);
}

vec4 applyWaterEffect(
    sampler2D cloudTex, vec3 v_pos, vec3 v_wpos, vec3 viewDir, vec3 V, vec4 diffuse,
    nl_skycolor skycol, nl_environment  env, vec3 FogColor,
    float time, float night, float dusk, float dawn, float rain,
    bool water, float lm, vec3 sunDir, vec3 N
) {
    if (!water) return diffuse;

    vec3 wnormal = getWaterNormal(v_pos.xz, time).xyz;
    mat3 TBN = getTBN(N);
    vec3 normal = mul(wnormal, TBN);

    vec3 reflDir = reflect(viewDir, normal);
    
    vec4 aurora = rdAurora(reflect(v_wpos, normal) * 0.0001, reflDir, env, time, vec3(0.0,0.0,0.0), 0.0);

    vec3 sun = sunS(sunDir, reflDir, dusk, dawn);
    // sun *= exp(min(reflDir.y, 0.0) * 100.0);
    sun *= (1.0-night);

    vec3 moonDir = normalize(vec3(-0.6, 0.45, -0.7))*smoothstep(0.0, 0.7, night);
    vec3 moon = getMoon(moonDir, reflDir, night);

    vec3 stars = vec3(0.0, 0.0, 0.0);

    vec2 starUV = reflDir.xz / (0.5 + reflDir.y);
    float starValue = star(starUV * NL_FALLING_STARS_SCALE, NL_FALLING_STARS_VELOCITY, NL_FALLING_STARS_DENSITY, time);
    float starFactor = smoothstep(0.67, 1.0, night)*(1.0-rain);
    stars = pow(vec3(starValue, starValue, starValue) * 1.1, vec3(16.0, 6.0, 4.0));
    stars *= starFactor;

    vec3 GalaxyStars = nlGalaxy(reflDir, FogColor, env, time);
    stars += NL_GALAXY_STARS * GalaxyStars;

    vec3 skyReflection = getSkyRefl(skycol, env, reflDir, FogColor.rgb, time);
    diffuse.rgb = mix(diffuse.rgb, skyReflection, 1.0);

    vec3 reflections;
    vec3 roundPos;
    #if NL_CLOUD_TYPE == 2
        roundPos.xz = 56.0 * reflDir.xz/max(reflDir.y, 0.05);
        roundPos.y = 1.0;
        vec4 roundedC = renderCloudsRounded(cloudTex, reflDir, roundPos, rain, time, skycol.horizonEdge, skycol.zenith, NL_CLOUD_PARAMS(_));
        reflections = mix(diffuse.rgb, roundedC.rgb * 0.4, 0.688 * roundedC.a);
    #else 
        vec2 cloudPos = 3.0 * reflDir.xz / max(reflDir.y, 0.05);
        vec4 clouds = renderClouds(cloudPos, 0.1 * time, rain, skycol.horizonEdge, skycol.zenith,NL_CLOUD3_SCALE, NL_CLOUD3_SPEED, NL_CLOUD3_SHADOW);
        reflections = mix(diffuse.rgb, clouds.rgb * 0.4, 0.688 * clouds.a);
    #endif
    
    reflections += stars;

    // bool flatWater = v_pos.y < 0.0;

    float NdotV = clamp(dot(normal, V), 0.0, 1.0);
    float fresnel = ( 0.2 + (1.0-0.2) * pow(1.0-NdotV, 5.0) ) * lm;

    if (!env.end){ 
        diffuse.rgb = mix(diffuse.rgb, reflections, fresnel);
        diffuse.rgb += aurora.rgb * aurora.a * smoothstep(0.9, 1.0, night) * (1.0-rain);
    }

    #ifdef NL_FULLBRIGHT
        diffuse.a = mix(diffuse.a*1.2, 1.0, saturate(fresnel));
    #endif
    
    diffuse.a = mix(diffuse.a*0.8, 1.0, saturate(fresnel));

    if(!env.end && !env.nether){
        diffuse.rgb += sun;
        diffuse.rgb += moon;
    }
    return diffuse;
}