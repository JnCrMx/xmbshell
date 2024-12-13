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
            auto file = Gio::File::create_for_path(entry.path().string());
            auto info = file->query_info(
                "standard::fast-content-type"   ","
                "standard::display-name"        ","
                "standard::symbolic-icon"       ","
                "standard::icon"                ","
                "standard::is-hidden"           ","
                "standard::is-backup"           ","
                "thumbnail::path"               ","
                "thumbnail::is-valid");
            std::string display_name = info->get_display_name();
            std::string content_type = info->get_attribute_string("standard::content-type");
            bool is_hidden = info->get_attribute_boolean("standard::is-hidden");
            bool is_backup = info->get_attribute_boolean("standard::is-backup");
            bool thumbnail_is_valid = info->get_attribute_boolean("thumbnail::is-valid");
            std::string thumbnail_path = info->get_attribute_as_string("thumbnail::path");

            if(is_hidden || is_backup) {
                continue;
            }

            std::string content_type_key = content_type;
            std::replace(content_type_key.begin(), content_type_key.end(), '/', '_');

            std::filesystem::path icon_file_path = config::CONFIG.asset_directory/
                "icons"/("icon_files_type_"+content_type_key+".png");
            if(thumbnail_is_valid) {
                icon_file_path = thumbnail_path;
            } else if(auto r = utils::resolve_icon(info->get_symbolic_icon().get())) {
                icon_file_path = *r;
            } else if(auto r = utils::resolve_icon(info->get_icon().get())) {
                icon_file_path = *r;
            }

            if(!std::filesystem::exists(icon_file_path)) {
                spdlog::warn("Icon file does not exist: {}", icon_file_path.string());
                icon_file_path = config::CONFIG.asset_directory/
                    "icons"/(entry.is_directory() ? "icon_files_folder.png" : "icon_files_file.png");
            }

            if(entry.is_directory()) {
                auto menu = make_simple<files_menu>(entry.path().filename().string(), icon_file_path, loader, xmb, entry.path(), loader);
                entries.push_back(std::move(menu));
            }
            else if (entry.is_regular_file()) {
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
