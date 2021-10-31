#pragma once

#include <simppl/interface.h>
#include <simppl/skeleton.h>

namespace backgroundwave
{
	using namespace simppl::dbus;
	INTERFACE(Control)
	{
		Property<std::tuple<int, int, int>, ReadWrite|Notifying> color;
		Property<double, ReadWrite|Notifying> speed;
		Method<simppl::dbus::oneway> shutdown;

		inline Control() : INIT(color), INIT(speed), INIT(shutdown)
		{

		}
	};
}

namespace render {
class window;
}

class MyControl : public simppl::dbus::Skeleton<backgroundwave::Control>
{
public:
    MyControl(simppl::dbus::Dispatcher& disp);
	render::window* win;
};
