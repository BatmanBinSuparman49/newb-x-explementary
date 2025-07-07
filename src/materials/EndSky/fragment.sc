#ifndef INSTANCING
$input v_texcoord0, v_posTime, v_fogColor
#endif

#include <bgfx_shader.sh>

#ifndef INSTANCING
  #include <newb/main.sh>

  SAMPLER2D_AUTOREG(s_SkyTexture);
#endif

// Starfield shader constants
#define NUM_LAYERS 2.78
#define TAU 6.28318
#define PI 3.141592
#define Velocity 0.001 //modified value to increse or decrease speed, negative value travel backwards
#define StarGlow 0.136
#define StarSize 2.1
#define CanvasView 20.0

float Star(vec2 uv, float flare) {
    float d = length(uv);
    float m = sin(StarGlow*1.2)/d;  
    float rays = max(0., .5-abs(uv.x*uv.y*1000.)); 
    m += (rays*flare)*2.;
    m *= smoothstep(1., .1, d);
    return m;
}

float Hash21(vec2 p) {
    p = fract(p*vec2(123.34, 456.21));
    p += dot(p, p+45.32);
    return fract(p.x*p.y);
}

vec3 StarLayer(vec2 uv, float time) {
    vec3 col = vec3(0.0, 0.0, 0.0);
    vec2 gv = fract(uv);
    vec2 id = floor(uv);
    for(int y=-1;y<=1;y++) {
        for(int x=-1; x<=1; x++) {
            vec2 offs = vec2(x,y);
            float n = Hash21(id+offs);
            float size = fract(n);
            float star = Star(gv-offs-vec2(n, fract(n*34.))+.5, smoothstep(.1,.9,size)*.46);
            vec3 color = sin(vec3(.2,.3,.9)*fract(n*2345.2)*TAU)*.25+.75;
            color = color*vec3(.9,.59,.9+size);
            star *= sin(time*.6+n*TAU)*.5+.5;
            col += star*size*color;
        }
    }
    return col;
}

vec3 renderStarfield(vec2 uv, float time) {
    vec2 M = vec2(0.0, 0.0);
    M -= vec2(M.x+sin(time*0.22), M.y-cos(time*0.22));
    // If you want mouse interaction, you'll need to handle that separately
    // M +=(iMouse.xy-iResolution.xy*.5)/iResolution.y;
    
    float t = time*Velocity; 
    vec3 col = vec3(0.0, 0.0, 0.0);  
    for(float i=0.; i<1.; i+=1./NUM_LAYERS) {
        float depth = fract(i+t);
        float scale = mix(CanvasView, .5, depth);
        float fade = depth*smoothstep(1.,.9,depth);
        col += StarLayer(uv*scale+i*453.2-time*.05+M, time)*fade;
    }
    return col;
}

// YSS shooting star (ig)
float random (in vec2 st) {
    return fract(sin(dot(st.xy,
                         vec2(12.9898,78.233)))
                 * 43758.5453123);
}

float noise(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);
    
    // Sample noise at four corners
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));

    // Cubic interpolation (smoother than linear)
    vec2 u = f*f*(3.0-2.0*f);
    return mix(a, b, u.x) + 
           (c - a)*u.y*(1.0 - u.x) + 
           (d - b)*u.x*u.y;
}

// Color generator using trigonometric patterns
vec3 random_color(float seed) {
    return 0.5 + 0.5*vec3(
        sin(seed * 1.0),
        sin(seed * 2.0),
        sin(seed * 3.0)
    );
}

void main() {
  #ifndef INSTANCING
    vec3 viewDir = normalize(v_posTime.xyz);
    vec4 diffuse = texture2D(s_SkyTexture, v_texcoord0);

    nl_environment env;
    env.end = true;
    env.underwater = false;
    env.rainFactor = 0.0;

    vec3 color = renderEndSky(getEndHorizonCol(), getEndZenithCol(), viewDir, v_posTime.w);
    //color += 0.2 * diffuse.rgb;

    // Convert view direction to UV coordinates for the starfield
    vec2 uv = viewDir.xy / (viewDir.z + 1.0); // Simple projection
    
    // Add the starfield effect
    color += renderStarfield(uv * 10.0, v_posTime.w) * 0.5; // Adjust multiplier as needed

    /* 
    vec4 bh = renderBlackhole(normalize(v_posTime.xyz), v_posTime.w);
    color = mix(color, bh.rgb, bh.a);
    */

    #ifdef NL_END_GALAXY_STARS
      color.rgb += NL_END_GALAXY_STARS*nlRenderGalaxy(viewDir, color, env, v_posTime.w);
    #endif

    #ifdef NL_END_CLOUD
      color.rgb += renderCloudsEnd(getEndHorizonCol(), getEndZenithCol(), viewDir, v_posTime.w);
    #endif

    vec2 st = viewDir.xy / (viewDir.z + 1.0);
    st.x *= iResolution.z; // Aspect ratio correction if needed
        
    vec2 expanded = st * 10.0;
    vec2 grid = floor(expanded);
    grid.x += v_posTime.w * 0.3; // Animate
        
    float n = noise(grid) * 0.08 + 0.08;
    vec3 border = random_color(grid.y + floor(v_posTime.w*0.05)*50.0);
        
    color += border * (1.0-smoothstep(0.01,0.02,fract(expanded).y));

    color = colorCorrection(color);

    gl_FragColor = vec4(color, 1.0);
  #else
    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
  #endif
}
