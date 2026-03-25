#ifndef PBR_H
#define PBR_H

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

// Cook Torrance's BRDF
vec3 brdf(vec3 lightDir, vec3 viewDir, float roughness, vec3 normal, vec3 albedo, float metallic, vec3 reflectance, vec3 sunCol) {
    
    float alpha = pow(roughness,2.0);

    vec3 H = normalize(lightDir + viewDir);
    

    //dot products
    float NdotV = clamp(dot(normal, viewDir), 0.5,1.0);
    float NdotL = clamp(dot(normal, lightDir), 0.5,1.0);
    float NdotH = clamp(dot(normal,H), 0.5,1.0);
    float VdotH = clamp(dot(viewDir, H), 0.5,1.0);

    // Fresnel
    vec3 F0 = reflectance;
    vec3 fresnelReflectance = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0); //Schlick's Approximation

    //phong diffuse
    vec3 rhoD = albedo;
    rhoD *= (vec3(1.0, 1.0, 1.0)- fresnelReflectance); //energy conservation - light that doesn't reflect adds to diffuse

    rhoD *= (1.0-metallic); //diffuse is 0 for metals

    // Geometric attenuation
    float k = alpha/2.0;
    float geometry = (NdotL / (NdotL*(1.0-k)+k)) * (NdotV / ((NdotV*(1.0-k)+k)));

    // Distribution of Microfacets
    float lowerTerm = pow(NdotH,2.0) * (pow(alpha,2.0) - 1.0) + 1.0;
    float normalDistributionFunctionGGX = pow(alpha,2.0) / (3.14159 * pow(lowerTerm,2.0));

    vec3 phongDiffuse = rhoD;
    vec3 cookTorrance = (fresnelReflectance*normalDistributionFunctionGGX*geometry)/(4.0*NdotL*NdotV);
    
    vec3 BRDF = (phongDiffuse+cookTorrance*sunCol)*NdotL;
   
    vec3 diffFunction = BRDF;
    
    return BRDF;
}

// specular brdf
vec3 brdf_specular(vec3 lightDir, vec3 viewDir, vec3 normal, float roughness, vec3 reflectance, vec3 sunCol) {
    vec3 H = normalize(lightDir + viewDir);

    float NdotV = max(dot(normal, viewDir), 0.0);
    float NdotL = max(dot(normal, lightDir), 0.0);
    float NdotH = max(dot(normal, H), 0.0);
    float VdotH = max(dot(viewDir, H), 0.0);

    float alpha = roughness * roughness;

    vec3  F = reflectance + (1.0 - reflectance) * pow(1.0 - VdotH, 5.0);
    float a2 = alpha * alpha;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    float D = a2 / (3.14159 * denom * denom + 1e-5);

    float k = (alpha + 1.0) * (alpha + 1.0) / 8.0;
    float Gv = NdotV / (NdotV * (1.0 - k) + k);
    float Gl = NdotL / (NdotL * (1.0 - k) + k);
    float G = Gv * Gl;

    vec3 specular = (F * D * G) / (4.0 * NdotV * NdotL + 1e-5);

    vec3 result = specular * sunCol * NdotL;

    return result;
}
#endif 