#include "config.hpp"
#include <giomm/settings.h>

namespace config {

void config::load() {
    Glib::RefPtr<Gio::Settings> shellSettings =
        Gio::Settings::create("re.jcm.xmbos.xmbshell");
    setFontPath(shellSettings->get_string("font-path"));
    setWaveColor(shellSettings->get_string("wave-color"));

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

}
