#version 450

layout(location = 0) out vec2 texCoord;

void main()
{
    // vertex array for a square as a triangle strip
    const vec2 positions[4] = vec2[4](
        vec2( -1.0,  -1.0),
        vec2( 1.0,  -1.0),
        vec2( -1.0,  1.0),
        vec2( 1.0,  1.0)
    );
    const vec2 texCoords[4] = vec2[4](
        vec2(0, 0),
        vec2(1, 0),
        vec2(0, 1),
        vec2(1, 1)
    );
    vec4 pos = vec4(positions[gl_VertexIndex], 0.0f, 1.0f);
    gl_Position = pos;
    texCoord = texCoords[gl_VertexIndex];
}
