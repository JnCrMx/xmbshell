#pragma once

#include <future>
#include <sstream>
#include <iomanip>

#include <glm/glm.hpp>

namespace utils
{
	template<typename R>
	bool is_ready(std::future<R> const& f)
	{ return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }

	template<typename R>
	bool is_ready(std::shared_future<R> const& f)
	{ return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }

	std::string to_fixed_string(double d, int n);

	template<int n, typename T>
	std::string to_fixed_string(T d)
	{
		return to_fixed_string(d, n);
	}
}
