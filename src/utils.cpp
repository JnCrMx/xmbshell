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
