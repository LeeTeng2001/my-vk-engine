#version 450
#extension GL_ARB_gpu_shader_int64 : enable

layout(location = 0) in vec3 inVertPos;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in mat3 inTBNMat; // quite expensive, can be optimised

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outVertPos;

layout(set = 0, binding = 0) uniform UBO {
    vec4 diffuse;
    vec4 emissive;
    int textureToggle;
} ubo;
layout(set = 0, binding = 1) uniform sampler2D texColorSampler;
layout(set = 0, binding = 2) uniform sampler2D normalColorSampler;
layout(set = 0, binding = 3) uniform sampler2D aoRoughnessHeightSampler;

void main() {
    // color
    if ((ubo.textureToggle & 1) == 1) {
        outColor = texture(texColorSampler, inTexCoord);
    } else {
        outColor = vec4(ubo.diffuse.rgb, 1);
    }
    // normal
    if (((ubo.textureToggle >> 1) & 1) == 1) {
        // Normal map in tangent space (pointing out +z from surface, base axis symbol same as our world coordinate)
        vec4 normaSamp = texture(normalColorSampler, inTexCoord) * 2 - 1; // transforms from [0,1] to [-1,1]
        outNormal = vec4(inTBNMat * normaSamp.xyz, 1);
    } else {
        outNormal = vec4(inNormal, 1); // already transformed
    }
    outVertPos = vec4(inVertPos, 1);
    // UV
    outNormal.a = inTexCoord.r;
    outVertPos.a = inTexCoord.g;
    // TODO: ao, roughness, height
}

