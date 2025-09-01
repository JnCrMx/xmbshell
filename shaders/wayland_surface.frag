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

layout(binding = 0) uniform sampler2D tex;

layout(location = 0) in vec2 inTexCoord;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform SurfaceParams
{
    mat4 matrix;
    vec4 color;
    bool is_opaque;
} params;

void main()
{
	outColor = texture(tex, inTexCoord) * params.color;
    outColor.a = params.is_opaque ? 1.0 : outColor.a;
}
