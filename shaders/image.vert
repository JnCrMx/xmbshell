#version 450

layout(location = 0) out vec2 texCoord;

layout(push_constant) uniform ImageParams
{
    mat4 matrix;
    uint index;
} params;

void main()
{
    // vertex array for a square as two triangles
    const vec2 positions[6] = vec2[6](
        vec2( 0.0,  0.0),
        vec2( 2.0,  0.0),
        vec2( 0.0,  2.0),
        vec2( 2.0,  0.0),
        vec2( 0.0,  2.0),
        vec2( 2.0,  2.0)
    );
    const vec2 texCoords[6] = vec2[6](
        vec2(0, 0),
        vec2(1, 0),
        vec2(0, 1),
        vec2(1, 0),
        vec2(0, 1),
        vec2(1, 1)
    );
    vec4 pos = vec4(positions[gl_VertexIndex], 0.0f, 1.0f);
    gl_Position = params.matrix * pos;
    texCoord = texCoords[gl_VertexIndex];
}
