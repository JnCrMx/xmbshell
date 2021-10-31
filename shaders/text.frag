#version 450

layout(binding = 0, std140) uniform UBO
{
	vec2 position;
	vec2 textureSize;
	float scale;
} uni;
layout(binding = 1) uniform sampler2D tex;

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main()
{
	vec4 c = texture(tex, inTexCoord/uni.textureSize);
	outColor = inColor*c;
}
