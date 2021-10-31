#include "render/obj_loader.hpp"

#include <sstream>
#include <array>
#include <tuple>

namespace render
{
	void load_obj(std::istream &in, std::vector<vertex_data> &vertices, std::vector<uint32_t> &indices)
	{
		std::vector<glm::vec3> positions;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec2> texCoords;
		std::vector<std::tuple<int, int, int>> cindices;

		std::string line;
		while(std::getline(in, line))
		{
			std::istringstream is(line);
			std::string type;
			is >> type;
			if(type=="v")
			{
				float x, y, z;
				is >> x >> y >> z;
				positions.push_back({x, y, z});
			}
			if(type=="vt")
			{
				float u, v;
				is >> u >> v;
				texCoords.push_back({u, -v});
			}
			if(type=="vn")
			{
				float x, y, z;
				is >> x >> y >> z;
				normals.push_back({x, y, z});
			}
			if(type=="f")
			{
				std::array<std::string, 3> args;
				is >> args[0] >> args[1] >> args[2];
				for(int i=0; i<3; i++)
				{
					std::stringstream s(args[i]);
					int vertex, uv, normal;

					s >> vertex;
					s.ignore(1);
					s >> uv;
					s.ignore(1);
					s >> normal;

					cindices.push_back({vertex-1, uv-1, normal-1});
				}
			}
		}

		std::vector<std::tuple<int, int, int>> indexCombos;
		for(int i=0; i<cindices.size(); i++)
		{
			auto index = cindices[i];
			auto p = std::find(indexCombos.begin(), indexCombos.end(), index);
			if(p == indexCombos.end())
			{
				indices.push_back(vertices.size());
				auto [pos, tex, nor] = index;
				vertices.push_back({positions[pos], normals[nor], texCoords[tex]});
				indexCombos.push_back(index);
			}
			else
			{
				indices.push_back(std::distance(indexCombos.begin(), p));
			}
		}
	}
}