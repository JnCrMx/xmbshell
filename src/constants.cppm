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

#include <cstdint>
#include <limits>

export module xmbshell.constants;

export namespace constants
{
    constexpr auto displayname = "XMB Shell";
    constexpr auto name = "xmbshell";

    constexpr uint32_t version = 1;

#if defined(_WIN32)
    constexpr auto asset_directory = "./xmbshell/";
    constexpr auto locale_directory = "./locale/";
    constexpr auto fallback_font = "./xmbshell/Ubuntu-R.ttf";
#else
    constexpr auto asset_directory = "../share/xmbshell/";
    constexpr auto locale_directory = "../share/locale/";
    constexpr auto fallback_font = "../share/xmbshell/Ubuntu-R.ttf";
#endif

    constexpr auto fallback_datetime_format = "%m/%d %H:%M";
    constexpr auto pipeline_cache_file = "pipeline_cache.bin";

    constexpr int controller_axis_input_threshold = 10000;
    constexpr float controller_axis_input_threshold_f = static_cast<float>(controller_axis_input_threshold) / std::numeric_limits<int16_t>::max();
}
