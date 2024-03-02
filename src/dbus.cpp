#include "dbus.hpp"
#include "render/window.hpp"
#include "config.hpp"

#include <sdbus-c++/sdbus-c++.h>
#include <spdlog/spdlog.h>

namespace dbus
{
	dbus_server::dbus_server(render::window* w) : win(w)
	{
    	const char* serviceName = "re.jcm.xmbos.xmbshell";
    	connection = sdbus::createSessionBusConnection(serviceName);

    	const char* objectPath = "/re/jcm/xmbos/xmbshell";
    	object = sdbus::createObject(*connection, objectPath);

		auto close = [this](){
			spdlog::info("Exit request from D-Bus");
			// TODO
		};
    	object->registerMethod("close").onInterface("re.jcm.xmbos.Window").implementedAs(std::move(close));
		object->registerProperty("fps").onInterface("re.jcm.xmbos.Render").withGetter([this](){return win->currentFPS;});
		object->registerProperty("maxFps").onInterface("re.jcm.xmbos.Render").withGetter([this](){return config::CONFIG.maxFPS;}).withSetter([this](double maxFps){
			config::CONFIG.maxFPS = maxFps;
			config::CONFIG.frameTime = std::chrono::duration<double>(std::chrono::seconds(1))/maxFps;
		});

		object->finishRegistration();
	}

	dbus_server::~dbus_server()
	{
		connection->leaveEventLoop();
	}

	void dbus_server::run()
	{
    	connection->enterEventLoopAsync();
	}
}
