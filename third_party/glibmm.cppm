module;

#include <glibmm.h>

export module glibmm;

export namespace Glib {
    using Glib::ustring;
    using Glib::RefPtr;
    using Glib::get_user_name;
    using Glib::get_real_name;
    using Glib::get_home_dir;
    using Glib::get_user_cache_dir;
    using Glib::MainLoop;
};

export namespace sigc {
    using sigc::mem_fun;
}
