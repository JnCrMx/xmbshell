#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inSize;
layout(location = 3) in vec4 inColor;

layout(location = 0) out vec2 outPosition;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec2 outSize;
layout(location = 3) out vec4 outColor;

void main()
{
	outPosition = inPosition;
	outTexCoord = inTexCoord;
	outSize = inSize;
	outColor = inColor;

	gl_Position = vec4(outPosition, 0.0, 1.0);
}
