#ifndef STARFIELD_H
#define STARFIELD_H
#include <newb/config.h>

vec2 starfieldProjection(vec3 viewDir) {
    vec3 dir = normalize(viewDir);
    vec2 uv = dir.xz / (dir.y + 1.0);
    
    // Adjust to control star density and distribution:
    return uv * 10.0;  // Try values between 6.0-15.0
}

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

vec3 renderStarfield(vec3 viewDir, float time) {
    // Use the simple, reliable projection
    vec2 uv = starfieldProjection(viewDir);
    
    // Optional: Very subtle rotation (remove if unwanted)
    float rotation = time * 0.01;
    float cosRot = cos(rotation);
    float sinRot = sin(rotation);
    uv = vec2(
        cosRot * (uv.x - 0.5) - sinRot * (uv.y - 0.5) + 0.5,
        sinRot * (uv.x - 0.5) + cosRot * (uv.y - 0.5) + 0.5
    );
    
    vec2 M = vec2(0.0, 0.0);
    M -= vec2(M.x + sin(time * 0.22), M.y - cos(time * 0.22));
    
    float t = time * VELOCITY; 
    vec3 col = vec3(0.0, 0.0, 0.0);  
    
    for(float i = 0.0; i < 1.0; i += 1.0 / NUM_LAYERS) {
        float depth = fract(i + t);
        float scale = mix(CanvasView, 0.5, depth);
        float fade = depth * smoothstep(1.0, 0.9, depth);
        
        vec2 layerUV = uv * scale + i * 453.2 - time * 0.05 + M;
        col += StarLayer(layerUV, time) * fade;
    }
    
    return col;
}
#endif
