#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) flat in int matIndex;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;
layout(location = 4) in vec2 fragTexCoord;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 modelIT;
    mat4 viewProjInverse;
    float time;
} ubo;

struct Material
{
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    vec3  transmittance;
    vec3  emission;
    float shininess;
    float metallic;
    float ior;
    float dissolve;
    int   illum;
    int   diffuseTextureId;
    int   specularTextureId;
    int   normalTextureId;
    float pad;
};

const int sizeofMat = 6;

layout(binding = 2) buffer matBufferObject
{
    vec4[] m;
} materials;

layout(binding = 1) uniform sampler2D[] textureSamplers;

Material unpackMaterial()
{
    Material m;
    vec4     d0 = materials.m[sizeofMat * matIndex + 0];
    vec4     d1 = materials.m[sizeofMat * matIndex + 1];
    vec4     d2 = materials.m[sizeofMat * matIndex + 2];
    vec4     d3 = materials.m[sizeofMat * matIndex + 3];
    vec4     d4 = materials.m[sizeofMat * matIndex + 4];
    vec4     d5 = materials.m[sizeofMat * matIndex + 5];

    m.ambient           = vec3(d0.x, d0.y, d0.z);
    m.diffuse           = vec3(d0.w, d1.x, d1.y);
    m.specular          = vec3(d1.z, d1.w, d2.x);
    m.transmittance     = vec3(d2.y, d2.z, d2.w);
    m.emission          = vec3(d3.x, d3.y, d3.z);
    m.shininess         = d3.w;
    m.metallic          = d4.x;
    m.ior               = d4.y;
    m.dissolve          = d4.z;
    m.illum             = int(d4.w);
    m.diffuseTextureId  = floatBitsToInt(d5.x);
    m.specularTextureId = floatBitsToInt(d5.y);
    m.normalTextureId   = floatBitsToInt(d5.z);

    return m;
}

void main()
{
    Material mat = unpackMaterial();

    const vec3 lightPos   = vec3(0.0, 10.0, 0.0);
    float r = (cos(3*ubo.time) + 2) * 0.5;
    float g = (sin(1*ubo.time) + 2) * 0.5;
    float b = (sin(2*ubo.time) + 2) * 0.5;
    const vec3 lightColor = vec3(r, g, b);
    //
    vec3  lightDir = normalize(lightPos - fragPos);
    float cosTheta = max(dot(fragNormal, lightDir), 0.1);

    vec3 color = mat.diffuse;
    if(mat.diffuseTextureId >= 0)
    {
        // color = texture(textureSamplers, fragTexCoord).xyz;
        color *= texture(textureSamplers[mat.diffuseTextureId], fragTexCoord).xyz;
    }
    vec3 result = lightColor * color * cosTheta;// * fragColor;
    outColor    = vec4(result, 1.0);
    // outColor = vec4(fragTexCoord, 1.0, 1.0);
}
