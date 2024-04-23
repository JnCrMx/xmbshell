#pragma once

#include <freetype2/ft2build.h>
#include <freetype/freetype.h>

namespace freetype {

class ft_library {
    private:
        FT_Library library;
    public:
        ft_library() {
            FT_Init_FreeType(&library);
        }
        ~ft_library() {
            FT_Done_FreeType(library);
        }
        FT_Library get() {
            return library;
        }
};

}
