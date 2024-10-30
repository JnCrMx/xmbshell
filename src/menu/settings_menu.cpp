module;

#include <array>
#include <chrono>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <sys/wait.h>

module xmbshell.app;

import spdlog;
import glibmm;
import giomm;
import i18n;
import dreamrender;
import xmbshell.config;

import :settings_menu;

import :choice_overlay;
import :message_overlay;
import :progress_overlay;

namespace menu {
    using namespace mfk::i18n::literals;

    class updater : public app::progress_item {
        public:
            updater() {}

            status init(std::string& message) override {
                message = "Updating..."_();
                start_time = std::chrono::system_clock::now();

                int pid = fork();
                if(pid == 0) {
                    std::filesystem::path appimageupdatetool = config::CONFIG.exe_directory / "appimageupdatetool"; 
                    execl(appimageupdatetool.c_str(), "appimageupdatetool", "--overwrite", std::getenv("APPIMAGE"), nullptr);
                    _exit(2);
                } else {
                    this->pid = pid;
                    return status::running;
                }
            }

            status progress(double& progress, std::string& message) override {
                auto now = std::chrono::system_clock::now();
                if(now - start_time < wait_duration) {
                    progress = utils::progress(now, start_time, wait_duration);
                    return status::running;
                }

                int status;
                if(waitpid(pid, &status, WNOHANG) != 0) {
                    int exit_code = WEXITSTATUS(status);
                    if(exit_code == 0) {
                        message = "Update successful. Please restart the application to apply it."_();
                        return status::success;
                    } else {
                        message = "Failed to update."_();
                        return status::error;
                    }
                }
                return status::running;
            }

            bool cancel(std::string& message) override {
                return false;
            }
        private:
            pid_t pid;
            std::chrono::time_point<std::chrono::system_clock> start_time;
            constexpr static auto wait_duration = std::chrono::seconds(2);
    };

    class update_checker : public app::progress_item {
        public:
            update_checker(app::xmbshell* xmb) : xmb(xmb) {}

            status init(std::string& message) override {
                message = "Checking for updates..."_();
                start_time = std::chrono::system_clock::now();

                int pid = fork();
                if(pid == 0) {
                    std::filesystem::path appimageupdatetool = config::CONFIG.exe_directory / "appimageupdatetool"; 
                    execl(appimageupdatetool.c_str(), "appimageupdatetool", "--check-for-update", std::getenv("APPIMAGE"), nullptr);
                    _exit(2);
                } else {
                    this->pid = pid;
                    return status::running;
                }
            }
            status progress(double& progress, std::string& message) override {
                auto now = std::chrono::system_clock::now();
                if(now - start_time < wait_duration) {
                    progress = utils::progress(now, start_time, wait_duration);
                    return status::running;
                }

                int status;
                if(waitpid(pid, &status, WNOHANG) != 0) {
                    int exit_code = WEXITSTATUS(status);
                    if(exit_code == 0) {
                        message = "No updates available."_();
                        return status::success;
                    } else if(exit_code == 1) {
                        message = {};
                        xmb->set_message_overlay(app::message_overlay{"Update Available"_(), "An update is available. Would you like to install it?"_(),
                            {"Yes"_(), "No"_()}, [xmb = this->xmb](unsigned int choice){
                                if(choice == 0) {
                                    xmb->set_progress_overlay(app::progress_overlay{"Updating"_(), std::make_unique<updater>()});
                                }
                            }
                        });
                        return status::success;
                    } else {
                        message = "Failed to check for updates."_();
                        return status::error;
                    }
                }
                return status::running;
            }
            bool cancel(std::string& message) override {
                return false;
            }
        private:
            pid_t pid;
            std::chrono::time_point<std::chrono::system_clock> start_time;
            constexpr static auto wait_duration = std::chrono::milliseconds(500);
            app::xmbshell* xmb;
    };

    std::unique_ptr<action_menu_entry> entry_base(dreamrender::resource_loader& loader,
        const std::string& name, const std::string& key,
        std::function<result()> callback)
    {
        std::string filename = std::format("icon_settings_{}.png", key);
        return make_simple<action_menu_entry>(name, config::CONFIG.asset_directory/"icons"/filename, loader, callback);
    }

    std::unique_ptr<action_menu_entry> entry_bool(dreamrender::resource_loader& loader, app::xmbshell* xmb,
        const std::string& name, const std::string& schema, const std::string& key)
    {
        return entry_base(loader, name, key, [xmb, key, schema](){
            auto settings = Gio::Settings::create(schema);
            bool value = settings->get_boolean(key);
            xmb->set_choice_overlay(app::choice_overlay{
                std::vector<std::string>{"Off"_(), "On"_()}, value ? 1u : 0u,
                [settings, schema, key](unsigned int choice) {
                    settings->set_boolean(key, choice == 1);
                    settings->apply();
                }
            });
            return result::success;
        });
    }

    std::unique_ptr<action_menu_entry> entry_int(dreamrender::resource_loader& loader, app::xmbshell* xmb,
        const std::string& name, const std::string& schema, const std::string& key, int min, int max, int step = 1)
    {
        return entry_base(loader, name, key, [xmb, key, schema, min, max, step](){
            auto settings = Gio::Settings::create(schema);
            int value = settings->get_int(key);
            std::vector<std::string> choices;
            for(int i = min; i <= max; i += step) {
                choices.push_back(std::to_string(i));
            }
            int current_choice = static_cast<unsigned int>((value - min)/step);
            if(current_choice < 0 || current_choice >= choices.size()) {
                current_choice = 0;
            }
            xmb->set_choice_overlay(app::choice_overlay{
                choices, static_cast<unsigned int>(current_choice),
                [settings, schema, key, min, step](unsigned int choice) {
                    int value = choice*step + min;
                    settings->set_int(key, value);
                    settings->apply();
                }
            });
            return result::success;
        });
    }
    std::unique_ptr<action_menu_entry> entry_int(dreamrender::resource_loader& loader, app::xmbshell* xmb,
        const std::string& name, const std::string& schema, const std::string& key, std::ranges::range auto values)
        requires std::is_integral_v<std::ranges::range_value_t<decltype(values)>>
    {
        return entry_base(loader, name, key, [xmb, key, schema, values](){
            auto settings = Gio::Settings::create(schema);
            int value = settings->get_int(key);
            std::vector<std::string> choices;
            for(auto v : values) {
                choices.push_back(std::to_string(v));
            }
            unsigned int current_choice = std::find(values.begin(), values.end(), value) - values.begin();
            xmb->set_choice_overlay(app::choice_overlay{
                choices, current_choice,
                [settings, schema, key, values](unsigned int choice) {
                    int value = values[choice];
                    settings->set_int(key, value);
                    settings->apply();
                }
            });
            return result::success;
        });
    }

    settings_menu::settings_menu(const std::string& name, dreamrender::texture&& icon, app::xmbshell* xmb, dreamrender::resource_loader& loader) : simple_menu(name, std::move(icon)) {
        entries.push_back(make_simple<simple_menu>("Video Settings"_(), config::CONFIG.asset_directory/"icons/icon_settings_video.png", loader,
            std::array{
                entry_bool(loader, xmb, "VSync"_(), "re.jcm.xmbos.xmbshell.render", "vsync"),
                entry_int(loader, xmb, "Sample Count"_(), "re.jcm.xmbos.xmbshell.render", "sample-count", std::array{1, 2, 4, 8, 16}),
                entry_int(loader, xmb, "Max FPS"_(), "re.jcm.xmbos.xmbshell.render", "max-fps", 15, 200, 5),
            }
        ));
        entries.push_back(make_simple<simple_menu>("Debug Settings"_(), config::CONFIG.asset_directory/"icons/icon_settings_debug.png", loader,
            std::array{
                entry_bool(loader, xmb, "Show FPS"_(), "re.jcm.xmbos.xmbshell.render", "show-fps"),
                entry_bool(loader, xmb, "Show Memory Usage"_(), "re.jcm.xmbos.xmbshell.render", "show-mem"),
#ifndef NDEBUG
                make_simple<action_menu_entry>("Toggle Background Blur"_(), config::CONFIG.asset_directory/"icons/icon_settings_toggle-background-blur.png", loader, [xmb](){
                    spdlog::info("Toggling background blur");
                    xmb->set_blur_background(!xmb->get_blur_background());
                    return result::success;
                }),
                make_simple<action_menu_entry>("Toggle Ingame Mode"_(), config::CONFIG.asset_directory/"icons/icon_settings-toggle-ingame-mode.png", loader, [xmb](){
                    spdlog::info("Toggling ingame mode");
                    xmb->set_ingame_mode(!xmb->get_ingame_mode());
                    return result::success;
                }),
                make_simple<action_menu_entry>("Test Choice Overlay"_(), config::CONFIG.asset_directory/"icons/icon_settings-toggle-ingame-mode.png", loader, [xmb](){
                    spdlog::info("Testing choice overlay");
                    if(xmb->get_choice_overlay()) {
                        xmb->set_choice_overlay(std::nullopt);
                    } else {
                        xmb->set_choice_overlay(app::choice_overlay{std::vector<std::string>{{"Hello"}, {"World"}}});
                    }
                    return result::success;
                }),
#endif
            }
        ));
        if(std::getenv("APPIMAGE")) {
            entries.push_back(make_simple<action_menu_entry>("Check for Updates"_(), config::CONFIG.asset_directory/"icons/icon_settings_update.png", loader, [xmb](){
                spdlog::info("Update request from XMB");
                xmb->set_progress_overlay(app::progress_overlay{"System Update"_(), std::make_unique<update_checker>(xmb)});
                return result::unsupported;
            }));
        }
        entries.push_back(make_simple<action_menu_entry>("Report bug"_(), config::CONFIG.asset_directory/"icons/icon_bug.png", loader, [](){
            spdlog::info("Bug report request from XMB");
            return result::unsupported;
        }));
        entries.push_back(make_simple<action_menu_entry>("Reset"_(), config::CONFIG.asset_directory/"icons/icon_settings_reset.png", loader, [](){
            spdlog::info("Settings reset request from XMB");
            Glib::RefPtr<Gio::Settings> shellSettings =
                Gio::Settings::create("re.jcm.xmbos.xmbshell");
            auto source = Gio::SettingsSchemaSource::get_default();
            if(!source) {
                spdlog::error("Failed to get default settings schema source");
                return result::failure;
            }
            auto schema = source->lookup("re.jcm.xmbos.xmbshell", true);
            if(!schema) {
                spdlog::error("Failed to find schema for re.jcm.xmbos.xmbshell");
                return result::failure;
            }
            for(auto key : schema->list_keys()) {
                shellSettings->reset(key);
            }
            config::CONFIG.load();
            return result::success;
        }));
    }
}
