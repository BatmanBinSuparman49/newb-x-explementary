#ifndef INSTANCING
  $input v_fogColor, v_worldPos, v_underwaterRainTimeDay, v_position
#endif

#include <bgfx_shader.sh>
#include <newb/config.h>
#include <newb/functions/galaxy.h>

#ifndef INSTANCING
  #include <newb/main.sh>
  uniform vec4 FogAndDistanceControl;
#endif

uniform vec4 FogColor;
uniform vec4 ViewPositionAndTime;
uniform vec4 SunDirection;
uniform vec4 TimeOfDay;

float hash12(vec2 p) {
    const vec3 K = vec3(0.3183099, 0.3678794, 43758.5453);
    vec3 x = fract(vec3(p.xyx) * K.x + K.y);
    x += dot(x, x.yzx + 19.19);
    return fract((x.x + x.y) * x.z);
}

float fastVoronoi3D(vec3 pos, float f) {
    vec3 p1 = fract(pos) - 0.5;
    vec3 p2 = fract(pos.zxy + vec3(0.12, 0.34, 0.56)) - 0.5;
    
    float d1 = dot(p1, p1);
    float d2 = dot(p2, p2);
    
    return 1.0 - f * min(d1, d2);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);

    float a = hash12(i + vec2(0.0, 0.0));
    float b = hash12(i + vec2(1.0, 0.0));
    float c = hash12(i + vec2(0.0, 1.0));
    float d = hash12(i + vec2(1.0, 1.0));

    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < 5; i++) {
        value += amplitude * noise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

float cloudsNoise3D(vec3 p, float t) {
    float n = fastVoronoi3D(p + t * 0.1, 1.8);
    n *= fastVoronoi3D(3.0 * p + t * 0.2, 1.5);
    n *= fastVoronoi3D(9.0 * p + t * 0.4, 0.4);
    n *= fastVoronoi3D(27.0 * p + t * 0.8, 0.1);
    
    return n * n;
}

float hash13(vec3 p3) {
    p3 = fract(p3 * 0.1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return fract((p3.x + p3.y) * p3.z);
}

float newnoise(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    vec3 u = f * f * (3.0 - 2.0 * f);

    float n000 = hash13(i + vec3(0.0, 0.0, 0.0));
    float n100 = hash13(i + vec3(1.0, 0.0, 0.0));
    float n010 = hash13(i + vec3(0.0, 1.0, 0.0));
    float n110 = hash13(i + vec3(1.0, 1.0, 0.0));
    float n001 = hash13(i + vec3(0.0, 0.0, 1.0));
    float n101 = hash13(i + vec3(1.0, 0.0, 1.0));
    float n011 = hash13(i + vec3(0.0, 1.0, 1.0));
    float n111 = hash13(i + vec3(1.0, 1.0, 1.0));

    float nx00 = mix(n000, n100, u.x);
    float nx10 = mix(n010, n110, u.x);
    float nx01 = mix(n001, n101, u.x);
    float nx11 = mix(n011, n111, u.x);

    float nxy0 = mix(nx00, nx10, u.y);
    float nxy1 = mix(nx01, nx11, u.y);

    return mix(nxy0, nxy1, u.z);
}

#define pi radians(180.0)
#define hpi (pi / 2.0)
#define tau (pi * 2.0)

float fbmVL(vec3 position, int k) {
    float result = 0.0;
    float density = 0.5;
    for (int i = 0; i < 5; i++) {
        result += density * newnoise(position);
        position *= 3.0;
        density *= 0.5;
    }
    return clamp(result, 0.0, 1.0) + 1.0 / tau / float(k);
}

#define CLOUD_MIN_HEIGHT 0.3
#define CLOUD_MAX_HEIGHT 4.0
#define CLOUD_THICKNESS (CLOUD_MAX_HEIGHT - CLOUD_MIN_HEIGHT)
#define MARCH_SIZE 0.15
#define MAX_STEPS 32
#define DENSITY_THRESHOLD 0.01

float remap(float value, float oldMin, float oldMax, float newMin, float newMax) {
    return newMin + (value - oldMin) * (newMax - newMin) / (oldMax - oldMin);
}
float getCloudDensity(vec3 p, float distance) {
    float heightGradient = (p.y - CLOUD_MIN_HEIGHT) / CLOUD_THICKNESS;
    float heightFalloff = smoothstep(0.0, 0.2, heightGradient) * smoothstep(1.0, 0.4, heightGradient);
    int octaves = distance > 8.0 ? 2 : 4;
    float density = fbmVL(p * 0.8, octaves);
    density = remap(density,0.4,0.8,0.0,1.0);
    density = smoothstep(0.5, 0.9, density * heightFalloff);
    return density * 2.0;
}

vec3 getCloudColor(vec3 p, float density, vec3 lightDir, float distance, vec3 rd, vec3 skyColor, vec3 sunColor) {
    float shadowSample = getCloudDensity(p + lightDir * 0.2, distance);
    float beer = exp(-shadowSample * 3.0);
    float powder = 1.0 - exp(-density * 2.0);
    float lightEnergy = beer * powder;
    
    float cosTheta = dot(rd, lightDir);
    float g = 0.6;
    float hg = ((1.0 - g*g) / pow(1.0 + g*g - 2.0*g*cosTheta, 1.5)) / (4.0 * 3.14159);
    
    vec3 cloudBase = mix(skyColor * 0.45, sunColor, lightEnergy);
    vec3 finalColor = cloudBase + (sunColor * hg * 0.4 * beer);
    return finalColor;
}

vec4 cumulusCloud(vec3 viewDir, float time, float jitter, vec3 sunColor, vec3 skyColor, float rain, int stepsValue) {
    float cloudBase = 1.0;
    float cloudTop = 2.0;
    const int steps = 10;
    float stepSize = (cloudTop - cloudBase) / float(steps);
    vec3 rayOrigin = vec3_splat(0.0);
    vec3 cloudAccum = vec3_splat(0.0);
    float alphaAccum = 0.0;
    float viewLift = smoothstep(0.01, 0.1, viewDir.y);
    if (viewDir.y < 0.0) return vec4_splat(0.0);

    for (int i = 0; i < steps; i++) {
        float height = cloudBase + stepSize * (float(i) + jitter);
        float t = height / max(viewDir.y, 0.001);
        vec3 pos = rayOrigin + viewDir * t;
        vec3 noisePos = vec3(pos.xz * 0.1 + time * 0.002, height * 0.1);
        vec3 p_base = noisePos;
        p_base.y *= 1.5;
        float tower = smoothstep(0.45, 0.5, cloudsNoise3D(p_base, 0.3));

        vec3 p_top = noisePos;
        p_top.xz /= 2.5;
        p_top.y -= 1.5;
        float anvil_top = smoothstep(0.4, 0.5, cloudsNoise3D(p_top, 0.3));
        anvil_top *= smoothstep(1.0, 1.5, noisePos.y) * smoothstep(2.5, 2.0, noisePos.y);

        float cloud = max(tower, anvil_top);
        float heightNorm = (height - cloudBase) / (cloudTop - cloudBase);

        float density = smoothstep(0.9, 0.3, cloud);
        float alpha = (1.0 - density) * (1.0 - alphaAccum) * viewLift;

        float scattering = smoothstep(0.0, 0.8, heightNorm);
        scattering = mix(scattering, 1.0, 0.3);
        scattering = pow(scattering, 1.5);
        scattering = mix(0.1, 5.0, scattering);
        
        vec3 cloudColor = sunColor * scattering;

        cloudAccum += cloudColor * (alpha * 0.8);
        alphaAccum += alpha;

        if (alphaAccum > 0.98 && viewDir.y < 0.9) break;
    }

#if !defined(TERRAIN)
    alphaAccum *= smoothstep(0.0, 0.5, viewDir.y);
#endif

    return vec4(cloudAccum, alphaAccum);
}

vec4 cirrus(vec2 uv, vec3 sunColor, vec3 sunDir, vec3 viewDir) {
    vec2 p = uv * vec2(12.0, 3.0);
    float flow1 = fbm(p * 0.3) * 2.0;
    float flow2 = fbm(p * 0.15 + vec2_splat(100.0)) * 1.2;
    vec2 distorted = p + vec2(flow1 * 2.5, flow1 * 0.4) + vec2(flow2 * 0.8, flow2 * 0.2);
    float clouds = fbm(distorted * 0.18);
    clouds += fbm(distorted * 0.4 + vec2_splat(50.0)) * 0.5;
    clouds += fbm(distorted * 1.2 + vec2_splat(200.0)) * 0.25;
    clouds /= 1.75;
    clouds = smoothstep(0.35, 0.75, clouds);
    float gaps = fbm(p * 0.25 + vec2_splat(300.0));
    clouds *= smoothstep(0.3, 0.6, gaps);
    float scattering = dot(sunDir, viewDir) * 0.5 + 0.5;
    float forwardScatter = pow(max(scattering, 0.0), 2.0);
    float backScatter = pow(max(1.0 - scattering, 0.0), 3.0) * 0.3;
    float totalScatter = 0.6 + forwardScatter * 0.8 + backScatter;
    vec3 cloudColor = sunColor * totalScatter;
    cloudColor += vec3(0.1, 0.15, 0.2) * (1.0 - totalScatter) * 0.3;
    clouds *= 0.85 * smoothstep(0.05, 1.0, viewDir.y);
    return vec4(cloudColor, clouds);
}

void main() {
  #ifndef INSTANCING
    vec3 viewDir = normalize(v_worldPos);

    vec4 whatTime = timeofday(TimeOfDay.x);
      float night1 = whatTime.x;
      float day   = whatTime.w;
      float dusk  = whatTime.z;
      float dawn  = whatTime.y;

    float rain = mix(smoothstep(0.66, 0.3, FogAndDistanceControl.x), 0.0, step(FogAndDistanceControl.x, 0.0));

    vec3 sunDir = normalize(SunDirection.xyz);
    vec3 moonDir = mix(sunDir, normalize(vec3(-0.6, 0.45, -0.7)), night1 * (1.0 - dawn) * (1.0 - dusk));
    float moonFactor = night1 * (1.0 - dawn) * (1.0 - dusk);

    nl_environment env;
    env.end = false;
    env.nether = false;
    env.underwater = v_underwaterRainTimeDay.x > 0.5;
    env.rainFactor = v_underwaterRainTimeDay.y;

    nl_skycolor skycol;
    if (env.underwater) {
      skycol = nlUnderwaterSkyColors(env.rainFactor, v_fogColor.rgb);
    } else {
      skycol = nlOverworldSkyColors(env.rainFactor, v_fogColor.rgb);
    }

    vec3 skyColor = nlRenderSky(skycol, env, -viewDir, v_fogColor, v_underwaterRainTimeDay.z);

    vec2 uvC = viewDir.xz/viewDir.y;
    vec4 clouds = cirrus(uvC, vec3(1.0, 0.8, 0.7), normalize(mix(sunDir, moonDir, moonFactor)), viewDir);
    // skyColor    = mix(skyColor, clouds.rgb, clouds.a);

    vec3 sun = getSun(sunDir, viewDir, night1, dusk, dawn);
    sun *= (1.0-night1);
    vec3 moon = getMoon(moonDir, viewDir, night1);
    moon *= night1;
    skyColor += sun;
    // vec4 clouds = cumulusCloud(viewDir, ViewPositionAndTime.w, 0.5, vec3(1.0, 0.8, 0.7), skyColor, rain, 10);

    #ifdef NL_SHOOTING_STAR
      skyColor += NL_SHOOTING_STAR*nlRenderShootingStar(viewDir, v_fogColor, v_underwaterRainTimeDay.z)*max(0.75, night1)*(1.0-rain);
    #endif

    #ifdef LYNX_AURORA
      if(!env.underwater){
        vec4 aurora = rdAurora(v_worldPos*0.001, viewDir, env, v_underwaterRainTimeDay.z, vec3(0.0,0.0,0.0), 0.0)*smoothstep(0.5, 1.0, night1)*(1.0-rain);
        skyColor += mix(skyColor, aurora.rgb*1.5, aurora.a);
      }
    #endif
    
    #ifdef FALLING_STARS 
      if(!env.underwater){
        vec2 starUV = viewDir.xz / (0.5 + viewDir.y);
        float starValue = star(starUV * NL_FALLING_STARS_SCALE, NL_FALLING_STARS_VELOCITY, NL_FALLING_STARS_DENSITY, ViewPositionAndTime.w);
        vec3 starColor = pow(vec3(starValue, starValue, starValue) * 1.1, vec3(16.0, 6.0, 4.0));
        float starFactor     = smoothstep(0.5, 1.0, night1)*(1.0-rain);
        starColor     *= starFactor;
        skyColor += starColor;
      }
    #endif

    #ifdef NL_GALAXY_STARS
      skyColor += NL_GALAXY_STARS*nlGalaxy(viewDir, v_fogColor, env, v_underwaterRainTimeDay.z);
    #endif

    skyColor = colorCorrection(skyColor);

    skyColor += moon;

    gl_FragColor = vec4(skyColor, 1.0);
  #else
    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
  #endif
}
