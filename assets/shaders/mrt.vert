#version 450
#extension GL_ARB_gpu_shader_int64 : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

layout(location = 0) out vec3 outVertPos;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outColor;
layout(location = 4) out mat3 outTBNMat;

layout (push_constant) uniform PushConstantData {
    mat4 viewModalTransform;
    mat4 perspectiveTransform;
} pushC;

void main() {
    // modal view perspective
    gl_Position = pushC.perspectiveTransform * pushC.viewModalTransform * vec4(inPosition, 1.0);
    outVertPos = inPosition; // world position
    outTexCoord = inTexCoord;
    // transforming normal https://www.scratchapixel.com/lessons/mathematics-physics-for-computer-graphics/geometry/transforming-normals.html
    // https://stackoverflow.com/questions/13654401/why-transform-normals-with-the-transpose-of-the-inverse-of-the-modelview-matrix
    mat3 normMatrix = transpose(inverse(mat3(pushC.viewModalTransform)));
    outNormal = normalize(normMatrix * inNormal);
    outTBNMat = mat3(normalize((pushC.viewModalTransform * vec4(inTangent, 1)).xyz),
                    normalize((pushC.viewModalTransform * vec4(inBitangent, 1)).xyz), outNormal); // inverse of TBN
}
