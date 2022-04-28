#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;
layout(location = 4) in int inMaterialIdx;

layout(location = 0) flat out int matIndex;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragPos;
layout(location = 4) out vec2 fragTexcoord;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 modelIT;
    mat4 viewProjInverse;
    float time;
}
ubo;

layout(push_constant) uniform PushConstants
{
    vec4 position;
}
pushConsts;

mat4 rotationMatrix(vec3 axis, float angle)
{
    vec3 a = normalize(axis);

    float s = sin(angle);
    float c = cos(angle);

    float oc = 1.0 - c;

    float sx = s * a.x;
    float sy = s * a.y;
    float sz = s * a.z;

    float ocx = oc * a.x;
    float ocy = oc * a.y;
    float ocz = oc * a.z;

    float ocxx = ocx * a.x;
    float ocxy = ocx * a.y;
    float ocxz = ocx * a.z;
    float ocyy = ocy * a.y;
    float ocyz = ocy * a.z;
    float oczz = ocz * a.z;

    return mat4(
            vec4(ocxx + c, ocxy - sz, ocxz + sy, 0.0),
            vec4(ocxy + sz, ocyy + c, ocyz - sx, 0.0),
            vec4(ocxz - sy, ocyz + sx, oczz + c, 0.0),
            vec4(0.0, 0.0, 0.0, 1.0));
}

void main()
{
    fragColor = inColor;

    fragPos = vec3(ubo.model * vec4(inPosition, 1.0)).xyz;

    vec4 posOffset = pushConsts.position;

    matIndex = inMaterialIdx;
    fragNormal = normalize(vec3(ubo.modelIT * vec4(inNormal, 0.0)));
    fragTexcoord = inTexCoord;
    gl_Position = ubo.proj * ubo.view * ubo.model * rotationMatrix(vec3(0,0,1), 3 * ubo.time)
                  * vec4(inPosition + posOffset.xyz, 1.0);
}
