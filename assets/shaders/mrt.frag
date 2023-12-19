#version 450
#extension GL_ARB_gpu_shader_int64 : enable

layout(location = 0) in vec3 inVertPos;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outVertPos;

layout(set = 0, binding = 0) uniform UBO {
    int textureToggle;
} ubo;
layout(set = 0, binding = 1) uniform sampler2D texColorSampler;
layout(set = 0, binding = 2) uniform sampler2D normalColorSampler;

layout (push_constant) uniform PushConstantData {
    mat4 viewModalTransform;
    mat4 rotationTransform;
} pushC;

void main() {
    if ((ubo.textureToggle & 1) == 1) {
        outColor = texture(texColorSampler, inTexCoord);
    } else {
        outColor = vec4(inColor, 1);
    }
    if (((ubo.textureToggle >> 1) & 1) == 1) {
        outNormal = texture(normalColorSampler, inTexCoord) * 2 - 1;// transforms from [0,1] to [-1,1]
    } else {
        outNormal = vec4(inNormal, 1);
    }
    outVertPos = vec4(inVertPos, 1);
}

