module;

#include <algorithm>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

#include <unistd.h>

module xmbshell.app;

import :files_menu;
import :menu_base;
import :menu_utils;
import :message_overlay;
import :choice_overlay;

import xmbshell.config;
import xmbshell.utils;
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

    files_menu::files_menu(std::string name, dreamrender::texture&& icon, app::xmbshell* xmb, std::filesystem::path path, dreamrender::resource_loader& loader)
    : simple_menu(std::move(name), std::move(icon)), xmb(xmb), path(std::move(path)), loader(loader)
    {

    }

    void files_menu::reload() {
        std::filesystem::directory_iterator it{path};

        std::unordered_set<std::filesystem::path> all_files;
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
            std::string content_type = info->get_attribute_string("standard::fast-content-type");
            bool thumbnail_is_valid = info->get_attribute_boolean("thumbnail::is-valid");
            std::string thumbnail_path = info->get_attribute_as_string("thumbnail::path");

            if(!filter(*info.get())) {
                continue;
            }

            // Skip entries we already have (TODO: update icon if needed)
            if(auto it = std::ranges::find_if(extra_data_entries, [&entry](const auto& e) {
                return e.path == entry.path();
            }); it != extra_data_entries.end()) {
                it->file = file;
                it->info = info;
                all_files.insert(entry.path());
                continue;
            }

            std::string content_type_key = content_type;
            std::ranges::replace(content_type_key, '/', '_');

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
                auto menu = make_simple<action_menu_entry>(entry.path().filename().string(), icon_file_path, loader,
                    std::function<result()>{}, [this, entry, file, info](auto && PH1) {
                        return activate_file(entry.path(), std::move(file), std::move(info), std::forward<decltype(PH1)>(PH1));
                    });
                entries.push_back(std::move(menu));
            } else {
                spdlog::warn("Unsupported file type: {}", entry.path().string());
                continue;
            }
            extra_data_entries.emplace_back(entry.path(), file, info);
            all_files.insert(entry.path());
        }

        for(auto it = entries.begin(); it != entries.end();) {
            auto dist = std::distance(entries.begin(), it);
            if(!all_files.contains(extra_data_entries[dist].path)) {
                it = entries.erase(it);
                extra_data_entries.erase(extra_data_entries.begin() + dist);

                if(selected_submenu == dist) {
                    selected_submenu = 0;
                } else if(selected_submenu > dist) {
                    --selected_submenu;
                }
            } else {
                ++it;
            }
        }

        resort();
    }

    void files_menu::resort() {
        std::vector<unsigned int> indices(entries.size());
        std::ranges::iota(indices, 0);

        std::ranges::sort(indices, [this](unsigned int a, unsigned int b) {
            const auto& a_entry = *extra_data_entries[a].info.get();
            const auto& b_entry = *extra_data_entries[b].info.get();
            return sort(a_entry, b_entry) ^ sort_descending;
        });

        decltype(entries) old_entries = std::move(entries);
        decltype(extra_data_entries) old_extra_data_entries = std::move(extra_data_entries);
        entries.clear();
        extra_data_entries.clear();
        for(auto i : indices) {
            entries.push_back(std::move(old_entries[i]));
            extra_data_entries.push_back(std::move(old_extra_data_entries[i]));
        }
        selected_submenu = std::ranges::find(indices, selected_submenu) - indices.begin();
    }

    void files_menu::on_open() {
        simple_menu::on_open();

        if(!std::filesystem::exists(path)) {
            spdlog::error("Path does not exist: {}", path.string());
            return;
        }

        reload();
        selected_submenu = old_selected_submenu;
        if(selected_submenu >= entries.size()) {
            selected_submenu = 0;
        }
    }

    bool copy_file(app::xmbshell* xmb, const std::filesystem::path& src, const std::filesystem::path& dst);
    bool cut_file(app::xmbshell* xmb, const std::filesystem::path& src, const std::filesystem::path& dst);

    result files_menu::activate_file(const std::filesystem::path& path,
            Glib::RefPtr<Gio::File> file,
            Glib::RefPtr<Gio::FileInfo> info,
            action action)
    {
        if(action == action::options) {
            std::vector options{
            //  0        , 1                   , 2                    , 3        , 4       , 5
                "Open"_(), "Open externally"_(), "View information"_(), "Copy"_(), "Cut"_(), "Delete"_(), "Refresh"_()
            };
            if(const auto& cb = xmb->get_clipboard()) {
                if(std::holds_alternative<std::function<bool(std::filesystem::path)>>(*cb)) {
                    //                7
                    options.push_back("Paste"_());
                }
            }
            xmb->set_choice_overlay(app::choice_overlay{options, 0, [this, path](unsigned int index){
                switch(index) {
                    case 3:
                        xmb->set_clipboard([xmb = this->xmb, path](std::filesystem::path dst){
                            return copy_file(xmb, path, dst);
                        });
                        break;
                    case 4:
                        xmb->set_clipboard([xmb = this->xmb, path](std::filesystem::path dst){
                            return cut_file(xmb, path, dst);
                        });
                        break;
                    case 6:
                        reload();
                        break;
                    case 7:
                        if(const auto& cb = xmb->get_clipboard()) {
                            if(auto f = std::get_if<std::function<bool(std::filesystem::path)>>(&cb.value())) {
                                if((*f)(path)) {
                                    reload();
                                }
                            }
                        }
                        break;
                    default:
                        break;
                }
            }});
            return result::success;
        }
        return result::unsupported;
    }

    bool copy_file(app::xmbshell* xmb, const std::filesystem::path& src, const std::filesystem::path& dst) {
        std::filesystem::path p = dst;
        if(!std::filesystem::is_directory(p)) {
            p = p.parent_path();
        }
        p /= src.filename();

        if(std::filesystem::exists(p)) {
            spdlog::error("File already exists: {}", p.string());
            return false;
        }

        try {
            std::filesystem::copy_file(src, p, std::filesystem::copy_options::overwrite_existing);
            return true;
        } catch(const std::exception& e) {
            spdlog::error("Failed to copy file: {}", e.what());

            std::string message = e.what();
            xmb->set_message_overlay(app::message_overlay{
                "Copy failed"_(), "Failed to copy file: {}"_(message), {"OK"_()}
            });
        }
        return false;
    }

    bool cut_file(app::xmbshell* xmb, const std::filesystem::path& src, const std::filesystem::path& dst) {
        std::filesystem::path p = dst;
        if(!std::filesystem::is_directory(p)) {
            p = p.parent_path();
        }
        p /= src.filename();

        if(std::filesystem::exists(p)) {
            spdlog::error("File already exists: {}", p.string());
            return false;
        }

        try {
            std::filesystem::rename(src, p);
            return true;
        } catch(const std::exception& e) {
            spdlog::error("Failed to copy file: {}", e.what());

            std::string message = e.what();
            xmb->set_message_overlay(app::message_overlay{
                "Move failed"_(), "Failed to move file: {}"_(message), {"OK"_()}
            });
        }
        return false;
    }

    void files_menu::get_button_actions(std::vector<std::pair<action, std::string>>& v) {
        if(!v.empty()) {
            return;
        }
        v.emplace_back(action::none, "");
        v.emplace_back(action::none, "");
        v.emplace_back(action::options, "Options"_());
        v.emplace_back(action::extra, "Sort and Filter"_());
    }
}
