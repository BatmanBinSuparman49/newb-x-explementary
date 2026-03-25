$input v_color0, v_color1, v_fog, v_refl, v_texcoord0, v_lightmapUV, v_extra, v_position, v_wpos

#include <bgfx_shader.sh>
#include <newb/main.sh>
#include <yildrim/starfield.h>
#include <yildrim/pbr.h>
#include <yildrim/puddles.h>
#include <yildrim/waters.h>

SAMPLER2D_AUTOREG(s_MatTexture);
SAMPLER2D_AUTOREG(s_SeasonsTexture);
SAMPLER2D_AUTOREG(s_LightMapTexture);

uniform vec4 FogAndDistanceControl;
uniform vec4 FogColor;
uniform vec4 DimensionID;
uniform vec4 TimeOfDay;
uniform vec4 ViewPositionAndTime;
uniform vec4 SunDirection;
uniform vec4 WeatherID;
uniform vec4 CameraPosition;

#define CAUSTICS_SPEED 0.5

// Caustics
highp float E_UNDW(vec3 v_position, vec2 v_uv1) {

  highp float SCALE = 0.6;
  highp float TAU2 = 6.283;
  int MAX_ITER = 2;

  highp float time = ViewPositionAndTime.w * CAUSTICS_SPEED + 23.0;
  // uv should be the 0-1 uv of texture...
  vec2 uv = v_position.xz * SCALE;
  vec2 p, i; p = i = mod(uv*TAU2, TAU2)-250.0;
  highp float c = 1.0;
  highp float inten = .005;

  for (int n = 0; n < MAX_ITER; n++) {
  highp float t = time * (1.0 - (3.5 / float(n+1)));
  i = p + vec2(cos(t - i.x) + sin(t + i.y), sin(t - i.y) + cos(t + i.x));
  c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten),p.y / (cos(i.y+t)/inten)));
  }
  c /= float(MAX_ITER);
  c = 1.17-pow(c, 1.4);
  float colour = clamp(pow(abs(c), 8.0), 0.0, 0.7 ) * ( v_uv1.y + 0.1 ) * ( 1.0 - v_uv1.x ) * 3.1;
  return colour;
}

// Rain ripples
#define rot(x) mat2(cos(x), sin(x), -sin(x), cos(x))

highp float hash(highp vec2 x){
  float t = ViewPositionAndTime.w * 0.5;
  return fract(sin(dot(x,vec2(11,57)))*4567.0+t);
}

highp float ripplenoise(highp vec2 x, highp float k){
  highp float r = hash(floor(x)), d = length(fract(x)-0.5);

  return smoothstep(0.05,0.025,distance(d, r*k))*smoothstep(0.5,0.475,d)*((1.0-r)*k);
}

highp float ripple(highp vec2 x, float time){
  highp float result = 0.0;

    for(int i = 0; i<3; i++){
    x -= 6.0, x = mul(x, rot(0.3333)),
    result += ripplenoise(x+time*0.5,float(i)/3.0+1.0);
  }

  return result;
}

highp vec3 norm(highp vec2 x, float time) {
  highp float

  m = 0.01,
  r = ripple(x-vec2(m,0),time),
  g = ripple(x-vec2(0,m),time),
  b = ripple(x,time);

  r = r-b,
  g = g-b;
  b = 1.0-(r+g);

  return (vec3(r+0.5,g+0.5,b*m+1.0));
}

vec3 filmicBSBE(vec3 x){
    return (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
}

void main() {
  #if defined(DEPTH_ONLY_OPAQUE) || defined(DEPTH_ONLY) || defined(INSTANCING)
    gl_FragColor = vec4(1.0,1.0,1.0,1.0);
    return;
  #endif

  vec4 diffuse = texture2D(s_MatTexture, v_texcoord0);
  vec4 texcol  = texture2D(s_MatTexture, v_texcoord0);

  vec3 realPos = v_wpos.xyz + CameraPosition.xyz;
  float camDist = v_wpos.w;
  vec3 viewDir = normalize(v_wpos.xyz);

  
  float endDist = FogAndDistanceControl.z*0.9;
  bool doEffect = (camDist < endDist);

    vec4 whatTime = timeofday(TimeOfDay.x);
      float night = whatTime.x;
      float day   = whatTime.w;
      float dusk  = whatTime.z;
      float dawn  = whatTime.y;

  float rain = mix(smoothstep(0.66, 0.3, FogAndDistanceControl.x), 0.0, step(FogAndDistanceControl.x, 0.0));
    
  vec4 color = v_color0;

  vec2 lit =  v_lightmapUV;
  float nolight = 1.0 - lit.y;
  
  //sun angle
  float a = radians(45.0);
  vec3 sunVector = normalize(vec3(cos(a), sin(a), 0.2));
  vec3 L = sunVector;
  vec3 V = normalize(-viewDir);
  vec3 N = normalize(cross(dFdx(v_position), dFdy(v_position)));
  vec3 sunDir = normalize(SunDirection.xyz);
  float sunA = clamp(((349.305545 * FogColor.g - 159.858192) * FogColor.g + 30.557216) * FogColor.g - 1.628452, -1.0, 1.0);
  vec3 sunPos =  normalize(vec3(cos(sunA), sin(sunA), 0.7));
  vec3 moonPos = -sunPos;
  vec3 SunMoonDir = normalize(mix(sunDir, moonPos, night));
  
  vec3 blockNormal = getNormal(s_MatTexture, v_texcoord0);
  vec3 worldNormal = normalize(mul((blockNormal),getTBN(N)));
  vec3 reflectNormal = reflect(V, worldNormal);
  float upwards = max(N.y, 0.0);

  bool water = v_extra.b > 0.9;

    bool reflective = false,
       leaf = false,
       grass =  false,
       transparent = false,
       soultorch = false,
       slightly = false;

    const vec3 Ambient = vec3(0.02, 0.04, 0.08);
    bool isCave = nolight > 0.3;
 
 #if !defined(TRANSPARENT) && !defined(ALPHA_TEST)
 bool detecttexture = texcol.a > 0.965 && texcol.a < 0.975; 
 if(detecttexture){
    reflective = true;
        }
 #endif

 #if !defined(TRANSPARENT) && !defined(ALPHA_TEST) 
 #endif

  nl_environment env = nlDetectEnvironment(FogColor.rgb, FogAndDistanceControl.xyz);
  nl_skycolor skycol;
  if (env.underwater) {
     skycol = nlUnderwaterSkyColors(env.rainFactor, FogColor.rgb);
    } else {
     skycol = nlOverworldSkyColors(env.rainFactor,FogColor.rgb);
    }

  float shadow = smoothstep(0.875, 0.860, pow(v_lightmapUV.y, 2.0));
  shadow = mix(shadow, 0.0, pow(v_lightmapUV.x * 1.2, 6.0)); 
  float shadowFactor = 1.0 - 0.25 * shadow;
  shadowFactor = max(shadowFactor, 0.5); 
  diffuse.rgb *= shadowFactor;

  // side block shadows
  float sideshadow = smoothstep(0.64, 0.62, v_color1.g);
  diffuse.rgb *= 1.0-0.3*sideshadow;                     //increase 0.38 = darker shadow

  #if defined(SEASONS) && (defined(OPAQUE) || defined(ALPHA_TEST))
    diffuse.rgb *= mix(vec3(1.0,1.0,1.0), texture2D(s_SeasonsTexture, v_color1.xy).rgb * 2.0, v_color1.z);
  #endif

  vec3 glow = nlGlow(s_MatTexture, v_texcoord0, v_extra.a);

  diffuse.rgb *= diffuse.rgb;
 
  vec3 lightTint = texture2D(s_LightMapTexture, v_lightmapUV).rgb;
  lightTint = mix(lightTint.bbb, lightTint*lightTint, 0.35 + 0.65*v_lightmapUV.y*v_lightmapUV.y*v_lightmapUV.y);

  color.rgb *= lightTint;

  float isLeaf = 0.0;
  #if defined(SEASONS) && (defined(ALPHA_TEST) || defined(OPAQUE))
    isLeaf = 1.0;
  #endif

  diffuse.rgb *= color.rgb;
  diffuse.rgb += glow;

  bool blockUnderWater = (v_lightmapUV.y < 0.9 && abs((2.0 * v_position.y - 15.0) / 16.0 - v_lightmapUV.y) < 0.00002);
  float causticDist = FogAndDistanceControl.z*0.5;
  bool doCaustics = (camDist < causticDist);

  // water 
  diffuse = applyWaterEffect(realPos, v_wpos.xyz, viewDir, V, L, texcol.rgb, diffuse, vec4(0,0,0,0), skycol, env, FogColor.rgb, ViewPositionAndTime.w, night, dusk, dawn, rain, nolight, isCave, water, FogAndDistanceControl.z, camDist, sunDir);

  // water absorption
  float depth = 1.0 - pow(v_lightmapUV.y,2.0);
  vec3 absorption;
  bool fromSurface = v_lightmapUV.y < 0.9 ;

  vec3 absorptionCoeff = vec3(2.5, 1.8, 1.2); // Red, Green, Blue
  absorption = exp(-depth * absorptionCoeff);
   
  if(water){
    diffuse.rgb *= absorption;
  }

  if(water){
    float fogDensity = depth * 0.5;
    float visibility = exp(-fogDensity);
    diffuse.a *= 0.4 + (visibility * 0.5);
  }

  float moonFactor = night * (1.0 - dawn) * (1.0 - dusk);
  vec3 dawnCol  = vec3(1.0, 0.35, 0.05); 
  vec3 nightCol = vec3(0.5765, 0.584, 0.98); 
  vec3 dayCol  = vec3(1.0, 0.98, 0.95);
  float twilight = saturate(dusk + dawn);
  sunDir *= (1.0-night);
  vec3 specularCol = dawnCol*twilight + dayCol*day + nightCol*night; 


  // Caustics
  if(doCaustics){
    if (env.underwater || blockUnderWater){
      float caustics = E_UNDW(realPos, v_lightmapUV);
      caustics *= upwards;
      diffuse += caustics;
    
      vec3 watercol = specularCol*0.6;
      diffuse.rgb *= watercol;
    }
  }

  /* mat3 TBN = mat3(abs(N).y + N.z, 0.0, -N.x, 0.0, -abs(N).x - abs(N).z, abs(N).y, N);
  vec3 ambVL = texture2D(s_LightMapTexture, vec2(0.0, v_lightmapUV.y)).rgb;
  if(env.underwater){
    if(!water){
      vec3 causDir = normalize(mul(vec3(-1.0, -1.0, 0.1), TBN));
      vec3 uwAmb = ambVL * vec3(0.0, 0.3, 0.8) + pow(v_lightmapUV.x, 3.0);
      uwAmb += pow(saturate(dot(causDir, getWaterNormal(realPos.xz, ViewPositionAndTime.w).xzy)), 3.0) * v_lightmapUV.y * 32768;
      // diffuse.rgb *= saturate(uwAmb);
    }
  } */

  float glossstrength = 0.5;
  vec3 F0 = mix(vec3(0.04, 0.04, 0.04), texcol.rgb, glossstrength);

  vec3 specular = brdf(SunMoonDir, V, 0.2, worldNormal, diffuse.rgb, 0.0, F0, specularCol);
  float fresnel = pow(1.0 - dot(V, worldNormal), 3.0); 
  viewDir = reflect(viewDir, worldNormal);
  
  vec3 cloudPos = v_wpos.xyz;
  cloudPos.xz = 3.0 * viewDir.xz / viewDir.y;
  
  vec3 p = normalize(v_wpos.xyz); 

  // firmaments declarations for using in reflections  
  vec3 skyReflection = getSkyRefl(skycol, env, viewDir, FogColor.rgb, ViewPositionAndTime.w);
  vec4 clouds = renderClouds(cloudPos.xz, 0.1 * ViewPositionAndTime.w, rain, skycol.horizonEdge, skycol.zenith, NL_CLOUD3_SCALE, NL_CLOUD3_SPEED, NL_CLOUD3_SHADOW);
  vec3 galaxyStars = nlGalaxy(viewDir, FogColor.rgb, env, ViewPositionAndTime.w);

  vec3 rippleN = norm(realPos.xz*1.5, ViewPositionAndTime.w);
  vec2 storeuv = v_texcoord0;
  vec2 texuv   = reflect(vec3(storeuv,1.0), normalize(rippleN*2.0-1.0)*0.125);
  vec4 rippleDiffuse = texture2D(s_MatTexture, texuv);
  if(!water && !blockUnderWater && !env.underwater && !env.nether && !env.end){
    rippleDiffuse *= upwards;
    vec3 reflectionEffect = mix(rippleDiffuse.rgb, skyReflection, 0.4);
    rippleDiffuse.rgb = mix(rippleDiffuse.rgb, reflectionEffect, upwards);
    vec3 wetColor = diffuse.rgb * 0.6;
    vec3 wetFinal = mix(wetColor, rippleDiffuse.rgb, 0.45);
    diffuse.rgb = mix(diffuse.rgb, wetFinal, rain); 
  }

  // specular highlights 
  float specDist = FogAndDistanceControl.z*0.67;
    if(!env.end && !env.nether && v_extra.b<0.9 && !reflective && !blockUnderWater && isLeaf==0.0){
      vec3 specHighlights = brdf_specular(SunMoonDir, V, worldNormal, mix(0.65, 3.0, rain), F0, specularCol);
      specHighlights     *= (1.0-sideshadow);
      specHighlights     *= (1.0-shadow);
      diffuse.rgb += specHighlights;
    } 


  /* float n1 = worley2(realPos.xz * 0.3).y - worley2(realPos.xz * 0.3 ).x;
  float n2 = worley2(realPos.xz * 0.02).y - worley2(realPos.xz * 0.02 ).x;
  float puddleNoise = mix(n1, n2, 0.25);
  float puddleMask  = smoothstep(0.35, 0.45, puddleNoise);
  puddleMask        = pow(puddleMask, 0.46);
  vec3 puddleSpec = brdf(SunMoonDir, V, 0.1, worldNormal, diffuse.rgb, 0.0, F0, vec3(0.05, 0.05, 0.05)*(1.0+night));
  float wetness = puddleMask * rain;
  puddleSpec *= wetness;
  vec3 puddleRefl = skyReflection * 1.2;
  float reflStrength = wetness * fresnel;
  if(v_extra.b < 0.9 && !blockUnderWater && !env.nether && !env.end && !env.underwater){
    vec3 puddleBase = diffuse.rgb * mix(1.0, 0.4, wetness);
    vec3 puddleColor = mix(puddleBase, puddleRefl, reflStrength);
    diffuse.rgb = puddleColor;
    diffuse.rgb *= upwards;
    puddleSpec    *= mix(1.0, 3.0, wetness);
    puddleSpec    *= (1.0-sideshadow);
    puddleSpec    *= (1.0-shadow);
    diffuse.rgb += puddleSpec;
  } */

  float downwards = max(-N.y, 0.0);
  float notBottom = 1.0 - downwards;

  vec3 fstars = vec3(0.0, 0.0, 0.0);
  if(!env.end && !env.nether){
  #ifdef FALLING_STARS
    if(!env.underwater) {
        vec2 starUV = viewDir.xz / (0.5 + viewDir.y);
        float starValue = star(starUV * NL_FALLING_STARS_SCALE, NL_FALLING_STARS_VELOCITY, NL_FALLING_STARS_DENSITY, ViewPositionAndTime.w);
        float starFactor = smoothstep(0.5, 1.0, night)*(1.0-rain);
        fstars = pow(vec3(starValue, starValue, starValue) * 1.1, vec3(16.0, 6.0, 4.0));
        fstars *= starFactor;
    }
  #endif
  }
  
  vec3 stars = vec3(0.0, 0.0, 0.0);
  if(!env.end && !env.nether){
    #ifdef NL_GALAXY_STARS
      stars += NL_GALAXY_STARS * galaxyStars;
      stars *= stars;
    #endif
  }
   
  vec3 reflection = skyReflection*0.8;
  if(!env.end && !env.nether){
    reflection = mix(skyReflection*0.8, clouds.rgb, clouds.a * smoothstep(0.05,1.0,viewDir.y));
  }

  vec3 endstars = vec3(0.0, 0.0, 0.0);
  if(env.end){
    #ifdef STARFIELD
      endstars += renderStarfield(viewDir, ViewPositionAndTime.w) * 0.5;
      reflection += endstars; 
    #endif
  }
  
  if(doEffect){
    if(reflective && !water){
      diffuse.rgb += stars * 2.1 * notBottom;
    }
  } 

  if(reflective && !water){
    reflection  += fstars * 0.7;
    diffuse.rgb *= 1.0 - F0;
    float notDown = diffuse.a * fresnel * notBottom;
    diffuse.rgb = mix(diffuse.rgb, reflection, notDown);
    diffuse.rgb += specular * notBottom; 
  }

  diffuse.rgb = mix(diffuse.rgb, v_fog.rgb, v_fog.a);

  diffuse.rgb = colorCorrection(diffuse.rgb);

  #ifdef ALPHA_TEST
    if (diffuse.a < 0.5) {
      discard;
    }
  #endif

  gl_FragColor = diffuse;
}
