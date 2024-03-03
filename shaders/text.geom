#version 450
layout(points) in;
layout(max_vertices = 4, triangle_strip) out;

layout(binding = 0, std140) uniform UBO
{
	mat4 matrix;
	vec2 textureSize;
} uni;

layout(location = 0) in vec2 inPosition[1];
layout(location = 1) in vec2 inTexCoord[1];
layout(location = 2) in vec2 inSize[1];
layout(location = 3) in vec4 inColor[1];

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec4 outColor;

void main()
{
	const vec2 offsets[4] = vec2[4](
		vec2(0, 0),
		vec2(0, 1),
		vec2(1, 0),
		vec2(1, 1)
	);

	for (int i = 0; i < 4; i++)
	{
		gl_Position = uni.matrix * vec4(inPosition[0] + offsets[i] * inSize[0], 0.0, 1.0);
		outTexCoord = inTexCoord[0] + offsets[i] * inSize[0];
		outColor = inColor[0];
		EmitVertex();
	}
}
