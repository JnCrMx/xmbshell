#version 450

layout(binding = 0) uniform sampler2D tex[512];

layout(location = 0) in vec2 inTexCoord;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform ImageParams
{
    mat4 matrix;
    vec4 color;
    uint index;
} params;

void main()
{
	outColor = texture(tex[params.index], inTexCoord) * params.color;
}
