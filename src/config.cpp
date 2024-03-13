#include "config.hpp"
#include <giomm/settings.h>

namespace config {

std::optional<glm::vec3> parse_color(std::string_view hex) {
    if(hex.size() != 7 || hex[0] != '#') {
        return std::nullopt;
    }
    auto parseHex = [](std::string_view hex) -> std::optional<unsigned int> {
        unsigned int value;
        if(std::from_chars(hex.data(), hex.data() + hex.size(), value, 16).ec == std::errc()) {
            return value;
        }
        return std::nullopt;
    };
    return glm::vec3(
        parseHex(hex.substr(1, 2)).value_or(0) / 255.0f,
        parseHex(hex.substr(3, 2)).value_or(0) / 255.0f,
        parseHex(hex.substr(5, 2)).value_or(0) / 255.0f
    );
}

void config::load() {
    Glib::RefPtr<Gio::Settings> shellSettings =
        Gio::Settings::create("re.jcm.xmbos.xmbshell");
    setFontPath(shellSettings->get_string("font-path"));
    setBackgroundType(shellSettings->get_string("background-type"));
    setBackgroundColor(shellSettings->get_string("background-color"));
    backgroundImage = shellSettings->get_string("background-image");
    setWaveColor(shellSettings->get_string("wave-color"));
    setDateTimeFormat(shellSettings->get_string("date-time-format"));
    dateTimeOffset = shellSettings->get_double("date-time-x-offset");
    controllerRumble = shellSettings->get_boolean("controller-rumble");
    controllerAnalogStick = shellSettings->get_boolean("controller-analog-stick");

    Glib::RefPtr<Gio::Settings> renderSettings =
        Gio::Settings::create("re.jcm.xmbos.xmbshell.render");
    bool vsync = renderSettings->get_boolean("vsync");
    if(vsync) {
        preferredPresentMode = vk::PresentModeKHR::eFifoRelaxed;
    } else {
        preferredPresentMode = vk::PresentModeKHR::eMailbox;
    }
    setMaxFPS(renderSettings->get_int("max-fps"));

    int sampleCount = renderSettings->get_int("sample-count");
    switch(sampleCount) {
        case 1: setSampleCount(vk::SampleCountFlagBits::e1); break;
        case 2: setSampleCount(vk::SampleCountFlagBits::e2); break;
        case 4: setSampleCount(vk::SampleCountFlagBits::e4); break;
        case 8: setSampleCount(vk::SampleCountFlagBits::e8); break;
        case 16: setSampleCount(vk::SampleCountFlagBits::e16); break;
        case 32: setSampleCount(vk::SampleCountFlagBits::e32); break;
        case 64: setSampleCount(vk::SampleCountFlagBits::e64); break;
        default: setSampleCount(vk::SampleCountFlagBits::e1); break;
    }

    showFPS = renderSettings->get_boolean("show-fps");
    showMemory = renderSettings->get_boolean("show-mem");
}

void config::setSampleCount(vk::SampleCountFlagBits count) {
    sampleCount = count;
}

void config::setMaxFPS(double fps) {
    if(fps <= 0) {
        maxFPS = std::numeric_limits<double>::max();
        frameTime = std::chrono::duration<double>(0);
        return;
    }
    maxFPS = fps;
    frameTime = std::chrono::duration<double>(std::chrono::seconds(1))/maxFPS;
}

void config::setFontPath(std::string path) {
    fontPath = path;
}

void config::setBackgroundType(background_type type) {
    backgroundType = type;
}
void config::setBackgroundType(std::string_view type) {
    if(type == "wave") {
        backgroundType = background_type::wave;
    } else if(type == "color") {
        backgroundType = background_type::color;
    } else if(type == "image") {
        backgroundType = background_type::image;
    } else {
        spdlog::error("Ignoring invalid background-type: {}", type);
        backgroundType = background_type::wave;
    }
}
void config::setBackgroundType(const std::string& type) {
    setBackgroundType(std::string_view(type));
}

void config::setBackgroundColor(glm::vec3 color) {
    backgroundColor = color;
}
void config::setBackgroundColor(std::string_view hex) {
    auto color = parse_color(hex);
    if(color) {
        backgroundColor = *color;
    } else {
        spdlog::error("Ignoring invalid hex background-color: {}", hex);
    }
}
void config::setBackgroundColor(const std::string& hex) {
    setBackgroundColor(std::string_view(hex));
}

void config::setWaveColor(glm::vec3 color) {
    waveColor = color;
}
void config::setWaveColor(std::string_view hex) {
    auto color = parse_color(hex);
    if(color) {
        waveColor = *color;
    } else {
        spdlog::error("Ignoring invalid hex wave-color: {}", hex);
    }
}
void config::setWaveColor(const std::string& hex) {
    setWaveColor(std::string_view(hex));
}

void config::setDateTimeFormat(const std::string& format) {
    auto now = std::chrono::zoned_time(std::chrono::current_zone(), std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
    try {
        std::ignore = std::vformat("{:"+format+"}", std::make_format_args(now));
        dateTimeFormat = format;
    } catch(const std::exception& e) {
        spdlog::error("Invalid date-time format: \"{}\", error is: {}", format, e.what());
        spdlog::warn("Using fallback format: {}", constants::fallback_datetime_format);
        dateTimeFormat = constants::fallback_datetime_format;
    }
}

}
