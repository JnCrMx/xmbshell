#pragma once

#include <future>

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
