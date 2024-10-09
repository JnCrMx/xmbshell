#include "app/components/news_display.hpp"

#include <chrono>
#include <cmath>

namespace app {

news_display::news_display(class xmbshell* shell) : shell(shell) {}

void news_display::preload(vk::Device device, vma::Allocator allocator, dreamrender::resource_loader& loader) {
}

void news_display::tick() {
}

void news_display::render(dreamrender::gui_renderer& renderer) {
    tick();

    constexpr double base_x = 0.75;
    constexpr double base_y = 0.15;
    constexpr double box_width = 0.15;
    constexpr double font_size = 0.021296296*2.5;
    constexpr double speed = 0.05;
    constexpr double spacing = 0.025;

    static auto begin = std::chrono::system_clock::now();
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration<double>(now - begin).count() * speed;

    std::string_view news = "Lorem ipsum dolor sit amet, consectetur adipiscing elit";
    double width = renderer.measure_text(news, font_size).x;
    double x = std::fmod(elapsed, width + spacing);

    renderer.set_clip(base_x, base_y, box_width, font_size);

    renderer.draw_text(news, base_x+box_width-x, base_y, font_size);
    renderer.draw_text(news, base_x+box_width-(x+width+spacing), base_y, font_size);

    renderer.reset_clip();
}

}
