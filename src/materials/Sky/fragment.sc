#ifndef INSTANCING
  $input v_fogColor, v_worldPos, v_underwaterRainTimeDay, v_position
#endif

#include <bgfx_shader.sh>
#include <newb/config.h>
#include <newb/functions/galaxy.h>
#include <newb/functions/PBR.h>

#ifndef INSTANCING
  #include <newb/main.sh>
  uniform vec4 FogAndDistanceControl;
#endif

uniform vec4 FogColor;
uniform vec4 ViewPositionAndTime;
uniform vec4 SunDirection;
uniform vec4 TimeOfDay;

float randC( vec2 p){
 return fract(cos(p.x +p.y * 13.0) * 335.552);
}

float snoise(  vec2 p) {
   vec2 i = floor(p);
   vec2 f = fract(p);
   vec2 u = pow(f, vec2(2.0, 2.0))*(2. - 1.0*f);

   return mix(mix(randC(i+vec2(0.,0.)), randC(i+vec2(1.,0.)), u.x), mix(randC(i+vec2(0.,1.)), randC(i+vec2(1.,1.)), u.x), u.y);
}

float fbm(vec2 p,float x){
 float t = ViewPositionAndTime.w;
 float a = 0.;
 float b = 1.;
 p += t*vec2(0.09,0.02);
for(int i=0; i<4; i++){
 a += snoise(p)*x/b;
 b *= 1.7;
 p *= 2.6;
 p += t*sin(float(i));
}

 return max(1.-a,0.);
}

 vec3 cloud(vec3 sky, vec3 p){
 float xp = p.y;
 xp = smoothstep(0.3,1.0,xp*8.0);
 p.xy = vec2(p.x,p.z)/max(p.y,0.0);
 p += randC(p.xy)*0.02;
 vec2 uv = p.xy;
 vec4 col = vec4_splat(0.0);
 float x = 1.7;
 for(int i = 0; i<6; i++){
 float re = fbm(uv,x);
 vec3 c = vec3_splat(0.4);
 col.rgb = vec3_splat(0.4);
 col = col+vec4(c,re*0.5)*(1.0-col.a);
 x /= 1.17;
 uv *= 1.015;
 }
 /*
 float a1 = 0.0, a2 = smoothstep(0.0,0.3,fbm(p.xy,1.0));
 float b = 1.0;
 float gr = 1.0;
 for(int j = 0; j<20; j++){
 a1 = smoothstep(0.0,0.3,fbm(p.xy,0.845));
 a1 -= float(j)/20.0;
 p.y *= 1.019;
 if(a1>a2){
 gr *= 0.98;
 }
 }
 gr = mix(1.0,gr,xp);
*/
 float c = 0.0;
 float d = 1.0;
 for(int k = 0; k<10; k++){
 p.y += 0.1;
 float den = smoothstep(0.0,0.01,fbm(p.xy,0.845));
 den *= d;
 d *= 0.95;
 c += den;
 }
 c *= 0.035;
 c = mix(0.0,c,xp);
 c = 1.0-smoothstep(0.0,1.0,c);
 vec3 final = mix(sky,col.rgb,smoothstep(0.0,0.4,col.a)*xp);
  return vec3(final);
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
    float sunDot = saturate(dot(viewDir, sunDir));

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
    vec3 clouds = cloud(skyColor, viewDir);
    clouds *= vec3(0.6, 0.7, 0.85);
    // skyColor = clouds;
    
    // vec4 clouds = cirrus(uvC, vec3(1.0, 0.8, 0.7), normalize(mix(sunDir, moonDir, moonFactor)), viewDir);
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
