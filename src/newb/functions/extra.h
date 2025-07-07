SAMPLER2D_AUTOREG(s_MatTexture);
SAMPLER2D_AUTOREG(s_SeasonsTexture);
SAMPLER2D_AUTOREG(s_LightMapTexture);

 uniform vec4 FogColor;
uniform vec4 ViewPositionAndTime;
uniform vec4 FogAndDistanceControl;
float time = ViewPositionAndTime.w;

#define PI 3.14159
//////// -CONFIG- //////
  #define STEPS 5 //layers
  #define SIDECOL vec3(1.0,1.0,1.0) //side col
  #define BOTTOMCOL vec3(0.75,0.75,0.75) //bottom col
  #define CLOUD_SPEED 0.3 //speed
  #define SIZE 4.0 // bigger =more clouds
  #define VISIBLE 1.0 //0.0 - 1.0 visibility of cloud
/////// -END- ///////

#define DYNAMIC 0

////// -FUNCTIONS- /////

float henyeyGreensteinPhase(float cosTheta, float g) {
  return (1.0 - g*g) / (4.0 * PI * pow(1.0 + g*g - 2.0*g*cosTheta, 1.5));
}
  
vec4 fastvanillaClouds(vec2 uv,vec3 sky,float fade) {
	  float a = 0.0;
    float isTime = (time) * 0.3;
    vec4 col;
   // uv *= SIZE; // bigger = more clouds

//layers
    for (int i = 0; i < STEPS; i++) {
        uv /= 1.007;
        float c = step(0.7, rand(floor(uv + isTime)));
        a = mix(a, 1.0, c);
    }

//cloud coloring
     vec2 b = vec2(step(0.7, rand(floor(uv + isTime))));
     vec3 ccol = SIDECOL; //side
     ccol = mix(ccol,BOTTOMCOL,b.x);//bottom

//mix
     float n = a;
     col.rgb =sky;
     col.rgb = mix(col.rgb,ccol,n);
         col.a = clamp(n * fade,0.0,1.0);
 return col;
}

/*Color/Lighting Calculations*/
float lumaGrayscale(vec3 d) {
    float luma = dot(d, vec3(0.299, 0.587, 0.114));
    return luma;
}

vec3 getNormal(sampler2D TEXTURE_0, vec2 uv) {
    	  vec3 base0 = texture2D(TEXTURE_0, uv).rgb;
   highp float base = lumaGrayscale(base0);

   highp float w1 = (base-lumaGrayscale(texture2D(TEXTURE_0, vec2(uv.x-0.00018,uv.y)).rgb));
   highp float w2 = (base-lumaGrayscale(texture2D(TEXTURE_0, vec2(uv.x,uv.y-0.00018)).rgb));

   highp vec3 Normals_w = normalize(vec3(w1,w2,1.5))*0.5 + 0.5;
   return Normals_w;
}

mat3 getTBN(vec3 normal) {
    vec3 T = vec3(abs(normal.y) + normal.z, 0.0, normal.x);
    vec3 B = vec3(0.0, -abs(normal).x - abs(normal).z, abs(normal).y);
    vec3 N = normal;

    return transpose(mat3(T, B, N));
}
float GGX (vec3 n, vec3 v, vec3 l, float r, float F0) {
  r*=r;r*=r;
  vec3 h = normalize(l + v);

  float dotLH = max(0., dot(l,h));
  float dotNH = max(0., dot(n,h));
  float dotNL = max(0., dot(n,l));
  
  float denom = (dotNH * r - dotNH) * dotNH + 1.;
  denom = max(denom,0.0);
  float D = r / (3.141592653589793 * denom * denom);
  float F = F0 + (1. - F0) * pow(1.-dotLH,5.);
  float k2 =r;

  float spec = dotNL * D * F / (dotLH*dotLH*(1.0-k2)+k2);
  return spec;
}


float shadowmap(float lighty){

float shadowmap = smoothstep(0.915,0.890,lighty);
 
return shadowmap;
}

float Bayer2(vec2 a) {
    a = floor(a);
    return fract(a.x / 2. + a.y * a.y * .75);
}


// doksli dither: https://www.shadertoy.com/view/7sfXDn
#define Bayer4(a)   (Bayer2 (.5 *(a)) * .25 + Bayer2(a))
#define Bayer8(a)   (Bayer4 (.5 *(a)) * .25 + Bayer2(a))
#define Bayer16(a)  (Bayer8 (.5 *(a)) * .25 + Bayer2(a))
#define Bayer32(a)  (Bayer16(.5 *(a)) * .25 + Bayer2(a))
#define Bayer64(a)  (Bayer32(.5 *(a)) * .25 + Bayer2(a))





float getHeight(vec2 uv, sampler2D TEXTURE_0) {
vec4 tex = texture2D(TEXTURE_0, uv);

return 1.0*(tex.r + tex.g + tex.b)/3.0; // your wave function or brightness of your texture (tex.r + tex.g + tex.b)/3.0
}

vec4 getNormalMap(vec2 uv, float resolution, float scale, sampler2D TEXTURE_0) {
  vec2 step = 1.0 / resolution  * vec2(2.0, 1.0);

  float height = getHeight(uv,TEXTURE_0);

  vec2 dxy = height - vec2(
      getHeight(uv + vec2(step.x, 0.0), TEXTURE_0),
      getHeight(uv + vec2(0.0, step.y), TEXTURE_0)
  );
  return vec4(normalize(vec3(dxy * scale / step, 1.0)), height);
}


vec3 getSunpos(float time, float night,float dusk){
#define SUN_ANGLE_POSITION 45
#define SUN_ANGLE_POSITION_NIGHT 135
#define	SUN_ANGLE_POSITION_DUSK 45

#define SUN_POSITION_SPEED 1.0
float sunpos;
if(night > 0.5){
   sunpos = float(SUN_ANGLE_POSITION_NIGHT);
   } else if(dusk > 0.3){
   sunpos = float(SUN_ANGLE_POSITION_DUSK);
   } else{
       sunpos = float(SUN_ANGLE_POSITION);
       }
       
    
  float sunAngle = 0.0;
  #if DYNAMIC
    sunAngle = time / 80.0 * SUN_POSITION_SPEED;
  #else
    sunAngle = radians(sunpos);
  #endif
 vec3 sunPos = normalize(vec3(cos(sunAngle), sin(sunAngle), cos(sunAngle) * sin(sunAngle)));
 
 return sunPos;
 }
float dirlight(vec3 normal ,float rain, float night){
float dirfac = mix(0.4, 0.2, night);
dirfac = mix(dirfac, 0.0, rain);
float dir = 1.0-dirfac*abs(normal.x);
 return dir;
 }
//////
//NOT USED
/////

float randb(vec2 n) {
    return
        fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

vec3 sunPosition() {
     float rot = 15.0;
    float sunAngle = rot;
     float sunX = 0.1;
    float sunY = sin(radians(sunAngle));
    float sunZ = cos(radians(sunAngle));
    return vec3(sunX, sunY, sunZ);
}
float hg(float low,const float v){
    float lv=v*v;
    return 0.25*(1.0 / 3.14159265)*
     (1.0-lv)*
     pow(1.0+lv-2.0*v*low,-1.5);
}
float calcu(vec3 a){
    vec3 b = a;
        return randb(floor(4.0 * (b.xz / b.y) + time * 0.51));
}
vec3 beamcast(vec3 dir){
    vec3 q = dir;

    vec3 sp = sunPosition();
    vec3 diff = (q - (sp));
    float ld = dot(sp, dir);
    float alpha = hg(ld, 0.35);

    vec3 occlusion = vec3(0.0, 0.0, 0.0);

    float ditherFactor = Bayer64(gl_FragCoord.xy);
    float decay = 1.0;
    float sc = calcu((q-diff) + (diff * ditherFactor));
    occlusion += sc;
    vec3 lRay = 1.0 - exp(-(occlusion * alpha));
    return lRay;
}

float fogtime(vec4 fogcol) {
    //三次多项式拟合，四次多项式拟合曲线存在明显突出故不使用
    // return fogcol.g > 0.213101 ? 1.0 : (((349.305545 * fogcol.g - 159.858192) * fogcol.g + 30.557216) * fogcol.g - 1.628452);
    return clamp(((349.305545 * fogcol.g - 159.858192) * fogcol.g + 30.557216) * fogcol.g - 1.628452, -1.0, 1.0);
}
bool blocksidewater(vec2 light) {
    // Arbitrary threshold—adjust based on testing
    return light.y < 0.5;
}
//faster and actually more precise than pow 2.2
vec3 toLinear(vec3 sRGB){
	return sRGB * (sRGB * (sRGB * 0.305306011 + 0.682171111) + 0.012522878);
}

vec3 LinearTosRGB(in vec3 color)

{

    vec3 x = color * 12.92f;
    vec3 y = 1.055f * pow(clamp(color,0.0,1.0), vec3(1.0f / 2.4f)) - 0.055f;

    vec3 clr = color;
    clr.r = color.r < 0.0031308f ? x.r : y.r;
    clr.g = color.g < 0.0031308f ? x.g : y.g;
    clr.b = color.b < 0.0031308f ? x.b : y.b;

    return clr;

}
float flare(vec2 uv, vec2 light, float intensity) {
    vec2 dir = uv - light;
    float dist = length(dir/FogAndDistanceControl.z);
    float angle = atan2(dir.y, dir.x);

    // Simple radial streaks
    float streaks = abs(sin(angle * 6.0)) * pow(1.0 - dist, 4.0);

    // Halo glow
    float glow = pow(1.0 - dist, 10.0);

    return streaks * 0.6 + glow * intensity;
}//noise
highp float randk(highp vec2 x){
    return fract(sin(dot(x, vec2(14.56, 56.2))) * 20000.);
}

highp float noise(highp vec2 x){
    highp vec2 ipos = floor(x);
    highp vec2 fpos = fract(x);
    fpos = smoothstep(0., 1., fpos);
    float a = randk(ipos), b = randk(ipos + vec2(1, 0)),
        c = randk(ipos + vec2(0, 1)), d = randk(ipos + 1.);
 return mix(a, b, fpos.x) + (c - a) * fpos.y * (1.0 - fpos.x) + (d - b) * fpos.x * fpos.y;
}