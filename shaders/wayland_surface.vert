/* XMBShell, a console-like desktop shell
 * Copyright (C) 2025 - JCM
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#version 450

layout(location = 0) out vec2 texCoord;

layout(push_constant) uniform SurfaceParams
{
    mat4 matrix;
    mat4 texture_matrix;
    vec4 color;
    bool is_opaque;
} params;

void main()
{
    // vertex array for a square as a triangle strip
    const vec2 positions[4] = vec2[4](
        vec2( 0.0,  0.0),
        vec2( 2.0,  0.0),
        vec2( 0.0,  2.0),
        vec2( 2.0,  2.0)
    );
    const vec2 texCoords[4] = vec2[4](
        vec2(0, 0),
        vec2(1, 0),
        vec2(0, 1),
        vec2(1, 1)
    );
    vec4 pos = vec4(positions[gl_VertexIndex], 0.0f, 1.0f);
    gl_Position = params.matrix * pos;
    texCoord = (params.texture_matrix * vec4(texCoords[gl_VertexIndex], 0.0f, 1.0f)).xy;
}
