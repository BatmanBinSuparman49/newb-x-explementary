#ifndef INSTANCING
  $input v_fogColor, v_worldPos, v_underwaterRainTimeDay, v_position
#endif

#include <bgfx_shader.sh>
#include <newb/functions/galaxy.h>
#include <yildrim/cloud.h>
#include <yildrim/atmosphere.h>
#include <yildrim/firmament.h>

#ifndef INSTANCING
  #include <newb/main.sh>
  uniform vec4 FogAndDistanceControl;
#endif

uniform vec4 FogColor;
uniform vec4 ViewPositionAndTime;
uniform vec4 SunDirection;
uniform vec4 TimeOfDay;

SAMPLER2D_AUTOREG(s_cirrusTex);

void main() {
  #ifndef INSTANCING
    vec3 viewDir = normalize(v_worldPos);

    vec4 whatTime = timeofday(TimeOfDay.x);
      float night = whatTime.x;
      float day   = whatTime.w;
      float dusk  = whatTime.z;
      float dawn  = whatTime.y;
      float twilight = saturate(dawn+dusk);

    float rain = mix(smoothstep(0.66, 0.3, FogAndDistanceControl.x), 0.0, step(FogAndDistanceControl.x, 0.0));

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

    vec3 sunDir = normalize(SunDirection.xyz);
    float sunA = clamp(((349.305545 * v_fogColor.g - 159.858192) * v_fogColor.g + 30.557216) * v_fogColor.g - 1.628452, -1.0, 1.0);
    vec3 moonPos =  normalize(vec3(cos(sunA), sin(sunA), 0.7));
    vec3 SunMoonDir = normalize(mix(sunDir, -moonPos, night));

    vec3 sunCol = vec3(1.2, 0.95, 0.72);
    vec3 dawnCol  = vec3(1.0, 0.35, 0.05); 
    sunCol = mix(sunCol, dawnCol, saturate(dawn+dusk));

    vec3 skyColor = nlRenderSky(skycol, env, -viewDir, v_fogColor, v_underwaterRainTimeDay.z);
    vec3 sky = getAtmosphere(s_cirrusTex, viewDir, sunDir, SunMoonDir, day, night, dusk, dawn, 1.0);
    
    vec3 moon = getMoon(-moonPos, viewDir, night);
    moon *= night;
    sky += moon;

    // vec2 uvC = viewDir.xz/viewDir.y;
    vec3 cloudCol = vec3(1.0, 0.8, 0.75)*day + vec3(0.8, 0.35, 0.05)*saturate(dawn+dusk) + vec3(0.5765, 0.584, 0.98)*night;
    // vec4 Cirrus = cirrus(uvC, cloudCol, SunMoonDir, viewDir);
    // sky    = mix(sky, Cirrus.rgb*1.5, Cirrus.a); 

    float jitter = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
    vec4 clouds3D = VLClouds(viewDir, SunMoonDir, cloudCol, ViewPositionAndTime.w, jitter);
    // sky = mix(sky, clouds3D.rgb, clouds3D.a);
    
    #ifdef NL_SHOOTING_STAR
      skyColor += NL_SHOOTING_STAR*nlRenderShootingStar(viewDir, v_fogColor, v_underwaterRainTimeDay.z)*max(0.75, night)*(1.0-rain);
    #endif

    #ifdef LYNX_AURORA
      if(!env.underwater){
        vec4 aurora = rdAurora(v_worldPos*0.001, viewDir, env, v_underwaterRainTimeDay.z, vec3(0.0,0.0,0.0), 0.0)*smoothstep(0.5, 1.0, night)*(1.0-rain);
        skyColor += mix(skyColor, aurora.rgb*1.5, aurora.a);
      }
    #endif
    
    #ifdef FALLING_STARS 
      if(!env.underwater){
        vec2 starUV = viewDir.xz / (0.5 + viewDir.y);
        float starValue = star(starUV * NL_FALLING_STARS_SCALE, NL_FALLING_STARS_VELOCITY, NL_FALLING_STARS_DENSITY, ViewPositionAndTime.w) * smoothstep(0.0, 0.5, viewDir.y);
        vec3 starColor = pow(vec3(starValue, starValue, starValue) * 1.1, vec3(16.0, 6.0, 4.0));
        float starFactor     = smoothstep(0.5, 1.0, night)*(1.0-rain);
        starColor     *= starFactor;
        skyColor += starColor;
      }
    #endif

    #ifdef NL_GALAXY_STARS
      skyColor += NL_GALAXY_STARS*nlGalaxy(viewDir, v_fogColor, env, v_underwaterRainTimeDay.z);
    #endif

    skyColor = colorCorrection(sky);
    gl_FragColor = vec4(sky, 1.0);
  #else
    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
  #endif
}
