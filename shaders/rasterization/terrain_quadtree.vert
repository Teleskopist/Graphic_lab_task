#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "../unpack_attributes.h"


layout(location = 0) in vec4 vPosNorm;
layout(location = 1) in vec4 vTexCoordAndTang;


layout(push_constant) uniform PushConstants
{
    mat4 model;
    mat4 view;
    mat4 proj;
} pushConstants;


layout (location = 0 ) out VS_OUT
{
    vec3 wPos;
    vec3 wNorm;
    vec3 wTangent;
    vec2 texCoord;

} vOut;

// layout (binding=0, set=1) uniform sampler2D heightMap;
layout(binding=0, set=1) readonly buffer HeightMapBuffer
{
    uint data[];
} heightMapBuffer;

#define SCALE 0.25f

// vec4 f(vec2 tex_coord)
// {
//     const vec2 size = vec2(2.0,0.0);
// const ivec3 off = ivec3(-1,0,1);
// #define unit_wave heightMap
//     vec4 wave = texture(unit_wave, tex_coord);
//     float s11 = wave.x;
//     float s01 = textureOffset(unit_wave, tex_coord, off.xy).x;
//     float s21 = textureOffset(unit_wave, tex_coord, off.zy).x;
//     float s10 = textureOffset(unit_wave, tex_coord, off.yx).x;
//     float s12 = textureOffset(unit_wave, tex_coord, off.yz).x;
//     vec2 size1 = textureSize(heightMap, 0);
//     float C = 1000.0f;
//     vec3 va = normalize(vec3(size.xy / size1 / size,(s21-s01)));
//     vec3 vb = normalize(vec3((size / size1).yx,(s12-s10)));
//     vec4 bump = vec4( cross(va,vb), s11 );
//     return bump;
// }

// float height(vec2 p)
// {
//     // return texture(heightMap, p).x * SCALE;
//     // vec4 g = textureGather(heightMap, p, 0);
//     // vec2 texSize = textureSize(heightMap, 0);
//     // vec2 uv_texel = p * texSize - 0.5f;
//     // vec2 f = fract(uv_texel);
//     // float top = mix(g.x, g.y, f.x);
//     // float bottom = mix(g.w, g.z, f.x);
//     // return mix(bottom, top, f.y) * SCALE;
//     return f(p).w;
// }

// vec3 normal(vec2 p)
// {
// #define ADJ(i, j) vec3(p.x + i / 3000.0f, textureOffset(heightMap, p, ivec2(i, j)).x * SCALE, p.y + j / 3000.0f)
    
//     // float h_L = textureOffset(heightMap, p, ivec2(-1, 0)).x * SCALE;
//     // vec3 L = vec3(p.x - 1, h_L, p.z);
//     // float R = textureOffset(heightMap, p, ivec2(1, 0)).x * SCALE;
//     // float D = textureOffset(heightMap, p, ivec2(0, -1)).x * SCALE;
//     // float U = textureOffset(heightMap, p, ivec2(0, 1)).x * SCALE;
//     // return normalize(vec3(L - R, 2.0f, U - D));
//     // vec3 L = ADJ(-1, 0);
//     // vec3 R = ADJ(1, 0);
//     // vec3 U = ADJ(0, 1);
//     // vec3 D = ADJ(0, -1);
//     // return normalize(cross(L-R, U-D));
//     //vec4 g = textureGather(heightMap, p, 0);
//     //return g.xyz;
//     // vec4 g = textureGather(heightMap, p, 0);
//     // vec2 texSize = textureSize(heightMap, 0);
//     // vec2 uv_texel = p * texSize - 0.5f;
//     // vec2 f = fract(uv_texel);
//     // float top = mix(g.x, g.y, f.x);
//     // float bottom = mix(g.w, g.z, f.x);
//     // //return mix(bottom, top, f.y) * SCALE;
//     // float dfX = (1.0f - f.y) * (g.z - g.w) + f.y * (g.y - g.x);
//     // float dfY = (1.0f - f.x) * (g.x - g.w) + f.x * (g.y - g.z);
//     // float du = texSize.x * dfX;
//     // float dv = texSize.y * dfY;
//     // return normalize(vec3(-du, 1.0f / SCALE, -dv));
//     // vec2 EPS = vec2(0.0001f, 0.0f);
    
//     // float du = height(p + EPS.xy) - height(p - EPS.xy);
//     // float dv = height(p + EPS.yx) - height(p - EPS.yx);
//     // return normalize(vec3(-du, 2 * EPS.x, -dv));
//     return f(p).xzy;
// }

uint get(uint i)
{
    return heightMapBuffer.data[i];
}

vec3 inter(float x, float y, float v00, float v01, float v10, float v11)
{
    float f = (1.0f - x) * (1.0f - y) * v00 + x * (1.0f - y) * v10 + (1.0f - x) * y * v01 + x * y * v11;

    float fdx = (1.0f - y) * (v10 - v00) + y * (v11 - v01);
    float fdy = (1.0f - x) * (v01 - v00) + x * (v11 - v10);
    return vec3(f, fdx, fdy);
}

// float inter(float x, float y, float v00, float v01, float v10, float v11)
// {
//     return (1.0f-x) * (1.0f-y) * v00 + x*(1.0f-y)*v10 + (1.0f-x)*y*v01 + x*y*v11;
// }

vec4 quadtree_sample(vec2 p)
{
    p = fract(p);

    uint root_addr = get(0);
    float F = 1.0f;
    uint curr_addr = root_addr;
    while (true) {
        uint curr_value = get(curr_addr);
        if (bool(curr_value & 0x80000000u)) // curr_addr is not leaf, curr_value is addr of first child
        {
            //uint first_addr = curr_value & 0x7fffffffu;
            uint offset = 0;
            if (p.x < 0.5 && p.y < 0.5) { 
                p *= 2.0f;
                // curr_addr = first_addr + 0; // bottom left
                offset = 0;
            } else if(p.x >= 0.5 && p.y < 0.5) {
                p.x -= 0.5f;
                p *= 2.0f;
                // curr_addr = first_addr + 1; // bottom right
                offset = 1;
            } else if(p.x >= 0.5 && p.y >= 0.5) {
                p -= 0.5f;
                p *= 2.0f;
                // curr_addr = first_addr + 2; // top right
                offset = 2;
            } else {
                p.y -= 0.5f;
                p *= 2.0f;
                offset = 3;
                //curr_addr = first_addr + 3; // top left
            }
            F *= 2.0f;
            curr_addr = get(curr_addr + offset) & 0x7fffffffu;
        } else { // curr_addr is leaf, curr_value is value
            float v00 = intBitsToFloat(int(get(curr_addr + 0)));
            float v10 = intBitsToFloat(int(get(curr_addr + 1)));
            float v11 = intBitsToFloat(int(get(curr_addr + 2)));
            float v01 = intBitsToFloat(int(get(curr_addr + 3)));

            vec3 f = inter(p.x, p.y, v00, v01, v10, v11);

            vec3 N = normalize(vec3(-f.y, 1.0f / F, -f.z));
            return vec4(N, f.x);
        }
    }
}

// float height(vec2 pos)
// {
//     return quadtree_sample(pos);
// }

// vec3 normal(vec2 p)
// {
//     vec2 EPS = vec2(0.00001f, 0.0f);
//     float du = height(p + EPS.xy) - height(p - EPS.xy);
//     float dv = height(p + EPS.yx) - height(p - EPS.yx);
//     return normalize(vec3(-du, 2 * EPS.x, -dv));
// }

out gl_PerVertex { vec4 gl_Position; };
void main(void)
{

    mat4 iv = inverse(pushConstants.view);
    vec3 cameraPos = vec3(iv[3][0], iv[3][1], iv[3][2]);
    cameraPos = vec3(0.0f, 0.0f, 0.0f);

    vec3 pos = vPosNorm.xyz;
    pos.xz += cameraPos.xz;

    vec4 sample1 = quadtree_sample(pos.xz);

    pos.y = sample1.w;
    vec3 N = sample1.xyz;

    mat4 model = pushConstants.model;
    mat4 view = pushConstants.view;
    mat4 proj = pushConstants.proj;

    const vec4 wNorm = vec4(DecodeNormal(floatBitsToInt(vPosNorm.w)),         0.0f);
    const vec4 wTang = vec4(DecodeNormal(floatBitsToInt(vTexCoordAndTang.z)), 0.0f);

    vOut.wPos     = (model * vec4(pos, 1.0f)).xyz;
    vOut.wNorm    = normalize(mat3(transpose(inverse(model))) * N);
    vOut.wTangent = normalize(mat3(transpose(inverse(model))) * wTang.xyz);
    vOut.texCoord = vTexCoordAndTang.xy;

    gl_Position   = proj * view * vec4(vOut.wPos, 1.0);
}
