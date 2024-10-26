module;

#include <chrono>
#include <future>

#include <glm/glm.hpp>

export module xmbshell.utils;

export enum class result {
	unsupported  = (1<<0),
	success      = (1<<1),
	failure      = (1<<2),
	submenu	     = (1<<3),

	ok_sound     = (1<<4),
	error_rumble = (1<<5),
};
export inline result operator|(result a, result b) {
	return static_cast<result>(static_cast<int>(a) | static_cast<int>(b));
}
export inline bool operator&(result a, result b) {
	return static_cast<bool>(static_cast<int>(a) & static_cast<int>(b));
}

export enum class action {
	none = 0,
	left = 1,
	right = 2,
	up = 3,
	down = 4,
	ok = 5,
	cancel = 6
};
export class action_receiver {
	public:
		virtual ~action_receiver() = default;
		virtual result on_action(action action) = 0;
};

export namespace utils
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

	using time_point = std::chrono::time_point<std::chrono::system_clock>;
	constexpr double progress(time_point now, time_point start, std::chrono::duration<double> duration) {
		double d = std::chrono::duration<double>(now - start).count() / duration.count();
		return std::clamp(d, 0.0, 1.0);
	}

	template<typename T>
	class aligned_wrapper {
		public:
			aligned_wrapper(void* data, std::size_t alignment) :
				data(static_cast<std::byte*>(data)), alignment(alignment) {}

			T& operator[](std::size_t i) {
				return *reinterpret_cast<T*>(data + i * aligned(sizeof(T), alignment));
			}

			const T& operator[](std::size_t i) const {
				return *reinterpret_cast<const T*>(data + i * aligned(sizeof(T), alignment));
			}

			T* operator+(std::size_t i) {
				return reinterpret_cast<T*>(data + i * aligned(sizeof(T), alignment));
			}

			std::size_t offset(std::size_t i) const {
				return i * aligned(sizeof(T), alignment);
			}
		private:
			constexpr static std::size_t aligned(std::size_t size, std::size_t alignment) {
				return (size + alignment - 1) & ~(alignment - 1);
			}

			std::byte* data;
			std::size_t alignment;
	};

	std::string demangle(const char* name);

    template <class T>
    std::string type_name(const T& t) {
        return demangle(typeid(t).name());
    }
}
