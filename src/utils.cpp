module;

#include <filesystem>
#include <iomanip>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <unordered_map>

#ifdef __GNUG__
#include <cxxabi.h>
#endif

module xmbshell.utils;

import glibmm;
import giomm;
import spdlog;

namespace utils {
    inline std::unordered_map<std::string, std::filesystem::path> themedIconPaths;
    inline std::once_flag themedIconPathsFlag;

    static void initialize_themed_icons(){
        std::unordered_map<std::string, std::filesystem::path> paths;
        auto process = [&paths](const std::string& dir) {
            auto path = std::filesystem::path(dir) / "icons" / "hicolor";
            if(!std::filesystem::exists(path)) {
                return;
            }
            for(const auto& icon : std::filesystem::recursive_directory_iterator(path)) {
                if(icon.is_regular_file()) {
                    auto iconName = icon.path().stem().string();
                    auto iconPath = icon.path();
                    paths[iconName] = iconPath;
                }
            }
        };
        process("/usr/share");
        process("/usr/local/share");
        process(Glib::get_home_dir()+"/.local/share");

        auto xdg_data_dirs = std::getenv("XDG_DATA_DIRS");
        if(xdg_data_dirs) {
            std::istringstream iss(xdg_data_dirs);
            std::string dir;
            while(std::getline(iss, dir, ':')) {
                process(dir);
            }
        }
        spdlog::debug("Found {} themed icons", paths.size());

        themedIconPaths = paths;
    };

    std::optional<std::filesystem::path> resolve_icon(const Gio::Icon* icon) {
        std::call_once(themedIconPathsFlag, initialize_themed_icons);

        if(auto* themed_icon = dynamic_cast<const Gio::ThemedIcon*>(icon)) {
            for(const auto& name : themed_icon->get_names()) {
                if(auto it = themedIconPaths.find(name); it != themedIconPaths.end()) {
                    return it->second;
                }
            }
            spdlog::warn("Themed icon \"{}\" not found", (*themed_icon->get_names().begin()).c_str());
        } else if(auto* file_icon = dynamic_cast<const Gio::FileIcon*>(icon)) {
            return file_icon->get_file()->get_path();
        } else {
            if(icon) {
                auto& r = *icon;
                spdlog::warn("Unsupported icon type: {}", typeid(r).name());
            }
        }
        return std::nullopt;
    }
}

namespace utils
{
    std::string to_fixed_string(double d, int n)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(n) << d;
        return oss.str();
    }
}

#ifdef __GNUG__
namespace utils
{
    std::string demangle(const char *name) {
        int status = -4;
        std::unique_ptr<char, void(*)(void*)> res{
            abi::__cxa_demangle(name, nullptr, nullptr, &status),
            std::free
        };
        return (status==0) ? res.get() : name;
    }
}
#else
namespace utils
{
    std::string demangle(const char *name) {
        return std::string(name);
    }
}
#endif
