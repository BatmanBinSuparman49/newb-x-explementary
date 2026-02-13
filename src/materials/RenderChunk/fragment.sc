$input v_color0, v_color1, v_fog, v_refl, v_texcoord0, v_lightmapUV, v_extra, v_position, v_wpos

#include <bgfx_shader.sh>
#include <newb/main.sh>
#include <newb/functions/custom.h>
#include <newb/functions/starfield.h>
#include <newb/functions/PBR.h>
#include <newb/functions/puddles.h>


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
  return fract(sin(dot(x,vec2(11,57)))*4567.0+ViewPositionAndTime.w);
}

highp float ripplenoise(highp vec2 x, highp float k){
  highp float r = hash(floor(x)), d = length(fract(x)-0.5);

  return smoothstep(0.05,0.025,distance(d, r*k))*smoothstep(0.5,0.475,d)*((1.0-r)*k);
}

highp float ripple(highp vec2 x,float time){
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

  
  float endDist = FogAndDistanceControl.z*0.9;
  bool doEffect = (camDist < endDist);

    vec4 gametime = timedetection(FogColor,FogAndDistanceControl);
    float day1 = gametime.x,
        night1 = gametime.y,
        dusk1 = gametime.z,
        rain1 = gametime.w;

    vec4 whatTime = timeofday(TimeOfDay.x);
      float night = whatTime.x;
      float day   = whatTime.w;
      float dusk  = whatTime.z;
      float dawn  = whatTime.y;

    float weatherFactor = WeatherID.x; 
    float rain          = clamp(WeatherID.x, 0.0, 1.0);
    
  vec4 color = v_color0;

  vec2 lit =  v_lightmapUV;
  float nolight = 1.0 - lit.y;
  
  //sun angle
  float a = radians(45.0);
  vec3 sunVector = normalize(vec3(cos(a), sin(a), 0.2));
  vec3 L = sunVector;
  vec3 V = normalize(-v_wpos.xyz);
  vec3 N = normalize(cross(dFdx(v_position), dFdy(v_position)));
  vec3 sunDir = normalize(SunDirection.xyz);
  vec3 moonDir = mix(sunDir, normalize(vec3(-0.6, 0.45, -0.7)), night * (1.0 - dawn) * (1.0 - dusk));
  
  vec3 blockNormal = getNormal(s_MatTexture, v_texcoord0);
  vec3 worldNormal = normalize(mul((blockNormal),getTBN(N)));
  vec3 reflectNormal = reflect(V, worldNormal);

    bool reflective = false,
       leaf = false,
       grass =  false,
       transparent = false,
       soultorch = false,// not used
       water = v_extra.b > 0.9,
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
 
  #ifdef ALPHA_TEST
    if (diffuse.a < 0.6) {
      discard;
    }
  #endif


  float shadow = smoothstep(0.875,0.860, pow(v_lightmapUV.y,2.0));
  shadow = mix(shadow, 0.0, pow(v_lightmapUV.x * 1.2, 6.0)); 
  float shadowFactor = 1.0 - 0.15 * shadow;
  shadowFactor = max(shadowFactor, 0.4); 
  diffuse.rgb *= shadowFactor;

  // side block shadows
  float sideshadow = smoothstep(0.64, 0.62, v_color1.g);
  diffuse.rgb *= 1.0-0.25*sideshadow;                     //increase 0.38 = darker shadow

  #if defined(SEASONS) && (defined(OPAQUE) || defined(ALPHA_TEST))
    diffuse.rgb *= mix(vec3(1.0,1.0,1.0), texture2D(s_SeasonsTexture, v_color1.xy).rgb * 2.0, v_color1.z);
  #endif

  vec3 glow = nlGlow(s_MatTexture, v_texcoord0, v_extra.a);

  diffuse.rgb *= diffuse.rgb;
 
  vec3 lightTint = texture2D(s_LightMapTexture, v_lightmapUV).rgb;
  lightTint = mix(lightTint.bbb, lightTint*lightTint, 0.35 + 0.65*v_lightmapUV.y*v_lightmapUV.y*v_lightmapUV.y);

  color.rgb *= lightTint;


  #if defined(TRANSPARENT) && !(defined(SEASONS) || defined(RENDER_AS_BILLBOARDS))
    if (v_extra.b > 0.9) {
      diffuse.rgb = vec3_splat(1.0 - NL_WATER_TEX_OPACITY*(1.0 - diffuse.b*1.8));
      diffuse.a = color.a;
    }
  #else
    diffuse.a = 1.0;
  #endif

  float isLeaf = 0.0;
  #if defined(SEASONS) && (defined(ALPHA_TEST) || defined(OPAQUE))
    isLeaf = 1.0;
  #endif

  diffuse.rgb *= color.rgb;
  diffuse.rgb += glow;

  vec3 viewDir = normalize(v_wpos.xyz);

  bool blockUnderWater = (v_lightmapUV.y < 0.9 && abs((2.0 * v_position.y - 15.0) / 16.0 - v_lightmapUV.y) < 0.00002);
  float causticDist = FogAndDistanceControl.z*0.5;
  bool doCaustics = (camDist < causticDist);

  // Caustics
  if(doCaustics){
    if (env.underwater || blockUnderWater){
      float upwards = max(N.y,0.0);
      float caustics = E_UNDW(realPos, v_lightmapUV);
      caustics *= upwards;
      diffuse += caustics;
    
      vec3 watercol = vec3(1.0, 1.0, 1.0);
      diffuse.rgb *= watercol;
    }
  }
  float glossstrength = 0.5;
  sunDir *= (1.0-night);
  vec3 F0 = mix(vec3(0.04, 0.04, 0.04), texcol.rgb, glossstrength);

  // water 
  diffuse = applyWaterEffect(realPos, v_wpos.xyz, viewDir, V, L, texcol.rgb, diffuse, vec4(0,0,0,0), skycol, env, FogColor.rgb, ViewPositionAndTime.w, night, dusk, dawn, rain1, nolight, isCave, water, FogAndDistanceControl.z, camDist, sunDir);

  float moonFactor = night * (1.0 - dawn) * (1.0 - dusk);
  vec3 dawnCol = vec3(1.0, 0.52, 0.278);
  vec3 nightCol = vec3(0.5765, 0.584, 0.98); 
  vec3 dayCol  = vec3_splat(1.0);
  float twilight = saturate(dusk + dawn);
  vec3 specularCol = dawnCol*twilight + dayCol*day + nightCol*night; 

  vec3 specular = brdf(normalize(mix(sunDir, moonDir, moonFactor)), V, 0.2, worldNormal, diffuse.rgb, 0.0, F0, vec3(1.0, 1.0, 1.0));
  float fresnel = pow(1.0 - dot(V, worldNormal), 3.0); 
  viewDir = reflect(viewDir, worldNormal);

  float ndotl = max(dot(N, normalize(mix(sunDir, moonDir, moonFactor))),0.0);
  float dirlight = ndotl + 0.2;

  /* if(v_extra.b < 0.9){
    diffuse.rgb *= dirlight;
  } */
  
  vec3 cloudPos = v_wpos.xyz;
  cloudPos.xz = 3.0 * viewDir.xz / viewDir.y;
  
  vec3 p = normalize(v_wpos.xyz); 

  // firmaments declarations for using in reflections  
  vec3 skyReflection = getSkyRefl(skycol, env, viewDir, FogColor.rgb, ViewPositionAndTime.w);
  vec4 clouds = renderClouds(cloudPos.xz, 0.1 * ViewPositionAndTime.w, rain1, skycol.horizonEdge, skycol.zenith, NL_CLOUD3_SCALE, NL_CLOUD3_SPEED, NL_CLOUD3_SHADOW);
  vec3 galaxyStars = nlGalaxy(viewDir, FogColor.rgb, env, ViewPositionAndTime.w);

  // specular highlights 
  float specDist = FogAndDistanceControl.z*0.67;
    if(!env.end && !env.nether && v_extra.b<0.9 && !reflective && !blockUnderWater && isLeaf==0.0){
      vec3 specHighlights = brdf_specular(normalize(mix(sunDir, moonDir, moonFactor)), V, worldNormal, 0.65, F0, specularCol);
      specHighlights     *= (1.0-sideshadow);
      specHighlights     *= (1.0-shadow);
      diffuse.rgb += specHighlights;
    } 

  /* // puddles 
  float n1 = worley2(realPos.xz * 0.3).y - worley2(realPos.xz * 0.3 ).x;
  float n2 = worley2(realPos.xz * 0.02).y - worley2(realPos.xz * 0.02 ).x;
  float puddleNoise = mix(n1, n2, 0.25);
  float puddleMask  = smoothstep(0.35, 0.45, puddleNoise);
  puddleMask        = pow(puddleMask, 0.46);
  vec3 puddleSpec = brdf(normalize(mix(sunDir, moonDir, moonFactor)), V, 0.1, worldNormal, diffuse.rgb, 0.0, F0, vec3(0.05, 0.05, 0.05)*(1.0+night));
  float wetness = puddleMask * rain;
  puddleSpec *= wetness;
  vec3 puddleRefl = skyReflection * 1.2;
  float reflStrength = wetness * fresnel;
  if(v_extra.b < 0.9 && !blockUnderWater && !env.nether && !env.end && !env.underwater){
    vec3 puddleBase = diffuse.rgb * mix(1.0, 0.4, wetness);
    vec3 puddleColor = mix(puddleBase, puddleRefl, reflStrength);
    diffuse.rgb = puddleColor;
    //puddleSpec    *= mix(1.0, 3.0, wetness);
    //puddleSpec    *= (1.0-sideshadow);
    //puddleSpec    *= (1.0-shadow);
    //diffuse.rgb += puddleSpec*0.67;
  } */

  /* vec3 rippleN = norm(realPos.xz, ViewPositionAndTime.w * 1.6);
  float Rupwards = max(N.y, 0.0);
  vec2 storeuv = v_texcoord0;
  vec2 texuv   = reflect(vec3(storeuv,1.0), normalize(rippleN*2.0-1.0)*0.125);
  vec4 rippleDiffuse = texture2D(s_MatTexture, texuv);
  if(!water && !blockUnderWater && !env.underwater && !env.nether && !env.end){
    rippleDiffuse *= Rupwards;
    rippleDiffuse.rgb *= vec3(0.257, 0.257, 0.265);
    diffuse += rippleDiffuse*rain;
  } */

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
      diffuse.rgb += stars * 2.1;
    }
  } 

  if(reflective && !water){
    reflection  += fstars * 0.7;
    diffuse.rgb *= 1.0 - F0;
    diffuse.rgb = mix(diffuse.rgb, reflection, diffuse.a * fresnel);
    diffuse.rgb += specular; 
  }

  diffuse.rgb = mix(diffuse.rgb, v_fog.rgb, v_fog.a);

  diffuse.rgb = colorCorrection(diffuse.rgb);

  gl_FragColor = diffuse;
}
