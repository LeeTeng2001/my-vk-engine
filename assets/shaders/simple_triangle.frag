#version 450
#extension GL_ARB_gpu_shader_int64 : enable

layout (location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout (push_constant) uniform PushConstantData {
    mat4 viewTransform;
    uint64_t time;
} pushC;

void main() {
    // Visualise tex coordinate using raw tex coordinate
//    outColor = vec4(fragTexCoord, 0.0, 1.0);
    // Sample directly
    outColor = texture(texSampler, fragTexCoord);

//    outColor = vec4(fragColor, 1.0);
}

