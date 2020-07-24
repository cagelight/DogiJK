#pragma once

#include "q_math.hh"

#ifndef QMATH2_HASH_FUNCTIONS
#define QMATH2_HASH_FUNCTIONS 1
#endif

#include <cstdint>
#include <array>
#include <ctgmath>
#include <random>

#if QMATH2_HASH_FUNCTIONS == 1
#include <functional>
#endif

//================================================================
//----------------------------------------------------------------
//================================================================

namespace qm::meta {
	
	template <typename T> static T constexpr const pi = static_cast<T>(3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844609550582231725359408128481117450284102701938521105559644622948954930381964428810975665933446128475648233786783165271201909145649L);
	template <typename T> constexpr T deg2rad(T const & v) { return v * pi<T> / static_cast<T>(180); }
	template <typename T> constexpr T rad2deg(T const & v) { return v / pi<T> * static_cast<T>(180); }
	
	template <typename T> struct vec2_t;
	template <typename T> struct vec3_t;
	template <typename T> struct vec4_t;
	
	template <typename T> struct quat_t;
	
	template <typename T> struct mat3_t;
	template <typename T> struct mat4_t;
	
	// SIGNAL TRANSFORMS
	
	template <typename T>
	constexpr T pexp(T const & v, T const & e) noexcept {
		T const ve = std::pow(v, e);
		return ve / (ve + std::pow(static_cast<T>(1) - v, e));
	}
	
	// LERPS
	
	template <typename T, typename V>
	constexpr T lerp(T const & a, T const & b, V const & v) noexcept {
		return (static_cast<V>(1) - v) * a + v * b;
	}
	
	template <typename T, typename V>
	constexpr T plerp(T const & a, T const & b, V const & v, V const & e) noexcept {
		V const l = pexp<V>(v, e);
		return (static_cast<V>(1) - l) * a + l * b;
	}
}

//================================================================
//----------------------------------------------------------------
//================================================================

namespace qm {
	
	static constexpr float pi = meta::pi<float>;
	constexpr float deg2rad(float const & v) { return v * pi / 180.0f; }
	constexpr float rad2deg(float const & v) { return v / pi * 180.0f; }
	
	template <typename T>
	constexpr T lerp(T const & A, T const & B, float v) { return meta::lerp<T, float>(A, B, v); }
	
	template <typename T> constexpr T clamp(T const & value, T const & min, T const & max) {
		if (value < min) return min;
		if (value > max) return max;
		return value;
	}
	
	template <typename T, typename std::enable_if_t<std::is_integral<T>::value && sizeof(T) == 4>* = nullptr>
	constexpr T next_pow2(T i) {
		if (i <= 0) return 0;
		if (i == 1) return 1;
		return 1 << (32 - __builtin_clz(i - 1));
	}
	
	template <typename T, typename std::enable_if_t<std::is_integral<T>::value && sizeof(T) == 8>* = nullptr>
	constexpr T next_pow2(T i) {
		if (i <= 0) return 0;
		if (i == 1) return 1;
		return 1 << (64 - __builtin_clzll(i - 1));
	}
	
	template <typename T, typename std::enable_if_t<std::is_integral<T>::value && sizeof(T) == 4>* = nullptr>
	constexpr T fast_log2(T i) {
		if (i <= 0) return 0;
		return 32 - __builtin_clz(i);
	}

	template <typename T, typename std::enable_if_t<std::is_integral<T>::value && sizeof(T) == 8>* = nullptr>
	constexpr T fast_log2(T i) {
		if (i <= 0) return 0;
		return 64 - __builtin_clzll(i);
	}
	
	using vec2_t = meta::vec2_t<float>;
	using ivec2_t = meta::vec2_t<int32_t>;
	
	using vec3_t = meta::vec3_t<float>;
	using ivec3_t = meta::vec3_t<int32_t>;
	
	using vec4_t = meta::vec4_t<float>;
	using ivec4_t = meta::vec4_t<int32_t>;
	
	using quat_t = meta::quat_t<float>;
	using mat3_t = meta::mat3_t<float>;
	using mat4_t = meta::mat4_t<float>;
	
	/*
	template <typename T>
	struct xorshift32 {
		
		using result_type = T;
		static_assert(sizeof(result_type) == 4);
		
		static constexpr void next(T & value) {
			
			value ^= value << 13;
			value ^= value >> 17;
			value ^= value << 5;
		}
		
		xorshift32(result_type const & r) : a(r) {}
		result_type operator() () {
			next(a);
			return a;
		}
		
		result_type & state() { return a; }
		
		static constexpr result_type min() { return std::numeric_limits<result_type>::min(); }
		static constexpr result_type max() { return std::numeric_limits<result_type>::max(); }
	private:
		result_type a;
	};
	
	template <typename T>
	struct xorshift128plus {
		
		using result_type = T;
		static_assert(sizeof(result_type) == 8);
		
		xorshift128plus(result_type const & r) : a(r), b(~r) {}
		result_type operator() () {
			result_type t = a;
			result_type const s = b;
			a = s;
			t ^= t << 23;
			t ^= t >> 17;
			t ^= s ^ (s >> 26);
			b = t;
			return t + s;
		}
		
		static constexpr result_type min() { return std::numeric_limits<result_type>().min(); }
		static constexpr result_type max() { return std::numeric_limits<result_type>().max(); }
	private:
		result_type a, b;
	};
	*/
	
	using random_engine = std::mt19937_64;
	//using random_engine = xorshift128plus<uint64_t>;
	static inline random_engine rng { std::random_device {}() };
}

//================================================================
//----------------------------------------------------------------
// VEC2_T
//----------------------------------------------------------------
//================================================================

template <typename T>
struct qm::meta::vec2_t {
	
	using data_t = std::array<T, 2>;
	data_t data;
	
	// CONSTRUCT
	
	vec2_t() noexcept = default; 
	constexpr vec2_t(vec2_t const &) noexcept = default;
	constexpr vec2_t(vec2_t &&) noexcept = default;
	
	constexpr vec2_t(T const & x, T const & y) noexcept : data { x, y } {}
	constexpr vec2_t(::vec2_t const & ptr) : data { ptr[0], ptr[1] } {}
	
	template <typename U>
	static vec2_t from(vec2_t<U> const & other) noexcept {
		return {
			static_cast<T>(other[0]),
			static_cast<T>(other[1]),
		};
	}
	
	// COMMON
	
	constexpr T & x() noexcept { return data[0]; }
	constexpr T const & x() const noexcept { return data[0]; }
	constexpr T & y() noexcept { return data[1]; }
	constexpr T const & y() const noexcept { return data[1]; }
	
	constexpr T * ptr() { return data.data(); }
	constexpr T const * ptr() const { return data.data(); }
	
	constexpr T magnitude_squared() const noexcept {
		return data[0] * data[0] + data[1] * data[1];
	}
	
	constexpr T magnitude() const noexcept {
		return std::sqrt(magnitude_squared());
	}
	
	constexpr T normalize() noexcept {
		T mag = magnitude();
		data[0] /= mag;
		data[1] /= mag;
		return mag;
	}
	
	constexpr vec2_t normalized() const noexcept {
		T mag = magnitude();
		return { data[0] / mag, data[1] / mag };
	}
	
	static constexpr T dot(vec2_t const & A, vec2_t const & B) noexcept {
		return A[0] * B[0] + A[1] * B[1];
	}
	constexpr T dot(vec2_t const & other) const noexcept { return vec2_t::dot(*this, other); }
	
	constexpr bool is_real() const {
		if ( std::isnan(data[0]) ) return false;
		if ( std::isnan(data[1]) ) return false;
		if ( std::isinf(data[0]) ) return false;
		if ( std::isinf(data[1]) ) return false;
		return true;
	}
	
	// OPERATORS
	
	constexpr vec2_t & operator = (vec2_t const &) noexcept = default;
	constexpr vec2_t & operator = (vec2_t &&) noexcept = default;
	
	constexpr bool operator == (vec2_t const & other) const noexcept { return data == other.data; }
	constexpr bool operator != (vec2_t const & other) const noexcept { return ! operator == (other); }
	
	constexpr vec2_t operator - () const noexcept { return { -data[0], -data[1] }; }
	
	constexpr vec2_t operator + (T const & v) const noexcept { return { data[0] + v, data[1] + v }; }
	constexpr vec2_t operator + (vec2_t const & other) const noexcept { return { data[0] + other[0], data[1] + other[1] }; }
	constexpr vec2_t & operator += (T const & v) noexcept { data[0] += v; data[1] += v; return *this; }
	constexpr vec2_t & operator += (vec2_t const & other) noexcept { data[0] += other[0]; data[1] += other[1]; return *this; }
	
	constexpr vec2_t operator - (T const & v) const noexcept { return { data[0] - v, data[1] - v }; }
	constexpr vec2_t operator - (vec2_t const & other) const noexcept { return { data[0] - other[0], data[1] - other[1] }; }
	constexpr vec2_t & operator -= (T const & v) noexcept { data[0] -= v; data[1] -= v; return *this; }
	constexpr vec2_t & operator -= (vec2_t const & other) noexcept { data[0] -= other[0]; data[1] -= other[1]; return *this; }
	
	constexpr vec2_t operator * (T const & v) const noexcept { return { data[0] * v, data[1] * v }; }
	constexpr vec2_t operator * (vec2_t const & other) const noexcept { return { data[0] * other[0], data[1] * other[1] }; }
	constexpr vec2_t & operator *= (T const & v) noexcept { data[0] *= v; data[1] *= v; return *this; }
	constexpr vec2_t & operator *= (vec2_t const & other) noexcept { data[0] *= other[0]; data[1] *= other[1]; return *this; }
	
	constexpr vec2_t operator / (T const & v) const noexcept { return { data[0] / v, data[1] / v }; }
	constexpr vec2_t operator / (vec2_t const & other) const noexcept { return { data[0] / other[0], data[1] / other[1] }; }
	constexpr vec2_t & operator /= (T const & v) noexcept { data[0] /= v; data[1] /= v; return *this; }
	constexpr vec2_t & operator /= (vec2_t const & other) noexcept { data[0] /= other[0]; data[1] /= other[1]; return *this; }
	
	constexpr T & operator [] (size_t i) noexcept { return data[i]; }
	constexpr T const & operator [] (size_t i) const noexcept { return data[i]; }
	
	constexpr operator T const * () const { return &data[0]; }
	
};

//================================================================
//----------------------------------------------------------------
// VEC3_T
//----------------------------------------------------------------
//================================================================

template <typename T>
struct qm::meta::vec3_t {
	
	using data_t = std::array<T, 3>;
	data_t data;
	
	// CONSTRUCT
	
	vec3_t() noexcept = default;
	constexpr vec3_t(vec3_t const &) noexcept = default;
	constexpr vec3_t(vec3_t &&) noexcept = default;
	
	constexpr vec3_t(T const & x, T const & y, T const & z) noexcept : data { x, y, z } {}
	constexpr vec3_t(vec2_t<T> const & v, float const & z) : data { v.data[0], v.data[1], z } {}
	constexpr vec3_t(::vec3_t const & ptr) : data { ptr[0], ptr[1], ptr[2] } {}
	
	template <typename U>
	static vec3_t from(vec3_t<U> const & other) noexcept {
		return {
			static_cast<T>(other[0]),
			static_cast<T>(other[1]),
			static_cast<T>(other[2]),
		};
	}
	
	// COMMON
	
	constexpr T & x() noexcept { return data[0]; }
	constexpr T const & x() const noexcept { return data[0]; }
	constexpr T & y() noexcept { return data[1]; }
	constexpr T const & y() const noexcept { return data[1]; }
	constexpr T & z() noexcept { return data[2]; }
	constexpr T const & z() const noexcept { return data[2]; }
	
	constexpr T * ptr() { return data.data(); }
	constexpr T const * ptr() const { return data.data(); }
	
	constexpr T magnitude_squared() const noexcept {
		return data[0] * data[0] + data[1] * data[1] + data[2] * data[2];
	}
	
	constexpr T magnitude() const noexcept {
		return std::sqrt(magnitude_squared());
	}
	
	constexpr T normalize() noexcept {
		T mag = magnitude();
		data[0] /= mag;
		data[1] /= mag;
		data[2] /= mag;
		return mag;
	}
	
	constexpr vec3_t normalized() const noexcept {
		T mag = magnitude();
		return { data[0] / mag, data[1] / mag, data[2] / mag };
	}
	
	static constexpr T dot(vec3_t const & A, vec3_t const & B) noexcept {
		return A[0] * B[0] + A[1] * B[1] + A[2] * B[2];
	}
	constexpr T dot(vec3_t const & other) const noexcept {
		return vec3_t::dot(*this, other);
	}
	
	constexpr vec3_t move_along(vec3_t const & dir, T dist) const noexcept {
		return *this + dir * dist;
	}
	
	// SPECIAL
	
	static constexpr vec3_t cross(vec3_t const & A, vec3_t const & B) noexcept {
		return {
			A[1] * B[2] - A[2] * B[1],
			A[2] * B[0] - A[0] * B[2],
			A[0] * B[1] - A[1] * B[0],
		};
	}
	constexpr vec3_t cross(vec3_t const & other) const noexcept { vec3_t::cross(*this, other); }
	
	constexpr void assign_to(::vec3_t & vec) const {
		vec[0] = data[0];
		vec[1] = data[1];
		vec[2] = data[2];
	}
	
	constexpr bool is_real() const {
		if ( std::isnan(data[0]) ) return false;
		if ( std::isnan(data[1]) ) return false;
		if ( std::isnan(data[2]) ) return false;
		if ( std::isinf(data[0]) ) return false;
		if ( std::isinf(data[1]) ) return false;
		if ( std::isinf(data[2]) ) return false;
		return true;
	}
	
	// OPERATORS
	
	constexpr vec3_t & operator = (vec3_t const &) noexcept = default;
	constexpr vec3_t & operator = (vec3_t &&) noexcept = default;
	
	constexpr bool operator == (vec3_t const & other) const noexcept { return data == other.data; }
	constexpr bool operator != (vec3_t const & other) const noexcept { return ! operator == (other); }
	
	constexpr vec3_t operator - () const noexcept { return { -data[0], -data[1], -data[2] }; }
	
	constexpr vec3_t operator + (T const & v) const noexcept { return { data[0] + v, data[1] + v, data[2] + v }; }
	constexpr vec3_t operator + (vec3_t const & other) const noexcept { return { data[0] + other[0], data[1] + other[1], data[2] + other[2] }; }
	constexpr vec3_t & operator += (T const & v) noexcept { data[0] += v; data[1] += v; data[2] += v; return *this; }
	constexpr vec3_t & operator += (vec3_t const & other) noexcept { data[0] += other[0]; data[1] += other[1]; data[2] += other[2]; return *this; }
	
	constexpr vec3_t operator - (T const & v) const noexcept { return { data[0] - v, data[1] - v, data[2] - v }; }
	constexpr vec3_t operator - (vec3_t const & other) const noexcept { return { data[0] - other[0], data[1] - other[1], data[2] - other[2] }; }
	constexpr vec3_t & operator -= (T const & v) noexcept { data[0] -= v; data[1] -= v; data[2] -= v; return *this; }
	constexpr vec3_t & operator -= (vec3_t const & other) noexcept { data[0] -= other[0]; data[1] -= other[1]; data[2] -= other[2]; return *this; }
	
	constexpr vec3_t operator * (T const & v) const noexcept { return { data[0] * v, data[1] * v, data[2] * v }; }
	constexpr vec3_t operator * (vec3_t const & other) const noexcept { return { data[0] * other[0], data[1] * other[1], data[2] * other[2] }; }
	constexpr vec3_t & operator *= (T const & v) noexcept { data[0] *= v; data[1] *= v; data[2] *= v; return *this; }
	constexpr vec3_t & operator *= (vec3_t const & other) noexcept { data[0] *= other[0]; data[1] *= other[1]; data[2] *= other[2]; return *this; }
	
	constexpr vec3_t operator / (T const & v) const noexcept { return { data[0] / v, data[1] / v, data[2] / v }; }
	constexpr vec3_t operator / (vec3_t const & other) const noexcept { return { data[0] / other[0], data[1] / other[1], data[2] / other[2] }; }
	constexpr vec3_t & operator /= (T const & v) noexcept { data[0] /= v; data[1] /= v; data[2] /= v; return *this; }
	constexpr vec3_t & operator /= (vec3_t const & other) noexcept { data[0] /= other[0]; data[1] /= other[1]; data[2] /= other[2]; return *this; }
	
	constexpr T & operator [] (size_t i) noexcept { return data[i]; }
	constexpr T const & operator [] (size_t i) const noexcept { return data[i]; }
	
	constexpr operator T const * () const { return &data[0]; }
};

//================================================================
//----------------------------------------------------------------
// VEC4_T
//----------------------------------------------------------------
//================================================================

template <typename T>
struct qm::meta::vec4_t {
	
	using data_t = std::array<T, 4>;
	data_t data;
	
	// CONSTRUCT
	
	vec4_t() noexcept = default;
	constexpr vec4_t(vec4_t const &) noexcept = default;
	constexpr vec4_t(vec4_t &&) noexcept = default;
	
	constexpr vec4_t(T const & x, T const & y, T const & z, T const & w) noexcept : data { x, y, z, w } {}
	constexpr vec4_t(vec3_t<T> const & v, float const & w) : data { v.data[0], v.data[1], v.data[2], w } {}
	
	template <typename U>
	static vec4_t from(vec4_t<U> const & other) noexcept {
		return {
			static_cast<T>(other[0]),
			static_cast<T>(other[1]),
			static_cast<T>(other[2]),
			static_cast<T>(other[3]),
		};
	}
	
	// COMMON
	
	constexpr T & x() noexcept { return data[0]; }
	constexpr T const & x() const noexcept { return data[0]; }
	constexpr T & y() noexcept { return data[1]; }
	constexpr T const & y() const noexcept { return data[1]; }
	constexpr T & z() noexcept { return data[2]; }
	constexpr T const & z() const noexcept { return data[2]; }
	constexpr T & w() noexcept { return data[3]; }
	constexpr T const & w() const noexcept { return data[3]; }
	
	constexpr T * ptr() { return data.data(); }
	constexpr T const * ptr() const { return data.data(); }
	
	constexpr T magnitude_squared() const noexcept {
		return data[0] * data[0] + data[1] * data[1] + data[2] * data[2] + data[3] * data[3];
	}
	
	constexpr T magnitude() const noexcept {
		return std::sqrt(magnitude_squared());
	}
	
	constexpr T normalize() noexcept {
		T mag = magnitude();
		data[0] /= mag;
		data[1] /= mag;
		data[2] /= mag;
		data[3] /= mag;
		return mag;
	}
	
	constexpr vec4_t normalized() const noexcept {
		T mag = magnitude();
		return { data[0] / mag, data[1] / mag, data[2] / mag, data[3] / mag };
	}
	
	static constexpr T dot(vec4_t const & A, vec4_t const & B) noexcept {
		return A[0] * B[0] + A[1] * B[1] + A[2] * B[2] + A[3] * B[3];
	}
	constexpr T dot(vec4_t const & other) const noexcept { return vec4_t::dot(*this, other); }
	
	constexpr bool is_real() const {
		if ( std::isnan(data[0]) ) return false;
		if ( std::isnan(data[1]) ) return false;
		if ( std::isnan(data[2]) ) return false;
		if ( std::isnan(data[3]) ) return false;
		if ( std::isinf(data[0]) ) return false;
		if ( std::isinf(data[1]) ) return false;
		if ( std::isinf(data[2]) ) return false;
		if ( std::isinf(data[3]) ) return false;
		return true;
	}
	
	// OPERATORS
	
	constexpr vec4_t & operator = (vec4_t const &) noexcept = default;
	constexpr vec4_t & operator = (vec4_t &&) noexcept = default;
	
	constexpr bool operator == (vec4_t const & other) const noexcept { return data == other.data; }
	constexpr bool operator != (vec4_t const & other) const noexcept { return ! operator == (other); }
	
	constexpr vec4_t operator - () const noexcept { return { -data[0], -data[1], -data[2], -data[3] }; }
	
	constexpr vec4_t operator + (T const & v) const noexcept { return { data[0] + v, data[1] + v, data[2] + v, data[3] + v }; }
	constexpr vec4_t operator + (vec4_t const & other) const noexcept { return { data[0] + other[0], data[1] + other[1], data[2] + other[2], data[3] + other[3] }; }
	constexpr vec4_t & operator += (T const & v) noexcept { data[0] += v; data[1] += v; data[2] += v; data[3] += v; return *this; }
	constexpr vec4_t & operator += (vec4_t const & other) noexcept { data[0] += other[0]; data[1] += other[1]; data[2] += other[2]; data[3] += other[3]; return *this; }
	
	constexpr vec4_t operator - (T const & v) const noexcept { return { data[0] - v, data[1] - v, data[2] - v, data[3] - v }; }
	constexpr vec4_t operator - (vec4_t const & other) const noexcept { return { data[0] - other[0], data[1] - other[1], data[2] - other[2], data[3] - other[3] }; }
	constexpr vec4_t & operator -= (T const & v) noexcept { data[0] -= v; data[1] -= v; data[2] -= v; data[3] -= v; return *this; }
	constexpr vec4_t & operator -= (vec4_t const & other) noexcept { data[0] -= other[0]; data[1] -= other[1]; data[2] -= other[2]; data[3] -= other[3]; return *this; }
	
	constexpr vec4_t operator * (T const & v) const noexcept { return { data[0] * v, data[1] * v, data[2] * v, data[3] * v }; }
	constexpr vec4_t operator * (vec4_t const & other) const noexcept { return { data[0] * other[0], data[1] * other[1], data[2] * other[2], data[3] * other[3] }; }
	constexpr vec4_t & operator *= (T const & v) noexcept { data[0] *= v; data[1] *= v; data[2] *= v; data[3] *= v; return *this; }
	constexpr vec4_t & operator *= (vec4_t const & other) noexcept { data[0] *= other[0]; data[1] *= other[1]; data[2] *= other[2]; data[3] *= other[3]; return *this; }
	
	constexpr vec4_t operator / (T const & v) const noexcept { return { data[0] / v, data[1] / v, data[2] / v, data[3] / v }; }
	constexpr vec4_t operator / (vec4_t const & other) const noexcept { return { data[0] / other[0], data[1] / other[1], data[2] / other[2], data[3] / other[3] }; }
	constexpr vec4_t & operator /= (T const & v) noexcept { data[0] /= v; data[1] /= v; data[2] /= v; data[3] /= v; return *this; }
	constexpr vec4_t & operator /= (vec4_t const & other) noexcept { data[0] /= other[0]; data[1] /= other[1]; data[2] /= other[2]; data[3] /= other[3]; return *this; }
	
	constexpr T & operator [] (size_t i) noexcept { return data[i]; }
	constexpr T const & operator [] (size_t i) const noexcept { return data[i]; }
	
	constexpr operator T const * () const { return &data[0]; }
};

//================================================================
//----------------------------------------------------------------
// QUAT_T
//----------------------------------------------------------------
//================================================================

template <typename T>
struct qm::meta::quat_t {
	
	using data_t = std::array<T, 4>;
	data_t data;
	
	// CONSTRUCT
	
	quat_t() noexcept = default;
	constexpr quat_t(quat_t const &) noexcept = default;
	constexpr quat_t(quat_t &&) noexcept = default;
	
	constexpr quat_t(vec3_t<T> const & axis, T const & angle) : data {0, 0, 0, 0} {
		T a = angle/2;
		T s = std::sin(a);
		data[0] = axis[0] * s;
		data[1] = axis[1] * s;
		data[2] = axis[2] * s;
		data[3] = std::cos(a);
		normalize();
	}
	
	constexpr quat_t(mat3_t<T> const & m) : data {0, 0, 0, 0} {
		
		T t;
		
		if (m[2][2] < 0) {
			if (m[0][0] > m[1][1]) {
				t = 1 + m[0][0] - m[1][1] - m[2][2];
				data[0] = t;
				data[1] = m[0][1] + m[1][0];
				data[2] = m[2][0] + m[0][2];
				data[3] = m[1][2] - m[2][1];
			} else {
				t = 1 - m[0][0] + m[1][1] - m[2][2];
				data[0] = m[0][1] + m[1][0];
				data[1] = t;
				data[2] = m[1][2] + m[2][1];
				data[3] = m[2][0] - m[0][2];
			}
		} else {
			if (m[0][0] < -m[1][1]) {
				t = 1 - m[0][0] - m[1][1] + m[2][2];
				data[0] = m[2][0] + m[0][2];
				data[1] = m[1][2] + m[2][1];
				data[2] = t;
				data[3] = m[0][1] - m[1][0];
			} else {
				t = 1 + m[0][0] + m[1][1] + m[2][2];
				data[0] = m[1][2] - m [2][1];
				data[1] = m[2][0] - m [0][2];
				data[2] = m[0][1] - m [1][0];
				data[3] = t;
			}
		}
		
		
		t = 0.5 * std::sqrt(t);
		data[0] *= t;
		data[1] *= t;
		data[2] *= t;
		data[3] *= t;
		
		normalize();
	}
	
	constexpr quat_t(T const & x, T const & y, T const & z, T const & w) noexcept : data { x, y, z, w } {}
	constexpr quat_t(T const * ptr) : data { ptr[0], ptr[1], ptr[2], ptr[3] } {}
	
	static constexpr quat_t identity() noexcept {
		return { static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1) };
	}
	
	static quat_t random() noexcept {
		static std::uniform_real_distribution<T> dist {0, 1};
		T u = dist(rng), v = dist(rng), w = dist(rng);
		return {
			std::sqrt(1 - u) * std::sin(2 * qm::meta::pi<T> * v),
			std::sqrt(1 - u) * std::cos(2 * qm::meta::pi<T> * v),
			std::sqrt(u)     * std::sin(2 * qm::meta::pi<T> * w),
			std::sqrt(u)     * std::cos(2 * qm::meta::pi<T> * w)
		};
	}
	
	// COMMON
	
	constexpr quat_t<T> conjugate() const {
		return {-data[0], -data[1], -data[2], data[3]};
	}
	
	constexpr quat_t<T> reciprocal() const {
		T m = norm();
		return {-data[0] / m, -data[1] / m, -data[2] / m, data[3] / m};
	}
	
	constexpr T product() const {
		return data[0] * data[0] + data[1] * data[1] + data[2] * data[2] + data[3] * data[3];
	}
	
	constexpr T norm() const {
		return std::sqrt(product());
	}
	
	constexpr quat_t<T> & normalize () {
		T v = norm();
		data[0] /= v;
		data[1] /= v;
		data[2] /= v;
		data[3] /= v;
		return *this;
	}
	
	constexpr quat_t<T> normalized () const {
		T v = norm();
		return { data[0] / v, data[1] / v, data[2] / v, data[3] / v };
	}
	
	constexpr bool is_null () const {
		return data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 1;
	}
	
	static constexpr quat_t<T> multiply(quat_t<T> const & A, quat_t<T> const & B) {
		quat_t<T> ret {
			(A.data[3] * B.data[0] + A.data[0] * B.data[3] + A.data[1] * B.data[2] - A.data[2] * B.data[1]),
			(A.data[3] * B.data[1] - A.data[0] * B.data[2] + A.data[1] * B.data[3] + A.data[2] * B.data[0]),
			(A.data[3] * B.data[2] + A.data[0] * B.data[1] - A.data[1] * B.data[0] + A.data[2] * B.data[3]),
			(A.data[3] * B.data[3] - A.data[0] * B.data[0] - A.data[1] * B.data[1] - A.data[2] * B.data[2])
		};
		ret.normalize();
		return ret;
	}

	// OPERATORS
	
	constexpr quat_t & operator = (quat_t const &) noexcept = default;
	constexpr quat_t & operator = (quat_t &&) noexcept = default;
	
	constexpr bool operator == (quat_t const & other) const noexcept { return data == other.data; }
	constexpr bool operator != (quat_t const & other) const noexcept { return ! operator == (other); }

	constexpr quat_t<T> operator * (quat_t<T> const & other) const { return quat_t<T>::multiply(*this, other); }
	
	constexpr quat_t<T> & operator *= (quat_t<T> const & other) {
		*this = quat_t<T>::multiply(other, *this); // order is reversed specifically for *= because it makes more contextual sense
		return *this;
	}
	
	constexpr T & operator [] (size_t i) noexcept { return data[i]; }
	constexpr T const & operator [] (size_t i) const noexcept { return data[i]; }
	
	constexpr operator T const * () const { return &data[0]; }
};

//================================================================
//----------------------------------------------------------------
// MAT3_T
//----------------------------------------------------------------
//================================================================

template <typename T>
struct qm::meta::mat3_t {
	
	using row_t = std::array<T, 3>;
	using data_t = std::array<row_t, 3>;
	data_t data;
	
	// CONSTRUCT
	
	mat3_t() noexcept = default;
	constexpr mat3_t(mat3_t const &) noexcept = default;
	constexpr mat3_t(mat3_t &&) noexcept = default;
	
	constexpr mat3_t(
		T const & v00, T const & v01, T const & v02,
		T const & v10, T const & v11, T const & v12,
		T const & v20, T const & v21, T const & v22
	) noexcept : data {
		row_t {v00, v01, v02,},
		row_t {v10, v11, v12,},
		row_t {v20, v21, v22,},
	} {}
	
	constexpr explicit mat3_t(quat_t<T> const & v) noexcept : mat3_t(identity()) {
		T sqx = v.data[0] * v.data[0];
		T sqy = v.data[1] * v.data[1];
		T sqz = v.data[2] * v.data[2];
		T sqw = v.data[3] * v.data[3];
		data[0][0] =  sqx - sqy - sqz + sqw;
		data[1][1] = -sqx + sqy - sqz + sqw;
		data[2][2] = -sqx - sqy + sqz + sqw;
		T t1 = v.data[0] * v.data[1];
		T t2 = v.data[2] * v.data[3];
		data[1][0] = ((T)2) * (t1 + t2);
		data[0][1] = ((T)2) * (t1 - t2);
		t1 = v.data[0] * v.data[2];
		t2 = v.data[1] * v.data[3];
		data[2][0] = ((T)2) * (t1 - t2);
		data[0][2] = ((T)2) * (t1 + t2);
		t1 = v.data[1] * v.data[2];
		t2 = v.data[0] * v.data[3];
		data[2][1] = ((T)2) * (t1 + t2);
		data[1][2] = ((T)2) * (t1 - t2);
	}
	
	constexpr T * ptr() { return &data[0][0]; }
	constexpr T const * ptr() const { return &data[0][0]; }
	
	static constexpr mat3_t identity() noexcept {
		return {
			static_cast<T>(1), static_cast<T>(0), static_cast<T>(0),
			static_cast<T>(0), static_cast<T>(1), static_cast<T>(0),
			static_cast<T>(0), static_cast<T>(0), static_cast<T>(1),
		};
	}
	
	// COMMON
	
	static constexpr mat3_t multiply(mat3_t const & A, mat3_t const & B) noexcept {
		return {
			A[0][0] * B[0][0] + A[0][1] * B[1][0] + A[0][2] * B[2][0],
			A[0][0] * B[0][1] + A[0][1] * B[1][1] + A[0][2] * B[2][1],
			A[0][0] * B[0][2] + A[0][1] * B[1][2] + A[0][2] * B[2][2],
			A[1][0] * B[0][0] + A[1][1] * B[1][0] + A[1][2] * B[2][0],
			A[1][0] * B[0][1] + A[1][1] * B[1][1] + A[1][2] * B[2][1],
			A[1][0] * B[0][2] + A[1][1] * B[1][2] + A[1][2] * B[2][2],
			A[2][0] * B[0][0] + A[2][1] * B[1][0] + A[2][2] * B[2][0],
			A[2][0] * B[0][1] + A[2][1] * B[1][1] + A[2][2] * B[2][1],
			A[2][0] * B[0][2] + A[2][1] * B[1][2] + A[2][2] * B[2][2]
		};
	}
	
	static constexpr mat3_t<T> scale(T x, T y) {
		mat3_t<T> w = identity();
		w[0][0] = x;
		w[1][1] = y;
		return w;
	}
	
	static constexpr mat3_t<T> scale(vec2_t<T> const & v) {
		mat3_t<T> w = identity();
		w[0][0] = v.data[0];
		w[1][1] = v.data[1];
		return w;
	}
	
	static constexpr mat3_t<T> translate(T x, T y) {
		mat3_t<T> w = identity();
		w[2][0] = x;
		w[2][1] = y;
		return w;
	}
	
	static constexpr mat3_t<T> translate(vec2_t<T> const & v) {
		mat3_t<T> w = identity();
		w[2][0] = v.data[0];
		w[2][1] = v.data[1];
		return w;
	}
	
	static constexpr mat3_t<T> rotate(T v) {
		mat3_t<T> m = identity();
		m[0][0] = cos(v);
		m[0][1] = -sin(v);
		m[1][0] = sin(v);
		m[1][1] = cos(v);
		return m;
	}
	
	static mat3_t<T> euler (vec3_t<T> const & v) {
		mat3_t<T> w;
		T rs = sin(-v.data[0]);
		T ps = sin(v.data[1]);
		T ys = sin(-v.data[2]);
		T rc = cos(-v.data[0]);
		T pc = cos(v.data[1]);
		T yc = cos(-v.data[2]);
		w[0][0] = yc * pc;
		w[0][1] = ps * rs - pc * ys * rc;
		w[0][2] = pc * ys * rs + ps * rc;
		w[1][0] = ys;
		w[1][1] = yc * rc;
		w[1][2] = -yc * rs;
		w[2][0] = -ps * yc;
		w[2][1] = ps * ys * rc + pc * rs;
		w[2][2] = -ps * yc * rs + pc * rc;
	}
	
	static mat3_t<T> ortho(T t, T b, T l, T r) {
		mat3_t<T> w = identity();
		w[0][0] = static_cast<T>(2) / (r - l);
		w[1][1] = static_cast<T>(2) / (t - b);
		w[2][0] = - (r + l) / (r - l);
		w[2][1] = - (t + b) / (t - b);
		return w;
	}
	
	// SPECIAL
	
	constexpr mat3_t transpose() const {
		return mat3_t {
			data[0][0], data[1][0], data[2][0],
			data[0][1], data[1][1], data[2][1],
			data[0][2], data[1][2], data[2][2]
		};
	}
	
	// OPERATORS
	
	constexpr mat3_t & operator = (mat3_t const &) noexcept = default;
	constexpr mat3_t & operator = (mat3_t &&) noexcept = default;
	
	constexpr bool operator == (mat3_t const & other) const noexcept { return data == other.data; }
	constexpr bool operator != (mat3_t const & other) const noexcept { return ! operator == (other); }
	
	constexpr mat3_t operator * (mat3_t const & other) const noexcept { return mat3_t::multiply(*this, other); }
		
	inline mat3_t<T> & operator *= (mat3_t<T> const & other) {
		*this = mat3_t<T>::multiply(*this, other);
		return *this;
	}
	
	constexpr row_t & operator [] (size_t i) noexcept { return data[i]; }
	constexpr row_t const & operator [] (size_t i) const noexcept { return data[i]; }
	
	constexpr operator T const * () const { return &data[0][0]; }
};

//================================================================
//----------------------------------------------------------------
// MAT4_T
//----------------------------------------------------------------
//================================================================

template <typename T>
struct qm::meta::mat4_t {
	
	using row_t = std::array<T, 4>;
	using data_t = std::array<row_t, 4>;
	data_t data;
	
	// CONSTRUCT
	
	mat4_t() noexcept = default;
	constexpr mat4_t(mat4_t const &) noexcept = default;
	constexpr mat4_t(mat4_t &&) noexcept = default;
	
	constexpr mat4_t(
		T const & v00, T const & v01, T const & v02, T const & v03,
		T const & v10, T const & v11, T const & v12, T const & v13,
		T const & v20, T const & v21, T const & v22, T const & v23,
		T const & v30, T const & v31, T const & v32, T const & v33
	) noexcept : data {
		row_t {v00, v01, v02, v03},
		row_t {v10, v11, v12, v13},
		row_t {v20, v21, v22, v23},
		row_t {v30, v31, v32, v33},
	} {}
	
	constexpr mat4_t(mat3_t<T> const & m) : mat4_t {
		 m[0][0], m[0][1], m[0][2], 0,
		 m[1][0], m[1][1], m[1][2], 0,
		 m[2][0], m[2][1], m[2][2], 0,
		 0,       0,       0,       1
	} { }
	
	constexpr T * ptr() { return &data[0][0]; }
	constexpr T const * ptr() const { return &data[0][0]; }
	
	static constexpr mat4_t identity() noexcept {
		return {
			static_cast<T>(1), static_cast<T>(0), static_cast<T>(0), static_cast<T>(0),
			static_cast<T>(0), static_cast<T>(1), static_cast<T>(0), static_cast<T>(0),
			static_cast<T>(0), static_cast<T>(0), static_cast<T>(1), static_cast<T>(0),
			static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1),
		};
	}
	
	// COMMON
	
	static constexpr mat4_t multiply(mat4_t const & A, mat4_t const & B) noexcept {
		return {
			A[0][0] * B[0][0] + A[0][1] * B[1][0] + A[0][2] * B[2][0] + A[0][3] * B[3][0],
			A[0][0] * B[0][1] + A[0][1] * B[1][1] + A[0][2] * B[2][1] + A[0][3] * B[3][1],
			A[0][0] * B[0][2] + A[0][1] * B[1][2] + A[0][2] * B[2][2] + A[0][3] * B[3][2],
			A[0][0] * B[0][3] + A[0][1] * B[1][3] + A[0][2] * B[2][3] + A[0][3] * B[3][3],
			A[1][0] * B[0][0] + A[1][1] * B[1][0] + A[1][2] * B[2][0] + A[1][3] * B[3][0],
			A[1][0] * B[0][1] + A[1][1] * B[1][1] + A[1][2] * B[2][1] + A[1][3] * B[3][1],
			A[1][0] * B[0][2] + A[1][1] * B[1][2] + A[1][2] * B[2][2] + A[1][3] * B[3][2],
			A[1][0] * B[0][3] + A[1][1] * B[1][3] + A[1][2] * B[2][3] + A[1][3] * B[3][3],
			A[2][0] * B[0][0] + A[2][1] * B[1][0] + A[2][2] * B[2][0] + A[2][3] * B[3][0],
			A[2][0] * B[0][1] + A[2][1] * B[1][1] + A[2][2] * B[2][1] + A[2][3] * B[3][1],
			A[2][0] * B[0][2] + A[2][1] * B[1][2] + A[2][2] * B[2][2] + A[2][3] * B[3][2],
			A[2][0] * B[0][3] + A[2][1] * B[1][3] + A[2][2] * B[2][3] + A[2][3] * B[3][3],
			A[3][0] * B[0][0] + A[3][1] * B[1][0] + A[3][2] * B[2][0] + A[3][3] * B[3][0],
			A[3][0] * B[0][1] + A[3][1] * B[1][1] + A[3][2] * B[2][1] + A[3][3] * B[3][1],
			A[3][0] * B[0][2] + A[3][1] * B[1][2] + A[3][2] * B[2][2] + A[3][3] * B[3][2],
			A[3][0] * B[0][3] + A[3][1] * B[1][3] + A[3][2] * B[2][3] + A[3][3] * B[3][3],
		};
	}
	
	// SPECIAL
	
	static mat4_t<T> perspective(T vfov, T width, T height, T near, T far) {
		mat4_t<T> w = identity();
		T t1 = 1 / std::tan(0.5 * vfov);
		w[0][0] = t1 * (height / width);
		w[1][1] = t1;
		w[2][2] = (far + near) / (far - near);
		w[2][3] = static_cast<T>(1);
		w[3][2] = - (static_cast<T>(2) * far * near) / (far - near);
		w[3][3] = 0;
		return w;
	}
	
	static constexpr mat4_t<T> ortho(T t, T b, T l, T r, T n, T f) {
		mat4_t<T> w = identity();
		w[0][0] = static_cast<T>(2) / (r - l);
		w[1][1] = static_cast<T>(2) / (t - b);
		w[2][2] = static_cast<T>(-2) / (f - n);
		w[3][0] = - (r + l) / (r - l);
		w[3][1] = - (t + b) / (t - b);
		w[3][2] = - (f + n) / (f - n);
		return w;
	}
	
	constexpr mat4_t inverse() const noexcept {

		T const A2323 = data[2][2] * data[3][3] - data[2][3] * data[3][2] ;
		T const A1323 = data[2][1] * data[3][3] - data[2][3] * data[3][1] ;
		T const A1223 = data[2][1] * data[3][2] - data[2][2] * data[3][1] ;
		T const A0323 = data[2][0] * data[3][3] - data[2][3] * data[3][0] ;
		T const A0223 = data[2][0] * data[3][2] - data[2][2] * data[3][0] ;
		T const A0123 = data[2][0] * data[3][1] - data[2][1] * data[3][0] ;
		T const A2313 = data[1][2] * data[3][3] - data[1][3] * data[3][2] ;
		T const A1313 = data[1][1] * data[3][3] - data[1][3] * data[3][1] ;
		T const A1213 = data[1][1] * data[3][2] - data[1][2] * data[3][1] ;
		T const A2312 = data[1][2] * data[2][3] - data[1][3] * data[2][2] ;
		T const A1312 = data[1][1] * data[2][3] - data[1][3] * data[2][1] ;
		T const A1212 = data[1][1] * data[2][2] - data[1][2] * data[2][1] ;
		T const A0313 = data[1][0] * data[3][3] - data[1][3] * data[3][0] ;
		T const A0213 = data[1][0] * data[3][2] - data[1][2] * data[3][0] ;
		T const A0312 = data[1][0] * data[2][3] - data[1][3] * data[2][0] ;
		T const A0212 = data[1][0] * data[2][2] - data[1][2] * data[2][0] ;
		T const A0113 = data[1][0] * data[3][1] - data[1][1] * data[3][0] ;
		T const A0112 = data[1][0] * data[2][1] - data[1][1] * data[2][0] ;

		T const det = static_cast<T>(1) / (
			  data[0][0] * ( data[1][1] * A2323 - data[1][2] * A1323 + data[1][3] * A1223 ) 
			- data[0][1] * ( data[1][0] * A2323 - data[1][2] * A0323 + data[1][3] * A0223 ) 
			+ data[0][2] * ( data[1][0] * A1323 - data[1][1] * A0323 + data[1][3] * A0123 ) 
			- data[0][3] * ( data[1][0] * A1223 - data[1][1] * A0223 + data[1][2] * A0123 )
		);

		return {
			det *   ( data[1][1] * A2323 - data[1][2] * A1323 + data[1][3] * A1223 ),
			det * - ( data[0][1] * A2323 - data[0][2] * A1323 + data[0][3] * A1223 ),
			det *   ( data[0][1] * A2313 - data[0][2] * A1313 + data[0][3] * A1213 ),
			det * - ( data[0][1] * A2312 - data[0][2] * A1312 + data[0][3] * A1212 ),
			det * - ( data[1][0] * A2323 - data[1][2] * A0323 + data[1][3] * A0223 ),
			det *   ( data[0][0] * A2323 - data[0][2] * A0323 + data[0][3] * A0223 ),
			det * - ( data[0][0] * A2313 - data[0][2] * A0313 + data[0][3] * A0213 ),
			det *   ( data[0][0] * A2312 - data[0][2] * A0312 + data[0][3] * A0212 ),
			det *   ( data[1][0] * A1323 - data[1][1] * A0323 + data[1][3] * A0123 ),
			det * - ( data[0][0] * A1323 - data[0][1] * A0323 + data[0][3] * A0123 ),
			det *   ( data[0][0] * A1313 - data[0][1] * A0313 + data[0][3] * A0113 ),
			det * - ( data[0][0] * A1312 - data[0][1] * A0312 + data[0][3] * A0112 ),
			det * - ( data[1][0] * A1223 - data[1][1] * A0223 + data[1][2] * A0123 ),
			det *   ( data[0][0] * A1223 - data[0][1] * A0223 + data[0][2] * A0123 ),
			det * - ( data[0][0] * A1213 - data[0][1] * A0213 + data[0][2] * A0113 ),
			det *   ( data[0][0] * A1212 - data[0][1] * A0212 + data[0][2] * A0112 ),
		};
	}
	
	static constexpr mat4_t<T> scale(T v) {
		mat4_t<T> w = identity();
		w[0][0] = v;
		w[1][1] = v;
		w[2][2] = v;
		return w;
	}
	
	static constexpr mat4_t<T> scale(T x, T y, T z) {
		mat4_t<T> w = identity();
		w[0][0] = x;
		w[1][1] = y;
		w[2][2] = z;
		return w;
	}
	
	static constexpr mat4_t<T> scale(vec3_t<T> const & v) {
		mat4_t<T> w = identity();
		w[0][0] = v.data[0];
		w[1][1] = v.data[1];
		w[2][2] = v.data[2];
		return w;
	}
	
	static constexpr mat4_t<T> translate(T x, T y, T z) {
		mat4_t<T> w = identity();
		w[3][0] = x;
		w[3][1] = y;
		w[3][2] = z;
		return w;
	}
	
	static constexpr mat4_t<T> translate(vec3_t<T> const & v) {
		mat4_t<T> w = identity();
		w[3][0] = v.data[0];
		w[3][1] = v.data[1];
		w[3][2] = v.data[2];
		return w;
	}
	
	// SPECIAL
	
	constexpr mat3_t<T> top() const {
		return mat3_t<T> {
			data[0][0], data[0][1], data[0][2],
			data[1][0], data[1][1], data[1][2],
			data[2][0], data[2][1], data[2][2],
		};
	}
	
	// OPERATORS
	
	constexpr mat4_t & operator = (mat4_t const &) noexcept = default;
	constexpr mat4_t & operator = (mat4_t &&) noexcept = default;
	
	constexpr bool operator == (mat4_t const & other) const noexcept { return data == other.data; }
	constexpr bool operator != (mat4_t const & other) const noexcept { return ! operator == (other); }
	
	constexpr mat4_t operator * (mat4_t const & other) const noexcept { return mat4_t::multiply(*this, other); }
	
	constexpr mat4_t<T> & operator *= (mat4_t<T> const & other) {
		*this = mat4_t<T>::multiply(*this, other);
		return *this;
	}
	
	constexpr row_t & operator [] (size_t i) noexcept { return data[i]; }
	constexpr row_t const & operator [] (size_t i) const noexcept { return data[i]; }
	
	constexpr operator T const * () const { return &data[0][0]; }
};

//================================================================
//----------------------------------------------------------------
// CROSS-CLASS AND REVERSE OPERATIONS
//----------------------------------------------------------------
//================================================================

namespace qm::meta {
	
	template <typename T> constexpr vec2_t<T> operator + (T const & a, vec2_t<T> const & b) { return b + a; }
	template <typename T> constexpr vec3_t<T> operator + (T const & a, vec3_t<T> const & b) { return b + a; }
	template <typename T> constexpr vec4_t<T> operator + (T const & a, vec4_t<T> const & b) { return b + a; }
	template <typename T> constexpr vec2_t<T> operator * (T const & a, vec2_t<T> const & b) { return b * a; }
	template <typename T> constexpr vec3_t<T> operator * (T const & a, vec3_t<T> const & b) { return b * a; }
	template <typename T> constexpr vec4_t<T> operator * (T const & a, vec4_t<T> const & b) { return b * a; }
	
	template <typename T> 
	constexpr vec4_t<T> operator * (vec4_t<T> const & v, mat4_t<T> const & m) noexcept {
		return vec4_t {
			v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m [2][0] + v[3] * m[3][0],
			v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m [2][1] + v[3] * m[3][1],
			v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m [2][2] + v[3] * m[3][2],
			v[0] * m[0][3] + v[1] * m[1][3] + v[2] * m [2][3] + v[3] * m[3][3],
		};
	}
	
	template <typename T> 
	constexpr vec4_t<T> operator * (mat4_t<T> const & m, vec4_t<T> const & v) noexcept {
		return vec4_t {
			v[0] * m[0][0] + v[1] * m[0][1] + v[2] * m [0][2] + v[3] * m[0][3],
			v[0] * m[1][0] + v[1] * m[1][1] + v[2] * m [1][2] + v[3] * m[1][3],
			v[0] * m[2][0] + v[1] * m[2][1] + v[2] * m [2][2] + v[3] * m[2][3],
			v[0] * m[3][0] + v[1] * m[3][1] + v[2] * m [3][2] + v[3] * m[3][3],
		};
	}

	template <typename T>
	constexpr vec3_t<T> operator * (quat_t<T> const & q, vec3_t<T> v) noexcept {
		vec3_t<T> qv {q[0], q[1], q[2]};
		vec3_t<T> t = static_cast<T>(2) * vec3_t<T>::cross(qv, v);
		return v + q[3] * t + vec3_t<T>::cross(qv, t);
	}
}

//================================================================
//----------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------
//================================================================


namespace qm {
	static inline constexpr vec3_t origin {  0,  0,  0 };

	static inline constexpr vec3_t cardinal_xp {  1,  0,  0 };
	static inline constexpr vec3_t cardinal_xn { -1,  0,  0 };
	static inline constexpr vec3_t cardinal_yp {  0,  1,  0 };
	static inline constexpr vec3_t cardinal_yn {  0, -1,  0 };
	static inline constexpr vec3_t cardinal_zp {  0,  0,  1 };
	static inline constexpr vec3_t cardinal_zn {  0,  0, -1 };
	
	static inline constexpr vec3_t intercardinal_xpyp {  0.707,  0.707, 0 };
	static inline constexpr vec3_t intercardinal_xpyn {  0.707, -0.707, 0 };
	static inline constexpr vec3_t intercardinal_xnyp { -0.707,  0.707, 0 };
	static inline constexpr vec3_t intercardinal_xnyn { -0.707, -0.707, 0 };
}

//================================================================
//----------------------------------------------------------------
// TESTS
//----------------------------------------------------------------
//================================================================


#ifndef QMATH2_ASSERT_TEST_TYPE
#define QMATH2_ASSERT_TEST_TYPE double
#endif

// size asserts
static_assert( sizeof(qm::meta::vec2_t<QMATH2_ASSERT_TEST_TYPE>) == sizeof(QMATH2_ASSERT_TEST_TYPE) * 2 );
static_assert( sizeof(qm::meta::vec3_t<QMATH2_ASSERT_TEST_TYPE>) == sizeof(QMATH2_ASSERT_TEST_TYPE) * 3 );
static_assert( sizeof(qm::meta::vec4_t<QMATH2_ASSERT_TEST_TYPE>) == sizeof(QMATH2_ASSERT_TEST_TYPE) * 4 );
//static_assert( sizeof(qm::meta::rect_t<QMATH2_ASSERT_TEST_TYPE>) == sizeof(QMATH2_ASSERT_TEST_TYPE) * 4 );
//static_assert( sizeof(qm::meta::quat_t<QMATH2_ASSERT_TEST_TYPE>) == sizeof(QMATH2_ASSERT_TEST_TYPE) * 4 );
//static_assert( sizeof(qm::meta::mat3_t<QMATH2_ASSERT_TEST_TYPE>) == sizeof(QMATH2_ASSERT_TEST_TYPE) * 9 );
static_assert( sizeof(qm::meta::mat4_t<QMATH2_ASSERT_TEST_TYPE>) == sizeof(QMATH2_ASSERT_TEST_TYPE) * 16 );

// POD asserts
static_assert( std::is_standard_layout<qm::meta::vec2_t<QMATH2_ASSERT_TEST_TYPE>>() && std::is_trivial<qm::meta::vec2_t<QMATH2_ASSERT_TEST_TYPE>>() );
static_assert( std::is_standard_layout<qm::meta::vec3_t<QMATH2_ASSERT_TEST_TYPE>>() && std::is_trivial<qm::meta::vec3_t<QMATH2_ASSERT_TEST_TYPE>>() );
static_assert( std::is_standard_layout<qm::meta::vec4_t<QMATH2_ASSERT_TEST_TYPE>>() && std::is_trivial<qm::meta::vec4_t<QMATH2_ASSERT_TEST_TYPE>>() );
static_assert( std::is_standard_layout<qm::meta::mat4_t<QMATH2_ASSERT_TEST_TYPE>>() && std::is_trivial<qm::meta::mat4_t<QMATH2_ASSERT_TEST_TYPE>>() );

//static_assert( std::is_pod<qm::meta::rect_t<QMATH2_ASSERT_TEST_TYPE>>() );
//static_assert( std::is_pod<qm::meta::quat_t<QMATH2_ASSERT_TEST_TYPE>>() );
//static_assert( std::is_pod<qm::meta::mat3_t<QMATH2_ASSERT_TEST_TYPE>>() );

#undef QMATH2_ASSERT_TEST_TYPE

#if QMATH2_HASH_FUNCTIONS == 1
namespace std {
	template <typename T> struct hash<qm::meta::vec2_t<T>> {
		constexpr size_t operator () (qm::meta::vec2_t<T> const & v) const {
			return std::hash(v[0]) + std::hash(v[1]);
		}
	};
	template <typename T> struct hash<qm::meta::vec3_t<T>> {
		constexpr size_t operator () (qm::meta::vec3_t<T> const & v) const {
			return std::hash(v[0]) + std::hash(v[1]) + std::hash(v[2]);
		}
	};
	template <typename T> struct hash<qm::meta::vec4_t<T>> {
		constexpr size_t operator () (qm::meta::vec4_t<T> const & v) const {
			return std::hash(v[0]) + std::hash(v[1]) + std::hash(v[2]) + std::hash(v[3]);
		}
	};
	/*
	template <typename T> struct hash<qm::meta::rect_t<T>> {
		size_t operator () (qm::meta::rect_t<T> const & v) const {
			return std::hash(v.origin) + std::hash(v.extents);
		}
	};
	template <typename T> struct hash<qm::meta::quat_t<T>> {
		size_t operator () (qm::meta::quat_t<T> const & v) const {
			return std::hash(v[0]) + std::hash(v[1]) + std::hash(v[2]) + std::hash(v[3]);
		}
	};
	template <typename T> struct hash<qm::meta::mat3_t<T>> {
		size_t operator () (qm::meta::mat3_t<T> const & v) const {
			size_t h = 0;
			for (size_t i = 0; i < 3; i++) for (size_t j = 0; j < 3; j++)
				h += std::hash(v[i][j]);
			return h;
		}
	};
	*/
	template <typename T> struct hash<qm::meta::mat4_t<T>> {
		constexpr size_t operator () (qm::meta::mat4_t<T> const & v) const {
			size_t h = 0;
			for (size_t i = 0; i < 4; i++) for (size_t j = 0; j < 4; j++)
				h += std::hash(v[i][j]);
			return h;
		}
	};
}
#endif

//================================================================
//----------------------------------------------------------------
//================================================================
