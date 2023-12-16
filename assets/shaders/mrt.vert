#version 450
#extension GL_ARB_gpu_shader_int64 : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 outFragColor;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec3 outNormal;

layout (push_constant) uniform PushConstantData {
    mat4 viewModalTransform;
    mat4 rotationTransform;
} pushC;

void main() {
    // TODO: transform normal
    // Very simple shader, assuming the position of the sun is coming from camera
    gl_Position = pushC.viewModalTransform * vec4(inPosition, 1.0);
//    outFragColor = vec3(1, 1, 1) * dot(normalize(pushC.sunPos), inNormal);
    outFragColor = vec3(1, 1, 1);
    outNormal = (pushC.rotationTransform * vec4(inNormal, 1.0)).xyz;
    outTexCoord = inTexCoord;
}
