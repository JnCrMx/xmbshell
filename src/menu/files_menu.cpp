module;

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#include <unistd.h>

module xmbshell.app;

import :files_menu;
import :menu_base;
import :menu_utils;
import :message_overlay;

import xmbshell.config;
import dreamrender;
import glibmm;
import giomm;
import sdl2;
import spdlog;
import i18n;

namespace menu {
    using namespace std::string_view_literals;
    constexpr auto supported_image_extensions = {
        ".avi"sv, ".avif"sv, ".bmp"sv, ".cur"sv, ".gif"sv, ".ico"sv, ".jpg"sv, ".jpeg"sv, ".jxl"sv,
        ".lbm"sv, ".pcx"sv, ".png"sv, ".pnm"sv, ".qoi"sv, ".svg"sv, ".tga"sv, ".tiff"sv, ".webp"sv,
        ".xcf"sv, ".xpm"sv, ".xv"sv
    };

    using namespace mfk::i18n::literals;

    files_menu::files_menu(const std::string& name, dreamrender::texture&& icon, app::xmbshell* xmb, const std::filesystem::path& path, dreamrender::resource_loader& loader)
    : simple_menu(name, std::move(icon)), xmb(xmb), path(path), loader(loader)
    {

    }

    void files_menu::on_open() {
        simple_menu::on_open();

        if(!std::filesystem::exists(path)) {
            spdlog::error("Path does not exist: {}", path.string());
            return;
        }

        std::filesystem::directory_iterator it{path};
        for (const auto& entry : it) {
            if (entry.is_directory()) {
                auto menu = make_simple<files_menu>(entry.path().filename().string(), "folder.png", loader, xmb, entry.path(), loader);
                entries.push_back(std::move(menu));
            }
            else if (entry.is_regular_file()) {
                std::filesystem::path icon_file_path = config::CONFIG.asset_directory/"icons/file.png";
                if(std::find(supported_image_extensions.begin(), supported_image_extensions.end(), entry.path().extension().string()) != supported_image_extensions.end()) {
                    icon_file_path = entry.path();
                }

                auto menu = make_simple<action_menu_entry>(entry.path().filename().string(), icon_file_path, loader, [](){
                    return result::unsupported;
                });
                entries.push_back(std::move(menu));
            } else {
                spdlog::warn("Unsupported file type: {}", entry.path().string());
            }
        }
    }
}
