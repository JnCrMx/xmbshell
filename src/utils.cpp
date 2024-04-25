#include "utils.hpp"

#include <sstream>
#include <iomanip>

namespace utils
{
	std::string to_fixed_string(double d, int n)
	{
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(n) << d;
		return oss.str();
	}
}

#ifdef __GNUG__
#include <cxxabi.h>
namespace utils
{
	std::string demangle(const char *name) {
		int status = -4;
		std::unique_ptr<char, void(*)(void*)> res{
			abi::__cxa_demangle(name, nullptr, nullptr, &status),
			std::free
		};
		return (status==0) ? res.get() : name;
	}
}
#else
namespace utils
{
	std::string demangle(const char *name) {
		return std::string(name);
	}
}
#endif
