module;

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

export module xmbshell.app:programs;

import :component;
import giomm;

namespace programs {

export struct open_info {
    std::string name;
    bool is_external;
    std::function<std::unique_ptr<app::component>(std::filesystem::path, dreamrender::resource_loader&)> create;
};

class program_registry {
    static std::unordered_multimap<std::string, open_info> programs;

    protected:
        friend std::vector<open_info> get_open_infos(const std::filesystem::path& path, const Gio::FileInfo& info);

        void do_register_program(std::string name, std::string mime_type, open_info info) {
            programs.emplace(mime_type, info);
        }
        static std::vector<open_info> get_program(std::string mime_type) {
            std::vector<open_info> infos;
            auto range = programs.equal_range(mime_type);
            for(auto it = range.first; it != range.second; ++it) {
                infos.push_back(it->second);
            }
            return infos;
        }
};
std::unordered_multimap<std::string, open_info> program_registry::programs{};

export template<typename T>
struct register_program : program_registry {
    register_program(std::string name, std::initializer_list<std::string> mime_types) {
        for(auto& mime_type : mime_types) {
            do_register_program(name, mime_type, {name, false, [](std::filesystem::path path, dreamrender::resource_loader& loader) {
                return std::make_unique<T>(path, loader);
            }});
        }
    }
};

export std::vector<open_info> get_open_infos(const std::filesystem::path& path, const Gio::FileInfo& info) {
    return program_registry::get_program(info.get_attribute_string("standard::fast-content-type"));
}

}
