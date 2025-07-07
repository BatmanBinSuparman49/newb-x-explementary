$input v_color0, v_color1, v_fog, v_refl, v_texcoord0, v_lightmapUV, v_extra, v_position, v_wpos

#include <bgfx_shader.sh>
#include <newb/main.sh>
#include "src\newb\functions\detections.h"


SAMPLER2D_AUTOREG(s_MatTexture);
SAMPLER2D_AUTOREG(s_SeasonsTexture);
SAMPLER2D_AUTOREG(s_LightMapTexture);

uniform vec4 FogAndDistanceControl;
uniform vec4 ViewPositionAndTime;
uniform vec4 FogColor;
uniform vec2 iResolution;
uniform float iTime;
uniform vec4 env;

float lumaGrayscale(vec3 d) {
    float luma = dot(d, vec3(0.299, 0.587, 0.114));
    return luma;
}

mat3 getTBN(vec3 normal) {
    vec3 T = vec3(abs(normal.y) + normal.z, 0.0, normal.x);
    vec3 B = vec3(0.0, -abs(normal).x - abs(normal).z, abs(normal).y);
    vec3 N = normal;

    return transpose(mat3(T, B, N));
}
 //based on Grey's normal mapping 
float luminance601(vec3 color) {
  return color.r*0.299 + color.g*0.587 + color.b*0.114;
}

//added resolution 
#define RESOLUTION 512
vec3 getNormal(sampler2D TEXTURE_0, vec2 coord) {
    float offsets = 1.0 / float(RESOLUTION) / 32.0;

    float lumR = luminance601(texture2DLod(TEXTURE_0, coord + vec2(offsets, 0.0), 0.0).rgb);
    float lumL = luminance601(texture2DLod(TEXTURE_0, coord - vec2(offsets, 0.0), 0.0).rgb);
    float lumD = luminance601(texture2DLod(TEXTURE_0, coord + vec2(0.0, offsets), 0.0).rgb);
    float lumU = luminance601(texture2DLod(TEXTURE_0, coord - vec2(0.0, offsets), 0.0).rgb);

vec2 gradient = vec2(lumR - lumL, lumD - lumU);
float lenSq = dot(gradient, gradient);
vec3 normal = normalize(vec3(gradient, sqrt(max(0.0, 1.0 - lenSq))));

    return normalize(normal);
}


float GGX (vec3 n, vec3 v, vec3 l, float r, float F0) {
  r*=r;r*=r;
  vec3 h = normalize(l + v);

  float dotLH = max(0., dot(l,h));
  float dotNH = max(0., dot(n,h));
  float dotNL = max(0., dot(n,l));
  
  float denom = (dotNH * r - dotNH) * dotNH + 1.;
  float D = r / (3.141592653589793 * denom * denom);
  float F = F0 + (1. - F0) * pow(1.-dotLH,5.);
  float k2 = .25 * r;

  return dotNL * D * F / (dotLH*dotLH*(1.0-k2)+k2);
}
/*
//dirlight
float dirlight(vec3 normal, float rain, float night) {
    float dirfac = mix(0.4, 0.2, night);
    dirfac = mix(dirfac, 0.0, rain);
    float dir = 1.0 - dirfac * abs(normal.x);
    return dir;
}
*/

//rain puddles/ ripples

//definitions
#define saturate(x) clamp(x,0.0,1.0)
#define rot(x) mat2(cos(x), sin(x), -sin(x), cos(x))

// the function
highp float hash(
	highp vec2 x){
return fract(sin(
	dot(x,vec2(11,57)))*4567.0+iTime);
	}

highp float ripple(
	highp vec2 x,
	highp float k){
highp float r = hash(floor(x)),
d = length(fract(x)-0.5);

return smoothstep(0.05,0.025,distance(d, r*k))*
	smoothstep(0.5,0.475,d)*((1.0-r)*k);
	}

highp float ripple(
	highp vec2 x){
highp float result = 0.0;
for(int i = 0; i<3; i++){
x -= 6.0;
x = mul(rot(0.3333), x);
result += ripple(x+iTime*0.5,float(i)/3.0+1.0);
	}
return result;
	}

highp vec3 norm(
	highp vec2 x) {
highp float

m = 0.01,
r = ripple(x-vec2(m,0)),
g = ripple(x-vec2(0,m)),
b = ripple(x);

r = r-b,
g = g-b;
b = 1.0-(r+g);

return (vec3(r+0.5,g+0.5,b*m+1.0));
}

void main() {
  #if defined(DEPTH_ONLY_OPAQUE) || defined(DEPTH_ONLY) || defined(INSTANCING)
    gl_FragColor = vec4(1.0,1.0,1.0,1.0);
    return;
  #endif

  vec4 diffuse = texture2D(s_MatTexture, v_texcoord0);
vec4 texcol  = texture2D(s_MatTexture,  v_texcoord0);
    
  vec4 color = v_color0;
  
  //sun angle
    float a = radians(45.0);
  vec3 sunVector = normalize(vec3(cos(a), sin(a), 0.2));
 vec3  L = sunVector;
 vec3  V = normalize(-v_wpos);
  vec3 N = normalize(cross(dFdx(v_position), dFdy(v_position))); 
  
 vec3 blockNormal = getNormal(s_MatTexture,v_texcoord0);
  vec3 worldNormal = normalize(mul((blockNormal),getTBN(N)));
  vec3 reflectNormal = reflect(V,worldNormal);
 
  bool reflective = false;
 #if !defined(TRANSPARENT) && !defined(ALPHA_TEST)
 bool detecttexture = texcol.a > 0.965 && texcol.a < 0.975; 
 if(detecttexture){
    reflective = true;
        }
 #endif

 #if !defined(TRANSPARENT) && !defined(ALPHA_TEST) 
 #endif
 
  #ifdef ALPHA_TEST
    if (diffuse.a < 0.6) {
      discard;
    }
  #endif

  float shadow = smoothstep(0.875,0.860, pow(v_lightmapUV.y,2.0));
  shadow = mix(shadow, 0.0, pow(v_lightmapUV.x * 1.2, 6.0)); 
  diffuse.rgb *= 1.0-0.4*shadow;

  #if defined(SEASONS) && (defined(OPAQUE) || defined(ALPHA_TEST))
    diffuse.rgb *= mix(vec3(1.0,1.0,1.0), texture2D(s_SeasonsTexture, v_color1.xy).rgb * 2.0, v_color1.z);
  #endif

  vec3 glow = nlGlow(s_MatTexture, v_texcoord0, v_extra.a);

  diffuse.rgb *= diffuse.rgb;
  env.rainFactor = detectRain(FOG_CONTROL.xyz);
  bool isRaining = env.rainFactor
  if (isRaining){
    vec2 uv = (v_texcoord0 * iResolution - iResolution*0.5) / iResolution.x;
    vec3 rippleNormal = norm(uv * 6.0);
    rippleNormal = normalize(rippleNormal * 2.0 - 1.0); // Convert from [0,1] to [-1,1]

    N = normalize(N + rippleNormal * 0.2); 
    worldNormal = normalize(mul(blockNormal, getTBN(N)));
    }
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

  diffuse.rgb *= color.rgb;
  diffuse.rgb += glow;

  if (v_extra.b > 0.9) {
    diffuse.rgb += v_refl.rgb*v_refl.a;
  } else if (v_refl.a > 0.0) {
    // reflective effect - only on xz plane
    float dy = abs(dFdy(v_extra.g));
    if (dy < 0.0002) {
      float mask = v_refl.a*(clamp(v_extra.r*10.0,8.2,8.8)-7.8);
      diffuse.rgb *= 1.0 - 0.6*mask;
      diffuse.rgb += v_refl.rgb*mask;
    }
  }

vec3 viewDir = normalize(v_wpos);
viewDir = reflect(viewDir, worldNormal);

float glossstrength = 0.72;

vec3 F0 = mix(vec3(0.04, 0.04, 0.04), texcol.rgb, glossstrength);
float spec = GGX(worldNormal,V,L,0.09,0.75);
float fresnel = pow(1.0 - dot(V, worldNormal), 5.0);
  
  nl_environment env = nlDetectEnvironment(FogColor.rgb, FogAndDistanceControl.xyz);
 
    nl_skycolor skycol;
  if (env.underwater) {
     skycol = nlUnderwaterSkyColors(env.rainFactor, FogColor.rgb);
    } else {
     skycol = nlOverworldSkyColors(env.rainFactor,FogColor.rgb);
    }
    
 vec3 skyColor = getSkyRefl(skycol, env, viewDir, FogColor.rgb, ViewPositionAndTime.w);
 
   //replace this with your sky and clouds 
vec3 reflection = skyColor;

if (reflective){

diffuse.rgb *= 1.0 - F0;
  diffuse.rgb = mix(diffuse.rgb, reflection, diffuse.a * fresnel);
  diffuse.rgb += spec;
  }
/*
float day = pow(max(min(1.0 - FogColor.r * 1.2, 1.0), 0.0), 0.4);
  float night = pow(max(min(1.0 - FogColor.r * 1.5, 1.0), 0.0), 1.2);
  float dusk = max(FogColor.r - FogColor.b, 0.0);
  float rain = mix(smoothstep(0.66, 0.3, FogAndDistanceControl.x), 0.0, step(FogAndDistanceControl.x, 0.0));

diffuse.rgb *= dirlight(N,rain,night);
*/



  diffuse.rgb = mix(diffuse.rgb, v_fog.rgb, v_fog.a);

  diffuse.rgb = colorCorrection(diffuse.rgb);

  gl_FragColor = diffuse;
}
