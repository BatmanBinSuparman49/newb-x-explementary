#ifndef TONEMAP_H
#define TONEMAP_H
vec3 filmic(vec3 x) {
      float A = 0.15;
      float B = 0.50;
      float C = 0.10;
      float D = 0.20;
      float E = 0.02;
      float F = 0.30;
      return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
    }
    
      vec3 lottes(vec3 x) {
      const float a = 1.6;
      const float d = 0.977;
      const float hdrMax = 8.0;
      const float midIn = 0.18;
      const float midOut = 0.267;
      const float c = (hdrMax * pow(midIn, a)) / (midOut * (pow(hdrMax, a) - pow(midIn, a)));
      const float b = (pow(midIn, a) * (pow(hdrMax, a) - pow(hdrMax, a) * midOut)) / (midOut * (pow(hdrMax, a) - pow(midIn, a)));
      return pow(x, vec3_splat(a)) / (pow(x, vec3_splat(a)) + c) + b;
    }
  
    vec3 reinhardJodie(vec3 x) {
      float l = dot(x, vec3(0.2126, 0.7152, 0.0722));
      vec3 tc = x / (1.0 + x);
      return mix(x / (1.0 + l), tc, tc);
    }
  
   vec3 hable(vec3 x) {
      float A = 0.22;
      float B = 0.30;
      float C = 0.10;
      float D = 0.20;
      float E = 0.01;
      float F = 0.30;
      return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
    }

   vec3 aces(vec3 x) {
      const float a = 2.51;
      const float b = 0.03;
      const float c = 2.43;
      const float d = 0.59;
      const float e = 0.14;
      return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
    }
   
vec3 colorCorrection(vec3 col) {
  #ifdef NL_EXPOSURE
    col *= NL_EXPOSURE;
  #endif

  // ref - https://64.github.io/tonemapping/
  #if NL_TONEMAP_TYPE == 3
    // extended reinhard tonemap
    const float whiteScale = 0.063;
    col = col*(1.0+col*whiteScale)/(1.0+col);
  #elif NL_TONEMAP_TYPE == 4
    // aces tonemap
    const float a = 1.04;
    const float b = 0.03;
    const float c = 0.93;
    const float d = 0.56;
    const float e = 0.14;
    col *= 0.85;
    col = clamp((col*(a*col + b)) / (col*(c*col + d) + e), 0.0, 1.0);
  #elif NL_TONEMAP_TYPE == 2
    // simple reinhard tonemap
    col = col/(1.0+col);
  #elif NL_TONEMAP_TYPE == 1
    // exponential tonemap
    col = 1.0-exp(-col*0.8);
  #elif NL_TONEMAP_TYPE == 5
    // Uncharted 2 filmic tonemap
    
    col = filmic(col * 2.0) / filmic(vec3_splat(11.2));
  #elif NL_TONEMAP_TYPE == 6
    // Lottes tonemap
    col = lottes(col);
  #elif NL_TONEMAP_TYPE == 7
    // Reinhard-Jodie tonemap
    col = reinhardJodie(col);
  #elif NL_TONEMAP_TYPE == 8
    // Hable's filmic tonemap
    col = hable(col * 2.0) / hable(vec3_splat(11.2));
  #elif NL_TONEMAP_TYPE == 9
    // ACES approximation (Narkowicz 2015)
    col = aces(col * 0.6);
  #elif NL_TONEMAP_TYPE == 10
    // Linear tonemap
    col = clamp(col, 0.0, 1.0);
  #endif

  // gamma correction
  col = pow(col, vec3_splat(1.0/NL_GAMMA));

  #ifdef NL_SATURATION
    col = mix(vec3_splat(dot(col,vec3(0.21, 0.71, 0.08))), col, NL_SATURATION);
  #endif

  #ifdef NL_TINT
    col *= mix(NL_TINT_LOW, NL_TINT_HIGH, col);
  #endif

  return col;
}
// inv used in fogcolor for nether
vec3 colorCorrectionInv(vec3 col) {
  #ifdef NL_TINT
    col /= mix(NL_TINT_LOW, NL_TINT_HIGH, col); // not accurate inverse
  #endif

  #ifdef NL_SATURATION
    col = mix(vec3_splat(dot(col,vec3(0.21, 0.71, 0.08))), col, 1.0/NL_SATURATION);
  #endif

  // incomplete
  // extended reinhard only
  float ws = 0.7966;
  col = pow(col, vec3_splat(NL_GAMMA));
  col = col*(ws + col)/(ws + col*(1.0 - ws));

  #ifdef NL_EXPOSURE
    col /= NL_EXPOSURE;
  #endif

  return col;
}

#endif
