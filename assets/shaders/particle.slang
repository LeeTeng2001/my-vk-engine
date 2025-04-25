// struct Particle {
//   vec2 position;
//   vec2 velocity;
//   vec4 color;
// };

// layout(std140, binding = 1) readonly buffer ParticleSSBOIn {
//    Particle particlesIn[ ];
// };

// layout(std140, binding = 2) buffer ParticleSSBOOut {
//    Particle particlesOut[ ];
// };

// void main() {
//     particlesOut[index].position = particlesIn[index].position + particlesIn[index].velocity.xy * ubo.deltaTime;
// }

StructuredBuffer<float> buffer0;
StructuredBuffer<float> buffer1;
RWStructuredBuffer<float> result;

[shader("compute")]
[numthreads(1,1,1)]
void main(uint3 threadId : SV_DispatchThreadID)
{
    uint index = threadId.x;
    result[index] = buffer0[index] + buffer1[index];
}
