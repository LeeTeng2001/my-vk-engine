#version 450
#extension GL_ARB_gpu_shader_int64 : enable

layout(location = 0) in vec3 inVertPos;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outVertPos;

layout(set = 0, binding = 0) uniform sampler2D texColorSampler;

void main() {
    outColor = texture(texColorSampler, inTexCoord);
//    outNormal = vec4(inNormal * 0.5 + 0.5, 1);// transforms from [-1,1] to [0,1]
    outNormal = vec4(inNormal, 1);
    outVertPos = vec4(inVertPos, 1);
}

