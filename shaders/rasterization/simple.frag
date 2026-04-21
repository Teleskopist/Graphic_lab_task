#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "../common.h"

layout(location = 0) out vec4 out_fragColor;

layout (location = 0 ) in VS_OUT
{
    vec3 wPos;
    vec3 wNorm;
    vec3 wTangent;
    vec2 texCoord;
} surf;

layout(binding = 0, set = 0) uniform AppData
{
    SimpleFragmentParams params;
};

layout(push_constant) uniform PushConstants
{
    mat4 model;
    mat4 view;
    mat4 proj;
} pushConstants;

void main()
{
    vec3 light = params.ambientLight.xyz;
    vec3 N = normalize(surf.wNorm);

    vec3 pos = surf.wPos.xyz;

    mat4 iv = inverse(pushConstants.view);
    vec3 cameraPos = vec3(iv[3][0], iv[3][1], iv[3][2]);

    vec3 cameraDir = cameraPos - pos;

    // N = dot(N, cameraDir) >=0 ? N : -N;

    for (int i = 0; i < RASTERIZER_MAX_DIRECTIONAL_LIGHTS; i++)
    {
        
        light += max(dot(N, params.directionalLights[i].dir.xyz), 0.0f) * params.directionalLights[i].color.xyz;
    }
    vec3 color = light * params.baseColor.xyz;
    out_fragColor = vec4(color + params.normalFactor.xyz * abs(N), 1.0f);

}
