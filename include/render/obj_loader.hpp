#pragma once

#include <istream>
#include <vector>
#include <array>

#include <glm/glm.hpp>

#include "render/model.hpp"

namespace render
{
	void load_obj(std::istream& in, std::vector<vertex_data>& vertices, std::vector<uint32_t>& indices);
}
