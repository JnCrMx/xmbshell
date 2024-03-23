#pragma once

#include <cstdint>

namespace constants
{
	constexpr auto displayname = "XMB Shell";
	constexpr auto name = "xmbshell";

	constexpr uint32_t version = 1;

	constexpr auto asset_directory = "../share/xmbshell/";
	constexpr auto fallback_font = "../share/xmbshell/Ubuntu-R.ttf";
	constexpr auto fallback_datetime_format = "%m/%d %H:%M";
	constexpr auto pipeline_cache_file = "pipeline_cache.bin";
}
