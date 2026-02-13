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

vec3 filmicBSBE(vec3 x){
    return (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
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

    vec3 sun = getSun(sunDir, viewDir, night1, dusk, dawn);
    sun *= (1.0-night1);
    vec3 moon = getMoon(mix(sunDir, normalize(vec3(-0.6, 0.45, -0.7)), night1 * (1.0 - dawn) * (1.0 - dusk)), viewDir, night1);
    moon *= night1;
    skyColor += sun;

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
