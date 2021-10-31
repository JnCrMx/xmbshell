#version 450
layout(points) in;
layout(max_vertices = 4, triangle_strip) out;

layout(binding = 0, std140) uniform UBO
{
	vec2 position;
	vec2 textureSize;
	float scale;
} uni;

layout(location = 0) in vec2 inPosition[1];
layout(location = 1) in vec2 inTexCoord[1];
layout(location = 2) in vec2 inSize[1];
layout(location = 3) in vec4 inColor[1];

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec4 outColor;

void main()
{
	gl_Position = vec4(inPosition[0]*uni.scale, 0.0, 1.0) - vec4(1.0, 1.0, 0.0, 0.0) + vec4(uni.position, 0.0, 0.0);
	outTexCoord = inTexCoord[0];
	outColor = inColor[0];
	EmitVertex();

	gl_Position = vec4((inPosition[0] + vec2(0, inSize[0].y))*uni.scale, 0.0, 1.0) - vec4(1.0, 1.0, 0.0, 0.0) + vec4(uni.position, 0.0, 0.0);
	outTexCoord = inTexCoord[0] + vec2(0, inSize[0].y);
	outColor = inColor[0];
	EmitVertex();

	gl_Position = vec4((inPosition[0] + vec2(inSize[0].x, 0))*uni.scale, 0.0, 1.0) - vec4(1.0, 1.0, 0.0, 0.0) + vec4(uni.position, 0.0, 0.0);
	outTexCoord = inTexCoord[0] + vec2(inSize[0].x, 0);
	outColor = inColor[0];
	EmitVertex();

	gl_Position = vec4((inPosition[0] + inSize[0])*uni.scale, 0.0, 1.0) - vec4(1.0, 1.0, 0.0, 0.0) + vec4(uni.position, 0.0, 0.0);
	outTexCoord = inTexCoord[0] + inSize[0];
	outColor = inColor[0];
	EmitVertex();
}
