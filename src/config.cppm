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
module;

#include <glm/vec3.hpp>

#include <chrono>
#include <filesystem>
#include <map>
#include <functional>
#include <unordered_set>
#include <variant>
#include <version>

#ifdef _WIN32
#include <libloaderapi.h>
#endif

export module xmbshell.config;

import giomm;
import glibmm;
import glm;
import vulkan_hpp;
import xmbshell.constants;

export namespace config
{
    class config
    {
        public:
            config() = default;

            enum class background_type {
                wave, color, image
            };
            struct simple_color {
                glm::vec3 color;

                template<typename Clock>
                glm::vec3 get(std::chrono::time_point<Clock, std::chrono::seconds>) {
                    return color;
                }
            };
            struct month_dependent_color{
                template<typename Clock>
                glm::vec3 get(std::chrono::time_point<Clock, std::chrono::seconds> time) {
                    std::chrono::year_month_day ymd = std::chrono::year_month_day(
                        std::chrono::floor<std::chrono::days>(time)
                    );
                    /* Adapted from OpenXMB: https://github.com/phenom64/OpenXMB/blob/243b5a33e3c3cd12746e7e5eba57d065f9519f09/src/utils.cpp#L146-L160
                    * Copyright (C) 2025 Syndromatic Ltd. All rights reserved
                    * Designed by Kavish Krishnakumar in Manchester.
                    */
                    // Approximate PS3 palette — monthly anchor colours (sRGB 0..1)
                    static constexpr std::array<glm::vec3, 12> MONTH_COLOURS = {
                        glm::vec3{0.95f, 0.90f, 0.65f}, // Jan  – pale yellow
                        glm::vec3{0.62f, 0.27f, 0.25f}, // Feb  – red/brown
                        glm::vec3{0.30f, 0.65f, 0.25f}, // Mar  – green
                        glm::vec3{0.95f, 0.60f, 0.80f}, // Apr  – pink
                        glm::vec3{0.60f, 0.80f, 0.35f}, // May  – light green
                        glm::vec3{0.70f, 0.60f, 0.90f}, // Jun  – purple
                        glm::vec3{0.50f, 0.85f, 0.95f}, // Jul  – cyan
                        glm::vec3{0.20f, 0.45f, 0.95f}, // Aug  – blue
                        glm::vec3{0.18f, 0.18f, 0.45f}, // Sep  – navy
                        glm::vec3{0.60f, 0.30f, 0.70f}, // Oct  – violet
                        glm::vec3{0.80f, 0.50f, 0.25f}, // Nov  – orange/brown
                        glm::vec3{0.90f, 0.25f, 0.25f}  // Dec  – red
                    };
                    /* Adapted from https://github.com/phenom64/OpenXMB/blob/243b5a33e3c3cd12746e7e5eba57d065f9519f09/src/utils.cpp#L183-L226
                     * Copyright (C) 2025 Syndromatic Ltd. All rights reserved
                     * Designed by Kavish Krishnakumar in Manchester.
                     */
                    auto month = static_cast<unsigned>(ymd.month());
                    auto day = static_cast<unsigned>(ymd.day());
                    auto days_in_month = static_cast<unsigned>((ymd.year() / ymd.month() / std::chrono::last).day());

                    // Temporal fade: major colour changes on the 15th and 24th with smooth easing
                    float r = (day - 1) / float(days_in_month);
                    float a1 = 15.0f / (days_in_month); // first anchor (~15th)
                    float a2 = 24.0f / (days_in_month); // second anchor (~24th)
                    glm::vec3 cPrev = MONTH_COLOURS[(month + 11) % 12];
                    glm::vec3 cCurr = MONTH_COLOURS[month % 12];
                    glm::vec3 cNext = MONTH_COLOURS[(month + 1) % 12];

                    auto ease = [](float x){ x = std::clamp(x,0.0f,1.0f); return x*x*(3.0f-2.0f*x); };
                    glm::vec3 c = cCurr;
                    if(r < a1) {
                        float t = ease(r / a1);
                        c = glm::mix(cPrev, cCurr, t);
                    } else if(r < a2) {
                        float t = ease((r - a1) / (a2 - a1));
                        c = glm::mix(cCurr, cCurr, t); // hold colour through middle third
                    } else {
                        float t = ease((r - a2) / (1.0f - a2));
                        c = glm::mix(cCurr, cNext, t);
                    }
                    return c;
                }
            };
            struct time_dimming_color {
                std::variant<simple_color, month_dependent_color> base;

                template<typename Clock>
                glm::vec3 get(std::chrono::time_point<Clock, std::chrono::seconds> time) {
                    glm::vec3 baseColor = std::visit([&time](auto&& arg) {
                        return arg.get(time);
                    }, base);
                    /* Adapted from OpenXMB: https://github.com/phenom64/OpenXMB/blob/243b5a33e3c3cd12746e7e5eba57d065f9519f09/src/utils.cpp#L168-L181
                     * Copyright (C) 2025 Syndromatic Ltd. All rights reserved
                     * Designed by Kavish Krishnakumar in Manchester.
                     */
                    std::chrono::hh_mm_ss time_of_day{time - std::chrono::floor<std::chrono::days>(time)};
                    constexpr std::array<float, 24> brightness = {
                        0.05f, 0.05f, 0.05f, 0.05f,
                        0.10f, 0.15f, 0.25f, 0.35f,
                        0.45f, 0.60f, 0.75f, 0.90f,
                        1.00f, 0.95f, 0.85f, 0.75f,
                        0.60f, 0.50f, 0.40f, 0.30f,
                        0.20f, 0.12f, 0.08f, 0.06f
                    };
                    int hours = time_of_day.hours().count() % 24;
                    int a = hours;
                    int b = (hours + 1) % 24;
                    float f = time_of_day.minutes().count() / 60.0f;
                    float brightness_factor = glm::mix(brightness[a], brightness[b], f);
                    return baseColor * (brightness_factor / 2.0f);
                }
            };
            using color_scheme = std::variant<simple_color, month_dependent_color, time_dimming_color>;

#if __linux__
            std::filesystem::path exe_directory = std::filesystem::canonical("/proc/self/exe").parent_path();
#elif _WIN32
            std::filesystem::path exe_directory = [](){
                std::array<char, MAX_PATH> buffer{};
                if (GetModuleFileNameA(nullptr, buffer.data(), MAX_PATH) == 0) {
                    throw std::runtime_error("Failed to get executable path");
                }
                return std::filesystem::path(std::string_view{buffer}).parent_path();
            }();
#else
            std::filesystem::path exe_directory = std::filesystem::current_path(); // best guess for other platforms
#endif
            std::filesystem::path asset_directory = [this](){
                if(auto v = std::getenv("XMB_ASSET_DIR"); v != nullptr) {
                    return std::filesystem::path(v);
                }
                return exe_directory / std::string(constants::asset_directory);
            }();
            std::filesystem::path locale_directory = [this](){
                if(auto v = std::getenv("XMB_LOCALE_DIR"); v != nullptr) {
                    return std::filesystem::path(v);
                }
                return exe_directory / std::string(constants::locale_directory);
            }();
            std::filesystem::path fallback_font = exe_directory / std::string(constants::fallback_font);

            vk::PresentModeKHR      preferredPresentMode = vk::PresentModeKHR::eFifoRelaxed; //aka VSync
            vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e4; // aka Anti-aliasing

            double                          maxFPS = 100;
            std::chrono::duration<double>   frameTime = std::chrono::duration<double>(std::chrono::seconds(1))/maxFPS;

            bool showFPS    = false;
            bool showMemory = false;

            std::filesystem::path   fontPath;
            background_type			backgroundType = background_type::wave;
            color_scheme            backgroundColor{};
            std::filesystem::path   backgroundImage;
            color_scheme            waveColor{};
            std::string             dateTimeFormat = constants::fallback_datetime_format;
            double                  dateTimeOffset = 0.0;
            std::string             language;

            std::unordered_set<std::string> excludedApplications;

            bool controllerRumble = true;
            bool controllerAnalogStick = true;

            std::string controllerType;

            void load();
            void reload();
            void addCallback(const std::string& key, std::function<void(const std::string&)> callback);

            void setSampleCount(vk::SampleCountFlagBits count);
            void setMaxFPS(double fps);
            void setFontPath(std::string path);
            void setBackgroundType(background_type type);
            void setBackgroundType(std::string_view type);
            void setBackgroundType(const std::string& type);
            void setBackgroundColor(color_scheme color);
            void setBackgroundColor(std::string_view hex);
            void setBackgroundColor(const std::string& hex);
            void setWaveColor(color_scheme color);
            void setWaveColor(std::string_view hex);
            void setWaveColor(const std::string& hex);
            void setDateTimeFormat(const std::string& format);
            void setLanguage(const std::string& lang);

            void excludeApplication(const std::string& application, bool exclude = true);
        private:
            Glib::RefPtr<Gio::Settings> shellSettings, renderSettings;
            std::multimap<std::string, std::function<void(const std::string&)>> callbacks;
            void on_update(const Glib::ustring& key);
    };
    inline class config CONFIG;
}
