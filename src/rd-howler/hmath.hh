#pragma once

#include <cstdint>
#include <ctgmath>
#include <numeric>
#include <vector>
#include <string>
#include <functional>

namespace math {
	
	typedef int_fast64_t work_int_t;
	typedef uint_fast64_t work_uint_t;
	typedef long double work_float_t;
	
	template <typename T> T constexpr pi = static_cast<T>(3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844609550582231725359408128481117450284102701938521105559644622948954930381964428810975665933446128475648233786783165271201909145649L);
	template <typename T> T deg2rad(T const & v) { return v * pi<T> / static_cast<T>(180); }
	template <typename T> T rad2deg(T const & v) { return v / pi<T> * static_cast<T>(180); }
	
	template <typename T> T const & min(T const & A, T const & B) { return A > B ? B : A; }
	template <typename T> T & min(T & A, T & B) { return A > B ? B : A; }
	template <typename T> T const & max(T const & A, T const & B) { return A > B ? A : B; }
	template <typename T> T & max(T & A, T & B) { return A > B ? A : B; }
	template <typename T> void clamp(T & V, T const & MIN, T const & MAX) { if (V > MAX) V = MAX; else if (V < MIN) V = MIN; }
	template <typename T> T clamped(T const & V, T const & MIN, T const & MAX) { if (V > MAX) return MAX; else if (V < MIN) return MIN; else return V; }
	template <typename T, typename L = double> T lerp(T const & A, T const & B, L v) {
		return (1 - v) * A + v * B;
	}

//================================================================================================
//------------------------------------------------------------------------------------------------
//================================================================================================
	
	template <typename T> struct vec2_t;
	template <typename T> struct vec3_t;
	template <typename T> struct vec4_t;
	
	template <typename T> struct rect_t;
	
	template <typename T> struct quaternion_t;
	
	template <typename T> struct mat3_t;
	template <typename T> struct mat4_t;
	
//================================================================================================
//------------------------------------------------------------------------------------------------
//================================================================================================
	
	template <typename T> struct vec2_t {
		
		typedef T length_t;
		T data [2] {0, 0};
		
		constexpr T & x() { return data[0]; }
		constexpr T & y() { return data[1]; }
		constexpr T & width() { return data[0]; }
		constexpr T & height() { return data[1]; }
		constexpr T const & x() const { return data[0]; }
		constexpr T const & y() const { return data[1]; }
		constexpr T const & width() const { return data[0]; }
		constexpr T const & height() const { return data[1]; }
		
		constexpr vec2_t() = default;
		constexpr vec2_t(T x, T y) : data {x, y} {}
		constexpr explicit vec2_t(T v) : data {v, v} {}
		constexpr vec2_t(vec2_t const &) = default;
		constexpr vec2_t(vec2_t &&) = default;
		template <typename U> constexpr vec2_t(vec2_t<U> const & other) { data[0] = other.data[0]; data[1] = other.data[1]; }
		
		static constexpr T dot(vec2_t<T> const & A, vec2_t<T> const & B) {
			return A.data[0] * B.data[0] + A.data[1] * B.data[1];
		}
		
		inline T magnitude() const { return static_cast<T>(sqrt( (data[0] * data[0]) + (data[1] * data[1]) )); }
		
		inline T & normalize() {
			T mag = magnitude();
			data[0] /= mag;
			data[1] /= mag;
			return *this;
		}
		
		inline T normalized() {
			T mag = magnitude();
			return { data[0]/mag, data[1]/mag };
		}
		
		constexpr vec2_t<T> & operator = (vec2_t<T> const & other) = default;
		
		constexpr bool operator == (vec2_t<T> const & other) const { return data[0] == other.data[0] && data[1] == other.data[1]; }
		
		constexpr vec2_t<T> operator + (vec2_t<T> const & other) const { return {data[0] + other.data[0], data[1] + other.data[1]}; }
		constexpr vec2_t<T> operator + (T const & value) const { return {data[0] + value, data[1] + value}; }
		constexpr vec2_t<T> operator - (vec2_t<T> const & other) const { return {data[0] - other.data[0], data[1] - other.data[1]}; }
		constexpr vec2_t<T> operator - (T const & value) const { return {data[0] - value, data[1] - value}; }
		constexpr T operator * (vec2_t<T> const & other) const { return dot(*this, other); }
		constexpr vec2_t<T> operator * (T const & value) const { return { data[0] * value, data[1] * value }; }
		constexpr vec2_t<T> operator / (T const & value) const { return { data[0] / value, data[1] / value }; }
		
		constexpr vec2_t<T> & operator += (vec2_t<T> const & other) { data[0] += other.data[0]; data[1] += other.data[1]; return *this; }
		constexpr vec2_t<T> & operator += (T const & value) { data[0] += value; data[1] += value; return *this; }
		constexpr vec2_t<T> & operator -= (vec2_t<T> const & other) { data[0] -= other.data[0]; data[1] -= other.data[1]; return *this; }
		constexpr vec2_t<T> & operator -= (T const & value) { data[0] -= value; data[1] -= value; return *this; }
		constexpr vec2_t<T> & operator *= (T const & value) { data[0] *= value; data[1] *= value; return *this; }
		constexpr vec2_t<T> & operator /= (T const & value) { data[0] /= value; data[1] /= value; return *this; }
		
		template <typename U> constexpr vec2_t<T> operator = (vec2_t<U> const & other) { data[0] = other.data[0]; data[1] = other.data[1]; return *this; }
		
		constexpr T & operator [] (size_t i) { return data[i]; }
		constexpr T const & operator [] (size_t i) const { return data[i]; }
		
#if BRASSICA_PRINT_FUNCTIONS == 1
		std::string to_string() const {
			return "V2[" + std::to_string(data[0]) + ", " + std::to_string(data[1]) + "]";
		}
#endif
	};
	
//================================================================================================
//------------------------------------------------------------------------------------------------
//================================================================================================
	
	template <typename T> struct vec3_t {
		
		typedef T length_t;
		T data [3] {0, 0, 0};
		constexpr T & x() { return data[0]; }
		constexpr T & y() { return data[1]; }
		constexpr T & z() { return data[2]; }
		constexpr T const & x() const { return data[0]; }
		constexpr T const & y() const { return data[1]; }
		constexpr T const & z() const { return data[2]; }
		constexpr T & yaw() { return data[0]; }
		constexpr T & pitch() { return data[1]; }
		constexpr T & roll() { return data[2]; }
		constexpr T const & yaw() const { return data[0]; }
		constexpr T const & pitch() const { return data[1]; }
		constexpr T const & roll() const { return data[2]; }
		constexpr T & r() { return data[0]; }
		constexpr T & g() { return data[1]; }
		constexpr T & b() { return data[2]; }
		constexpr T const & r() const { return data[0]; }
		constexpr T const & g() const { return data[1]; }
		constexpr T const & b() const { return data[2]; }
		
		constexpr vec3_t() = default;
		constexpr vec3_t(T x, T y, T z) : data {x, y, z} {}
		constexpr explicit vec3_t(T v) : data {v, v, v} {}
		constexpr vec3_t(vec3_t const &) = default;
		constexpr vec3_t(vec3_t &&) = default;
		template <typename U> constexpr vec3_t(vec3_t<U> const & other) { data[0] = other.data[0]; data[1] = other.data[1]; data[2] = other.data[2]; }
		constexpr vec3_t(vec2_t<T> const & v, T z) : data {v.data[0], v.data[1], z} {}
		
		static constexpr T dot(vec3_t<T> const & A, vec3_t<T> const & B) {
			return A.data[0] * B.data[0] + A.data[1] * B.data[1] + A.data[2] * B.data[2];
		}
		
		static constexpr vec3_t<T> cross(vec3_t<T> const & A, vec3_t<T> const & B) {
			return {
				A.data[1] * B.data[2] - A.data[2] * B.data[1],
				A.data[2] * B.data[0] - A.data[0] * B.data[2],
				A.data[0] * B.data[1] - A.data[1] * B.data[0],
			};
		}
		
		inline T magnitude() const { return static_cast<T>(sqrt((data[0]*data[0])+(data[1]*data[1])+(data[2]*data[2]))); }
		
		inline vec3_t<T> & normalize() {
			T mag = magnitude();
			data[0] /= mag;
			data[1] /= mag;
			data[2] /= mag;
			return *this;
		}
		
		inline vec3_t<T> normalized() const {
			T mag = magnitude();
			return { data[0]/mag, data[1]/mag, data[2]/mag };
		}
		
		constexpr bool is_null() const {
			return data[0] == 0 && data[1] == 0 && data[2] == 0;
		}
		
		constexpr bool is_cardinal() const {
			bool cv = false;
			for (size_t i = 0; i < 3; i++) {
				switch (data[i]) {
					case 0:
						break;
					case -1:
					case 1:
						if (cv) return false;
						cv = true;
						break;
					default:
						return false;
				}
			}
			return cv;
		}
		
		constexpr vec3_t<T> & operator = (vec3_t<T> const & other) = default;
		constexpr vec3_t<T> & operator = (vec3_t<T> && other) = default;
		
		constexpr bool operator == (vec3_t<T> const & other) const { return data[0] == other.data[0] && data[1] == other.data[1] && data[2] == other.data[2]; }
		constexpr vec3_t<T> operator + (vec3_t<T> const & other) const { return { data[0] + other.data[0], data[1] + other.data[1], data[2] + other.data[2] }; }
		constexpr vec3_t<T> operator - (vec3_t<T> const & other) const { return { data[0] - other.data[0], data[1] - other.data[1], data[2] - other.data[2] }; }
		constexpr vec3_t<T> operator * (mat4_t<T> const & mat) const {
			return {
				data[0] * mat[0][0] + data[1] * mat[1][0] + data[2] * mat[2][0] + mat[3][0],
				data[0] * mat[0][1] + data[1] * mat[1][1] + data[2] * mat[2][1] + mat[3][1],
				data[0] * mat[0][2] + data[1] * mat[1][2] + data[2] * mat[2][3] + mat[3][2],
			};
		}
		
		constexpr vec3_t<T> & operator += (vec3_t<T> const & other) { data[0] += other.data[0]; data[1] += other.data[1]; data[2] += other.data[2]; return *this; }
		constexpr vec3_t<T> & operator += (T const & value) { data[0] += value; data[1] += value; data[2] += value; return *this; }
		constexpr vec3_t<T> & operator -= (vec3_t<T> const & other) { data[0] -= other.data[0]; data[1] -= other.data[1]; data[2] -= other.data[2]; return *this; }
		constexpr vec3_t<T> & operator -= (T const & value) { data[0] -= value; data[1] -= value; data[2] -= value; return *this; }
		constexpr vec3_t<T> & operator *= (T const & value) { data[0] *= value; data[1] *= value; data[2] *= value; return *this; }
		constexpr vec3_t<T> & operator *= (mat4_t<T> const & mat) {
			operator = ( operator * (mat) );
			return *this;
		}
		constexpr vec3_t<T> & operator /= (T const & value) { data[0] /= value; data[1] /= value; data[2] /= value; return *this; }
		
		template <typename U> constexpr vec3_t<T> & operator = (vec3_t<U> const & other) { data[0] = other.data[0]; data[1] = other.data[1]; data[2] = other.data[2]; return *this; }
		
		constexpr T & operator [] (size_t i) { return data[i]; }
		constexpr T const & operator [] (size_t i) const { return data[i]; }
		
		constexpr vec3_t<T> operator - () const { return {-x(), -y(), -z()}; }
		
#if BRASSICA_PRINT_FUNCTIONS == 1
		std::string to_string() const {
			return "V3[" + std::to_string(data[0]) + ", " + std::to_string(data[1]) + ", " + std::to_string(data[2]) + "]";
		}
#endif
	};
	
	template <typename T, typename U> constexpr vec3_t<T> operator + (vec3_t<T> const & A, U const & value) { 
		return { A[0] + value, A[1] + value, A[2] + value };
	}
	template <typename T, typename U> constexpr vec3_t<T> operator + (U const & value, vec3_t<T> const & A) { 
		return { A[0] + value, A[1] + value, A[2] + value };
	}
	template <typename T, typename U> constexpr vec3_t<T> operator - (vec3_t<T> const & A, U const & value) { 
		return { A[0] - value, A[1] - value, A[2] - value };
	}
	template <typename T, typename U> constexpr vec3_t<T> operator * (vec3_t<T> const & A, U const & value) { 
		return { A[0] * value, A[1] * value, A[2] * value };
	}
	template <typename T, typename U> constexpr vec3_t<T> operator * (U const & value, vec3_t<T> const & A) { 
		return { A[0] * value, A[1] * value, A[2] * value };
	}
	template <typename T, typename U> constexpr vec3_t<T> operator / (vec3_t<T> const & A, U const & value) { 
		return { A[0] / value, A[1] / value, A[2] / value };
	}
	
//================================================================================================
//------------------------------------------------------------------------------------------------
//================================================================================================
	
	template <typename T> struct vec4_t {
		
		typedef T length_t;
		T data [4] {0, 0, 0, 0};
		constexpr T & x() { return data[0]; }
		constexpr T & y() { return data[1]; }
		constexpr T & z() { return data[2]; }
		constexpr T & w() { return data[3]; }
		constexpr T const & x() const { return data[0]; }
		constexpr T const & y() const { return data[1]; }
		constexpr T const & z() const { return data[2]; }
		constexpr T const & w() const { return data[3]; }
		constexpr T & r() { return data[0]; }
		constexpr T & g() { return data[1]; }
		constexpr T & b() { return data[2]; }
		constexpr T & a() { return data[3]; }
		constexpr T const & r() const { return data[0]; }
		constexpr T const & g() const { return data[1]; }
		constexpr T const & b() const { return data[2]; }
		constexpr T const & a() const { return data[3]; }
		
		constexpr vec4_t() = default;
		constexpr vec4_t(T x, T y, T z, T w) : data {x, y, z, w} {}
		constexpr explicit vec4_t(T v) : data {v, v, v, v} {}
		constexpr vec4_t(vec4_t const &) = default;
		constexpr vec4_t(vec4_t &&) = default;
		template <typename U> constexpr vec4_t(vec4_t<U> const & other) { data[0] = other.data[0]; data[1] = other.data[1]; data[2] = other.data[2]; data[3] = other.data[3]; }
		constexpr vec4_t(vec3_t<T> const & v, T w) : data {v.data[0], v.data[1], v.data[2], w} {}
		
		static constexpr T dot(vec4_t<T> const & A, vec4_t<T> const & B) {
			return A.data[0] * B.data[0] + A.data[1] * B.data[1] + A.data[2] * B.data[2] + A.data[3] * B.data[3];
		}
		
		inline T magnitude() const { return static_cast<T>(sqrt((data[0]*data[0])+(data[1]*data[1])+(data[2]*data[2])+(data[3]*data[3]))); }
		
		inline T & normalize() {
			T mag = magnitude();
			data[0] /= mag;
			data[1] /= mag;
			data[2] /= mag;
			data[3] /= mag;
			return *this;
		}
		
		inline T normalized() {
			T mag = magnitude();
			return { data[0]/mag, data[1]/mag, data[2]/mag, data[3]/mag };
		}
		
		constexpr vec3_t<T> xyz() const { return vec3_t<T> { data[0], data[1], data[2] }; }
		
		constexpr vec4_t<T> & operator = (vec4_t<T> const & other) = default;
		constexpr vec4_t<T> & operator = (vec4_t<T> && other) = default;
		
		constexpr bool operator == (vec4_t<T> const & other) const { return data[0] == other.data[0] && data[1] == other.data[1] && data[2] == other.data[2] && data[3] == other.data[3]; }
		constexpr vec4_t<T> operator + (vec4_t<T> const & other) const { return { data[0] + other.data[0], data[1] + other.data[1], data[2] + other.data[2], data[3] + other.data[3] }; }
		constexpr vec4_t<T> operator + (T const & value) const { return { data[0] + value, data[1] + value, data[2] + value, data[3] + value }; }
		constexpr vec4_t<T> operator - (vec4_t<T> const & other) const { return { data[0] - other.data[0], data[1] - other.data[1], data[2] - other.data[2], data[3] - other.data[3] }; }
		constexpr vec4_t<T> operator - (T const & value) const { return { data[0] - value, data[1] - value, data[2] - value, data[3] - value }; }
		constexpr vec4_t<T> operator * (T const & value) const { return { data[0] * value, data[1] * value, data[2] * value, data[3] * value }; }
		constexpr vec4_t<T> operator * (mat4_t<T> const & mat) const {
			return {
				data[0] * mat[0][0] + data[1] * mat[1][0] + data[2] * mat[2][0] + data[3] * mat[3][0],
				data[0] * mat[0][1] + data[1] * mat[1][1] + data[2] * mat[2][1] + data[3] * mat[3][1],
				data[0] * mat[0][2] + data[1] * mat[1][2] + data[2] * mat[2][2] + data[3] * mat[3][2],
				data[0] * mat[0][3] + data[1] * mat[1][3] + data[2] * mat[2][3] + data[3] * mat[3][3],
			};
		}
		constexpr vec4_t<T> operator / (T const & value) const { return { data[0] / value, data[1] / value, data[2] / value, data[3] / value }; }
		
		constexpr vec4_t<T> & operator += (vec4_t<T> const & other) { data[0] += other.data[0]; data[1] += other.data[1]; data[2] += other.data[2]; data[3] += other.data[3]; return *this; }
		constexpr vec4_t<T> & operator += (T const & value) { data[0] += value; data[1] += value; data[2] += value; data[3] += value; return *this; }
		constexpr vec4_t<T> & operator -= (vec4_t<T> const & other) { data[0] -= other.data[0]; data[1] -= other.data[1]; data[2] -= other.data[2]; data[3] -= other.data[3]; return *this; }
		constexpr vec4_t<T> & operator -= (T const & value) { data[0] -= value; data[1] -= value; data[2] -= value; data[3] -= value; return *this; }
		constexpr vec4_t<T> & operator *= (T const & value) { data[0] *= value; data[1] *= value; data[2] *= value; data[3] *= value; return *this; }
		constexpr vec4_t<T> & operator *= (mat4_t<T> const & mat) {
			operator = ( operator * (mat) );
			return *this;
		}
		constexpr vec4_t<T> & operator /= (T const & value) { data[0] /= value; data[1] /= value; data[2] /= value; data[3] /= value; return *this; }
		
		template <typename U> constexpr vec4_t<T> & operator = (vec4_t<U> const & other) { data[0] = other.data[0]; data[1] = other.data[1]; data[2] = other.data[2]; data[3] = other.data[3]; return *this; }
		
		constexpr T & operator [] (size_t i) { return data[i]; }
		constexpr T const & operator [] (size_t i) const { return data[i]; }
		
		constexpr operator T const * () const { return &data[0]; }
		
		constexpr vec4_t<T> operator - () const { return {-x(), -y(), -z(), -w()}; }
		
#if BRASSICA_PRINT_FUNCTIONS == 1
		std::string to_string() const {
			return "V4[" + std::to_string(data[0]) + ", " + std::to_string(data[1]) + ", " + std::to_string(data[2]) + ", " + std::to_string(data[3]) + "]";
		}
#endif
	};
	
//===========================================================================================m=====
//------------------------------------------------------------------------------------------------
//================================================================================================
	
	template <typename T> struct rect_t {
		
		typedef T length_t;
		vec2_t<T> origin {};
		vec2_t<T> extents {};
		
		inline T & x() { return origin.x(); }
		inline T & y() { return origin.y(); }
		inline T & width() { return extents.width(); }
		inline T & height() { return extents.height(); }
		inline T const & x() const { return origin.x(); }
		inline T const & y() const { return origin.y(); }
		inline T const & width() const { return extents.width(); }
		inline T const & height() const { return extents.height(); }
		inline T ratio() const { return width() / height(); }
		
		inline rect_t() = default;
		inline rect_t(T width, T height) : extents(width, height) {}
		inline rect_t(T x, T y, T width, T height) : origin(x, y), extents(width, height) {}
		inline rect_t(vec2_t<T> const & extents) : extents(extents) {}
		inline rect_t(vec2_t<T> const & origin, vec2_t<T> const & extents) : origin(origin), extents(extents) {}
		inline rect_t(vec4_t<T> const & rect) : origin(rect[0], rect[1]), extents(rect[2], rect[3]) {}
		inline rect_t(rect_t<T> const & other) = default;
		inline rect_t(rect_t<T> && other) = default;
		
		inline void center_in_x(rect_t<T> const & c) {
			origin[0] = c.origin[0] + c.extents[0] / static_cast<T>(2) - extents[0] / static_cast<T>(2);
		}
		inline void center_in_y(rect_t<T> const & c) {
			origin[1] = c.origin[1] + c.extents[1] / static_cast<T>(2) - extents[1] / static_cast<T>(2);
		}
		inline void center_in(rect_t<T> const & c) {
			center_in_x(c);
			center_in_y(c);
		}
		
		inline void fit_in(rect_t<T> const & c) {
			T fratio = ratio();
			if (fratio > c.ratio()) {
				extents[0] = c.width();
				extents[1] = c.width() / fratio;
			} else {
				extents[0] = c.height() * fratio;
				extents[1] = c.height();
			}
			center_in(c);
		}
		
		inline void bound_in_x(rect_t<T> const & c) {
			if (extents[0] < c.extents[0]) {
				if (origin[0] < c.origin[0]) origin[0] = c.origin[0];
				else if (origin[0] > c.origin[0] + c.extents[0] - extents[0]) origin[0] = c.origin[0] + c.extents[0] - extents[0];
			} else if (extents[0] > c.extents[0]) {
				if (origin[0] > c.origin[0]) origin[0] = c.origin[0];
				else if (origin[0] < c.origin[0] + c.extents[0] - extents[0]) origin[0] = c.origin[0] + c.extents[0] - extents[0];
			} else {
				origin[0] = c.origin[0];
			}
		}
		inline void bound_in_y(rect_t<T> const & c) {
			if (extents[1] < c.extents[1]) {
				if (origin[1] < c.origin[1]) origin[1] = c.origin[1];
				else if (origin[1] > c.origin[1] + c.extents[1] - extents[1]) origin[1] = c.origin[1] + c.extents[1] - extents[1];
			} else if (extents[1] > c.extents[1]) {
				if (origin[1] > c.origin[1]) origin[1] = c.origin[1];
				else if (origin[1] < c.origin[1] + c.extents[1] - extents[1]) origin[1] = c.origin[1] + c.extents[1] - extents[1];
			} else {
				origin[1] = c.origin[1];
			}
		}
		inline void bound_in(rect_t<T> const & c) {
			bound_in_x(c);
			bound_in_y(c);
		}
		
		/*
		inline rect_t<T> fit_shrink_in(rect_t<T> const & container) const {
			T fratio = ratio(), cratio = container.ratio();
			if (fratio >= cratio && width() <= container.width()) return center_in(container);
			if (fratio < cratio && height() <= container.height()) return center_in(container);
			return fit_in(container);
		}
		
		inline rect_t<T> fit_grow_in(rect_t<T> const & container) const {
			T fratio = ratio(), cratio = container.ratio();
			if (fratio >= cratio && width() >= container.width()) return center_in(container);
			if (fratio < cratio && height() >= container.height()) return center_in(container);
			return fit_in(container);
		}
		*/
		
		inline rect_t<T> & offset(vec2_t<T> const & v) { origin += v; return *this; }
		inline rect_t<T> offsetted(vec2_t<T> const & v) const { return { origin + v, extents }; }
		
		inline rect_t<T> & scale(T const & v) { extents *= v; return *this; }
		inline rect_t<T> scaled(T const & v) const { return {origin, extents * v}; }
		
		inline rect_t<T> & operator = (rect_t<T> const & other) = default;
		inline bool operator == (rect_t<T> const & other) const { return origin == other.origin && extents = other.extents; }
		
		template <typename U> rect_t<T> & operator = (rect_t<U> const & other) { origin = other.origin; extents = other.extents; return *this; }
		
		inline operator vec4_t<T> () const { return {origin[0], origin[1], extents[0], extents[1]}; }
		template<typename U> inline operator vec4_t<U> () const { return { static_cast<U>(origin[0]), static_cast<U>(origin[1]), static_cast<U>(origin[2]), static_cast<U>(origin[3])}; }
		
#if BRASSICA_PRINT_FUNCTIONS == 1
		std::string to_string() const {
			return "RECT[" + origin.to_string() + ", " + extents.to_string() + "]";
		}
#endif
	};
	
//================================================================================================
//------------------------------------------------------------------------------------------------
//================================================================================================
	
	template <typename T> struct quaternion_t {
		T data [4] {0, 0, 0, 1};
		constexpr T & x() { return data[0]; }
		constexpr T & y() { return data[1]; }
		constexpr T & z() { return data[2]; }
		constexpr T & w() { return data[3]; }
		
		constexpr quaternion_t() = default;
		quaternion_t(T const & x, T const & y, T const & z, T const & w) : data{x, y, z, w} { normalize(); }
		quaternion_t(vec3_t<T> const & axis, T const & angle) {
			T a = angle/2;
			T s = std::sin(a);
			data[0] = axis[0] * s;
			data[1] = axis[1] * s;
			data[2] = axis[2] * s;
			data[3] = std::cos(a);
			normalize();
		}
		constexpr quaternion_t(quaternion_t const &) = default;
		constexpr quaternion_t(quaternion_t &&) = default;
		
		// ================
		// COMMON
		// ================
		
		constexpr quaternion_t<T> conjugate() const {
			return {-data[0], -data[1], -data[2], data[3]};
		}
		
		inline quaternion_t<T> reciprocal() const {
			T m = norm();
			return {-data[0] / m, -data[1] / m, -data[2] / m, data[3] / m};
		}
		
		constexpr T product() const {
			return data[0] * data[0] + data[1] * data[1] + data[2] * data[2] + data[3] * data[3];
		}
		
		inline T norm() const {
			return std::sqrt(product());
		}
		
		inline quaternion_t<T> & normalize () {
			T v = norm();
			data[0] /= v;
			data[1] /= v;
			data[2] /= v;
			data[3] /= v;
			return *this;
		}
		
		inline quaternion_t<T> normalized () const {
			T v = norm();
			return { data[0] / v, data[1] / v, data[2] / v, data[3] / v };
		}
		
		inline bool is_null () const {
			return data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 1;
		}
		
		// ================
		// CORRECTIONS -- always return quaternion needed to reach destination, not destination itself
		// ================
		
		// LIMITERS
		
		quaternion_t<T> range_limit_up(vec3_t<T> const & up_pre, T angle_lim, T lerp = static_cast<T>(1)) const {
			vec3_t<T> up_q = -up_pre * conjugate();
			up_q.normalize();
			T dot = vec3_t<T>::dot({0, 1, 0}, up_q);
			if (dot >= 1) return {};
			if (dot < -1) dot = -1;
			T angle = std::acos(dot);
			if (angle > angle_lim) return {};
			vec3_t<T> rotaxis = vec3_t<T>::cross({0, 1, 0}, up_q);
			rotaxis.normalize();
			return quaternion_t<T> { rotaxis, (angle_lim - angle) * lerp };
		}
		
		// SEEKERS
		
		quaternion_t<T> roll_up(vec3_t<T> const & up_pre, T lerp = static_cast<T>(1)) const {
			
			vec3_t<T> up_q = up_pre * conjugate();
			up_q.normalize();
			vec3_t<T> side = vec3_t<T>::cross({0, 0, 1}, up_q);
			side.normalize();
			if (vec3_t<T>::dot({0, 1, 0}, up_q) > 0) side = -side;
			vec3_t<T> up = vec3_t<T>::cross({0, 0, 1}, side);
			up.normalize();
			
			return vector_delta({0, 1, 0}, up, lerp);
		}
		
		quaternion_t<T> look_at(vec3_t<T> const & origin, vec3_t<T> const & target, T lerp = static_cast<T>(1)) const {
			vec3_t<T> front_to = vec3_t<T> { target - origin } .normalized() * conjugate();
			return vector_delta(vec3_t<T> { 0, 0, 1 }, front_to, lerp);
		}
		
		// ================
		// GENERATORS
		// ================
		
		static quaternion_t<T> vector_delta(vec3_t<T> from, vec3_t<T> to, T lerp = static_cast<T>(1)) {
			
			#if BRASSICA_INPUT_SANITIZING == 1
			if (lerp < 0) lerp = 0;
			if (lerp > 1) lerp = 1;
			#endif
			
			T dot = vec3_t<T>::dot(to, from);
			if (dot > 1) dot = 1;
			if (dot < -1) dot = -1;
			if (dot == 1) return quaternion_t<T> {0, 0, 0, 1};
			if (dot == -1) return quaternion_t<T> {0, 0, 1, 0};
			
			T rot_val = std::acos(dot);
			
			vec3_t<T> rotaxis = vec3_t<T>::cross(to, from);
			rotaxis.normalize();
			
			return quaternion_t<T> { rotaxis, rot_val * lerp };
		}
		
		static quaternion_t<T> look_at(vec3_t<T> const & origin, vec3_t<T> const & target, vec3_t<T> const & up_in) {
			vec3_t<T> front { target - origin };
			front.normalize();
			vec3_t<T> side = vec3_t<T>::cross(front, -up_in);
			side.normalize();
			vec3_t<T> up = vec3_t<T>::cross(front, side);
			up.normalize();
			
			T m [3][3];
			m[0][0] = side[0];
			m[0][1] = side[1];
			m[0][2] = side[2];
			m[1][0] = up[0];
			m[1][1] = up[1];
			m[1][2] = up[2];
			m[2][0] = front[0];
			m[2][1] = front[1];
			m[2][2] = front[2];
			
			T trace = side[0] + up[1] + front[2];
			
			if (trace > 0) {
				T root = static_cast<T>(2) * std::sqrt(trace + static_cast<T>(1));
				return { 
					(m[2][1] - m[1][2]) / root,
					(m[0][2] - m[2][0]) / root,
					(m[1][0] - m[0][1]) / root,
					root / static_cast<T>(4)
				};
			} else if (m[0][0] > m[1][1] && m[0][0] > m[2][2]) {
				T root = static_cast<T>(2) * std::sqrt(static_cast<T>(1) + m[0][0] - m[1][1] - m[2][2]);
				return { 
					root / static_cast<T>(4),
					(m[0][1] + m[1][0]) / root,
					(m[0][2] + m[2][0]) / root,
					(m[2][1] - m[1][2]) / root
				};
			} else if (m[1][1] > m[2][2]) {
				T root = static_cast<T>(2) * std::sqrt(static_cast<T>(1) + m[1][1] - m[0][0] - m[2][2]);
				return { 
					(m[0][1] + m[1][0]) / root,
					root / static_cast<T>(4),
					(m[1][2] + m[2][1]) / root,
					(m[0][2] - m[2][0]) / root
				};
			} else {
				T root = static_cast<T>(2) * std::sqrt(static_cast<T>(1) + m[2][2] - m[0][0] - m[1][1]);
				return {
					(m[0][2] + m[2][0]) / root,
					(m[1][2] + m[2][1]) / root,
					root / static_cast<T>(4),
					(m[1][0] - m[0][1]) / root
				};
			}
		}
		
		// ================
		// STATIC ALGEBRA
		// ================
		
		static inline quaternion_t<T> slerp (quaternion_t<T> const & A, quaternion_t<T> const & B, T value) {
			
			#if BRASSICA_INPUT_SANITIZING == 1
			if (value < 0) value = 0;
			if (value > 1) value = 1;
			#endif
			
			if (value == 0) return A;
			if (value == 1) return B;
			
			T dot = A[0] * B[0] + A[1] * B[1] + A[2] * B[2] + A[3] * B[3];
			if (dot > 1) dot = 1;
			if (dot < -1) dot = -1;
			if (dot == 1) return A;
			T angle = std::acos(dot);
			T sqi = std::sqrt(static_cast<T>(1) - dot * dot);
			T vA = std::sin((static_cast<T>(1) - value) * angle) / sqi;
			T vB = std::sin(value * angle) / sqi;
			
			quaternion_t<T> ret {
				A[0] * vA + B[0] * vB,
				A[1] * vA + B[1] * vB,
				A[2] * vA + B[2] * vB,
				A[3] * vA + B[3] * vB,
			};
			
			ret.normalize();
			return ret;
		}
		
		static quaternion_t<T> multiply(quaternion_t<T> const & A, quaternion_t<T> const & B) {
			quaternion_t<T> ret {
				(A.data[3] * B.data[0] + A.data[0] * B.data[3] + A.data[1] * B.data[2] - A.data[2] * B.data[1]),
				(A.data[3] * B.data[1] - A.data[0] * B.data[2] + A.data[1] * B.data[3] + A.data[2] * B.data[0]),
				(A.data[3] * B.data[2] + A.data[0] * B.data[1] - A.data[1] * B.data[0] + A.data[2] * B.data[3]),
				(A.data[3] * B.data[3] - A.data[0] * B.data[0] - A.data[1] * B.data[1] - A.data[2] * B.data[2])
			};
			ret.normalize();
			return ret;
		}
		
		// ================
		// OPERATORS
		// ================
		
		constexpr quaternion_t<T> & operator = (quaternion_t<T> const & other) = default;
		constexpr quaternion_t<T> & operator = (quaternion_t<T> && other) = default;
		
		inline quaternion_t<T> operator * (quaternion_t<T> const & other) const {
			return quaternion_t<T>::multiply(*this, other);
		}
		
		inline quaternion_t<T> & operator *= (quaternion_t<T> const & other) {
			*this = quaternion_t<T>::multiply(other, *this); // order is reversed specifically for *= because it makes more contextual sense
			return *this;
		}
		
		constexpr T & operator [] (size_t i) { return data[i]; }
		constexpr T const & operator [] (size_t i) const { return data[i]; }
		
		template <typename U> quaternion_t<T> & operator = (quaternion_t<U> const & other) {
			data[0] = other.data[0];
			data[1] = other.data[1];
			data[2] = other.data[2];
			data[3] = other.data[3];
			normalize();
			return *this;
		}
		
#if BRASSICA_PRINT_FUNCTIONS == 1
		std::string to_string() const {
			return "Q[" + std::to_string(data[0]) + ", " + std::to_string(data[1]) + ", " + std::to_string(data[2]) + ", " + std::to_string(data[3]) + "]";
		}
#endif
	};
	
//================================================================================================
//------------------------------------------------------------------------------------------------
//================================================================================================
	
	template <typename T> struct mat3_t {
		T data [3][3] {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
		
		mat3_t() = default;
		mat3_t(T v1, T v2, T v3, T v4, T v5, T v6, T v7, T v8, T v9) : data {{v1, v2, v3}, {v4, v5, v6}, {v7, v8, v9}} {}
		
		explicit mat3_t(quaternion_t<T> const & v) {
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
		
		mat3_t(mat3_t const &) = default;
		mat3_t(mat3_t &&) = default;
		template <typename U> mat3_t(mat3_t<U> const & other) {
			data[0][0] = other.data[0][0];
			data[0][1] = other.data[0][1];
			data[0][2] = other.data[0][2];
			data[1][0] = other.data[1][0];
			data[1][1] = other.data[1][1];
			data[1][2] = other.data[1][2];
			data[2][0] = other.data[2][0];
			data[2][1] = other.data[2][1];
			data[2][2] = other.data[2][2];
		}
		
		static constexpr mat3_t<T> scale(T x, T y) {
			mat3_t<T> w {};
			w[0][0] = x;
			w[1][1] = y;
			return w;
		}
		
		static constexpr mat3_t<T> scale(vec2_t<T> const & v) {
			mat3_t<T> w {};
			w[0][0] = v.data[0];
			w[1][1] = v.data[1];
			return w;
		}
		
		static constexpr mat3_t<T> translate(T x, T y) {
			mat3_t<T> w {};
			w[2][0] = x;
			w[2][1] = y;
			return w;
		}
		
		static constexpr mat3_t<T> translate(vec2_t<T> const & v) {
			mat3_t<T> w {};
			w[2][0] = v.data[0];
			w[2][1] = v.data[1];
			return w;
		}
		
		static constexpr mat3_t<T> rotate(T v) {
			mat3_t<T> m;
			m[0][0] = cos(v);
			m[0][1] = -sin(v);
			m[1][0] = sin(v);
			m[1][1] = cos(v);
			return m;
		}
		
		static mat3_t<T> euler (vec3_t<T> const & v) {
			mat3_t<T> w {};
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
			mat3_t<T> w {};
			w[0][0] = static_cast<T>(2) / (r - l);
			w[1][1] = static_cast<T>(2) / (t - b);
			w[2][0] = - (r + l) / (r - l);
			w[2][1] = - (t + b) / (t - b);
			return w;
		}
		
		static mat3_t<T> multiply(mat3_t<T> const & A, mat3_t<T> const & B) {
			mat3_t<T> mat;
			mat.data[0][0] = A.data[0][0] * B.data[0][0] + A.data[0][1] * B.data[1][0] + A.data[0][2] * B.data[2][0];
			mat.data[0][1] = A.data[0][0] * B.data[0][1] + A.data[0][1] * B.data[1][1] + A.data[0][2] * B.data[2][1];
			mat.data[0][2] = A.data[0][0] * B.data[0][2] + A.data[0][1] * B.data[1][2] + A.data[0][2] * B.data[2][2];
			mat.data[1][0] = A.data[1][0] * B.data[0][0] + A.data[1][1] * B.data[1][0] + A.data[1][2] * B.data[2][0];
			mat.data[1][1] = A.data[1][0] * B.data[0][1] + A.data[1][1] * B.data[1][1] + A.data[1][2] * B.data[2][1];
			mat.data[1][2] = A.data[1][0] * B.data[0][2] + A.data[1][1] * B.data[1][2] + A.data[1][2] * B.data[2][2];
			mat.data[2][0] = A.data[2][0] * B.data[0][0] + A.data[2][1] * B.data[1][0] + A.data[2][2] * B.data[2][0];
			mat.data[2][1] = A.data[2][0] * B.data[0][1] + A.data[2][1] * B.data[1][1] + A.data[2][2] * B.data[2][1];
			mat.data[2][2] = A.data[2][0] * B.data[0][2] + A.data[2][1] * B.data[1][2] + A.data[2][2] * B.data[2][2];
			return mat;
		}
		
		inline mat3_t<T> & operator = (mat3_t<T> const & other) = default;
		inline mat3_t<T> & operator = (mat3_t<T> && other) = default;
		
		inline mat3_t<T> operator * (mat3_t<T> const & other) const {
			return mat3_t<T>::multiply(*this, other);
		}
		
		inline mat3_t<T> & operator *= (mat3_t<T> const & other) {
			*this = mat3_t<T>::multiply(*this, other);
			return *this;
		}
		
		template <typename U> mat3_t<T> & operator = (mat3_t<U> const & other) {
			data[0][0] = other.data[0][0];
			data[0][1] = other.data[0][1];
			data[0][2] = other.data[0][2];
			data[1][0] = other.data[1][0];
			data[1][1] = other.data[1][1];
			data[1][2] = other.data[1][2];
			data[2][0] = other.data[2][0];
			data[2][1] = other.data[2][1];
			data[2][2] = other.data[2][2];
			return *this;
		}
		
		inline T * operator [] (size_t i) { return data[i]; }
		inline T const * operator [] (size_t i) const { return data[i]; }
		
		inline operator T const * () const { return &data[0][0]; }
		
#if BRASSICA_PRINT_FUNCTIONS == 1
		std::string to_string() const {
			std::string out = "M3[";
			for (int x = 0; x < 3; x++) {
				if (x > 0) out += ", ";
				out += "[";
				for (int y = 0; y < 3; y++) {
					if (y > 0) out += ", ";
					out += std::to_string(data[x][y]);
				}
				out += "]";
			}
			out += "]";
			return out;
		}
#endif
	};
	
	
//================================================================================================
//------------------------------------------------------------------------------------------------
//================================================================================================
	
	template <typename T> struct mat4_t {
		T data [4][4] {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
		
		constexpr mat4_t() = default;
		constexpr mat4_t(T v1, T v2, T v3, T v4, T v5, T v6, T v7, T v8, T v9, T v10, T v11, T v12, T v13, T v14, T v15, T v16) : data {{v1, v2, v3, v4}, {v5, v6, v7, v8}, {v9, v10, v11, v12}, {v13, v14, v15, v16}} {}
		
		constexpr explicit mat4_t(quaternion_t<T> const & v) {
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
		
		constexpr mat4_t(mat4_t const &) = default;
		constexpr mat4_t(mat4_t &&) = default;
		template <typename U> constexpr mat4_t(mat4_t<U> const & other) {
			data[0][0] = other.data[0][0];
			data[0][1] = other.data[0][1];
			data[0][2] = other.data[0][2];
			data[0][3] = other.data[0][3];
			data[1][0] = other.data[1][0];
			data[1][1] = other.data[1][1];
			data[1][2] = other.data[1][2];
			data[1][3] = other.data[1][3];
			data[2][0] = other.data[2][0];
			data[2][1] = other.data[2][1];
			data[2][2] = other.data[2][2];
			data[2][3] = other.data[2][3];
			data[3][0] = other.data[3][0];
			data[3][1] = other.data[3][1];
			data[3][2] = other.data[3][2];
			data[3][3] = other.data[3][3];
		}
		
		static mat4_t<T> euler (vec3_t<T> const & v) {
			mat4_t<T> w {};
			T sa = sin(v.data[0]);
			T sb = sin(v.data[1]);
			T sh = sin(v.data[2]);
			T ca = cos(v.data[0]);
			T cb = cos(v.data[1]);
			T ch = cos(v.data[2]);
			w[0][0] = ch * ca;
			w[0][1] = -ch * sa * cb + sh * sb;
			w[0][2] = ch * sa * sb + sh * cb;
			w[1][0] = sa;
			w[1][1] = ca * cb;
			w[1][2] = -ca * sb;
			w[2][0] = -sh * ca;
			w[2][1] = sh * sa * cb + ch * sb;
			w[2][2] = -sh * sa * sb + ch * cb;
			return w;
		}
		
		static constexpr mat4_t<T> ortho(T t, T b, T l, T r, T n, T f) {
			mat4_t<T> w {};
			w[0][0] = static_cast<T>(2) / (r - l);
			w[1][1] = static_cast<T>(2) / (t - b);
			w[2][2] = static_cast<T>(-2) / (f - n);
			w[3][0] = - (r + l) / (r - l);
			w[3][1] = - (t + b) / (t - b);
			w[3][2] = - (f + n) / (f - n);
			return w;
		}
		
		static mat4_t<T> perspective(T vfov, T width, T height, T near, T far) {
			mat4_t<T> w {};
			T t1 = 1 / std::tan(0.5 * vfov);
			w[0][0] = t1 * (height / width);
			w[1][1] = t1;
			w[2][2] = (far + near) / (far - near);
			w[2][3] = static_cast<T>(1);
			w[3][2] = - (static_cast<T>(2) * far * near) / (far - near);
			w[3][3] = 0;
			return w;
		}
		
		static constexpr mat4_t<T> scale(T x, T y, T z) {
			mat4_t<T> w {};
			w[0][0] = x;
			w[1][1] = y;
			w[2][2] = z;
			return w;
		}
		
		static constexpr mat4_t<T> scale(vec3_t<T> const & v) {
			mat4_t<T> w {};
			w[0][0] = v.data[0];
			w[1][1] = v.data[1];
			w[2][2] = v.data[2];
			return w;
		}
		
		static constexpr mat4_t<T> translate(T x, T y, T z) {
			mat4_t<T> w {};
			w[3][0] = x;
			w[3][1] = y;
			w[3][2] = z;
			return w;
		}
		
		static constexpr mat4_t<T> translate(vec3_t<T> const & v) {
			mat4_t<T> w {};
			w[3][0] = v.data[0];
			w[3][1] = v.data[1];
			w[3][2] = v.data[2];
			return w;
		}
		
		constexpr T determinant() const {
			return
				data[0][3] * data[1][2] * data[2][1] * data[3][0] - data[0][2] * data[1][3] * data[2][1] * data[3][0] - data[0][3] * data[1][1] * data[2][2] * data[3][0] + data[0][1] * data[1][3] * data[2][2] * data[3][0] +
				data[0][2] * data[1][1] * data[2][3] * data[3][0] - data[0][1] * data[1][2] * data[2][3] * data[3][0] - data[0][3] * data[1][2] * data[2][0] * data[3][1] + data[0][2] * data[1][3] * data[2][0] * data[3][1] +
				data[0][3] * data[1][0] * data[2][2] * data[3][1] - data[0][0] * data[1][3] * data[2][2] * data[3][1] - data[0][2] * data[1][0] * data[2][3] * data[3][1] + data[0][0] * data[1][2] * data[2][3] * data[3][1] +
				data[0][3] * data[1][1] * data[2][0] * data[3][2] - data[0][1] * data[1][3] * data[2][0] * data[3][2] - data[0][3] * data[1][0] * data[2][1] * data[3][2] + data[0][0] * data[1][3] * data[2][1] * data[3][2] +
				data[0][1] * data[1][0] * data[2][3] * data[3][2] - data[0][0] * data[1][1] * data[2][3] * data[3][2] - data[0][2] * data[1][1] * data[2][0] * data[3][3] + data[0][1] * data[1][2] * data[2][0] * data[3][3] +
				data[0][2] * data[1][0] * data[2][1] * data[3][3] - data[0][0] * data[1][2] * data[2][1] * data[3][3] - data[0][1] * data[1][0] * data[2][2] * data[3][3] + data[0][0] * data[1][1] * data[2][2] * data[3][3];
		}
		
		constexpr mat4_t<T> inverted() const {
			mat4_t<T> mat {*this};
			mat.data[0][0] = data[1][2] * data[2][3] * data[3][1] - data[1][3] * data[2][2] * data[3][1] + data[1][3] * data[2][1] * data[3][2] - data[1][1] * data[2][3] * data[3][2] - data[1][2] * data[2][1] * data[3][3] + data[1][1] * data[2][2] * data[3][3];
			mat.data[0][1] = data[0][3] * data[2][2] * data[3][1] - data[0][2] * data[2][3] * data[3][1] - data[0][3] * data[2][1] * data[3][2] + data[0][1] * data[2][3] * data[3][2] + data[0][2] * data[2][1] * data[3][3] - data[0][1] * data[2][2] * data[3][3];
			mat.data[0][2] = data[0][2] * data[1][3] * data[3][1] - data[0][3] * data[1][2] * data[3][1] + data[0][3] * data[1][1] * data[3][2] - data[0][1] * data[1][3] * data[3][2] - data[0][2] * data[1][1] * data[3][3] + data[0][1] * data[1][2] * data[3][3];
			mat.data[0][3] = data[0][3] * data[1][2] * data[2][1] - data[0][2] * data[1][3] * data[2][1] - data[0][3] * data[1][1] * data[2][2] + data[0][1] * data[1][3] * data[2][2] + data[0][2] * data[1][1] * data[2][3] - data[0][1] * data[1][2] * data[2][3];
			mat.data[1][0] = data[1][3] * data[2][2] * data[3][0] - data[1][2] * data[2][3] * data[3][0] - data[1][3] * data[2][0] * data[3][2] + data[1][0] * data[2][3] * data[3][2] + data[1][2] * data[2][0] * data[3][3] - data[1][0] * data[2][2] * data[3][3];
			mat.data[1][1] = data[0][2] * data[2][3] * data[3][0] - data[0][3] * data[2][2] * data[3][0] + data[0][3] * data[2][0] * data[3][2] - data[0][0] * data[2][3] * data[3][2] - data[0][2] * data[2][0] * data[3][3] + data[0][0] * data[2][2] * data[3][3];
			mat.data[1][2] = data[0][3] * data[1][2] * data[3][0] - data[0][2] * data[1][3] * data[3][0] - data[0][3] * data[1][0] * data[3][2] + data[0][0] * data[1][3] * data[3][2] + data[0][2] * data[1][0] * data[3][3] - data[0][0] * data[1][2] * data[3][3];
			mat.data[1][3] = data[0][2] * data[1][3] * data[2][0] - data[0][3] * data[1][2] * data[2][0] + data[0][3] * data[1][0] * data[2][2] - data[0][0] * data[1][3] * data[2][2] - data[0][2] * data[1][0] * data[2][3] + data[0][0] * data[1][2] * data[2][3];
			mat.data[2][0] = data[1][1] * data[2][3] * data[3][0] - data[1][3] * data[2][1] * data[3][0] + data[1][3] * data[2][0] * data[3][1] - data[1][0] * data[2][3] * data[3][1] - data[1][1] * data[2][0] * data[3][3] + data[1][0] * data[2][1] * data[3][3];
			mat.data[2][1] = data[0][3] * data[2][1] * data[3][0] - data[0][1] * data[2][3] * data[3][0] - data[0][3] * data[2][0] * data[3][1] + data[0][0] * data[2][3] * data[3][1] + data[0][1] * data[2][0] * data[3][3] - data[0][0] * data[2][1] * data[3][3];
			mat.data[2][2] = data[0][1] * data[1][3] * data[3][0] - data[0][3] * data[1][1] * data[3][0] + data[0][3] * data[1][0] * data[3][1] - data[0][0] * data[1][3] * data[3][1] - data[0][1] * data[1][0] * data[3][3] + data[0][0] * data[1][1] * data[3][3];
			mat.data[2][3] = data[0][3] * data[1][1] * data[2][0] - data[0][1] * data[1][3] * data[2][0] - data[0][3] * data[1][0] * data[2][1] + data[0][0] * data[1][3] * data[2][1] + data[0][1] * data[1][0] * data[2][3] - data[0][0] * data[1][1] * data[2][3];
			mat.data[3][0] = data[1][2] * data[2][1] * data[3][0] - data[1][1] * data[2][2] * data[3][0] - data[1][2] * data[2][0] * data[3][1] + data[1][0] * data[2][2] * data[3][1] + data[1][1] * data[2][0] * data[3][2] - data[1][0] * data[2][1] * data[3][2];
			mat.data[3][1] = data[0][1] * data[2][2] * data[3][0] - data[0][2] * data[2][1] * data[3][0] + data[0][2] * data[2][0] * data[3][1] - data[0][0] * data[2][2] * data[3][1] - data[0][1] * data[2][0] * data[3][2] + data[0][0] * data[2][1] * data[3][2];
			mat.data[3][2] = data[0][2] * data[1][1] * data[3][0] - data[0][1] * data[1][2] * data[3][0] - data[0][2] * data[1][0] * data[3][1] + data[0][0] * data[1][2] * data[3][1] + data[0][1] * data[1][0] * data[3][2] - data[0][0] * data[1][1] * data[3][2];
			mat.data[3][3] = data[0][1] * data[1][2] * data[2][0] - data[0][2] * data[1][1] * data[2][0] + data[0][2] * data[1][0] * data[2][1] - data[0][0] * data[1][2] * data[2][1] - data[0][1] * data[1][0] * data[2][2] + data[0][0] * data[1][1] * data[2][2];
			mat = mat * mat4_t<T>::scale( vec3_t {1 / mat.determinant()} );
			return mat;
		}
		
		static constexpr mat4_t<T> multiply(mat4_t<T> const & A, mat4_t<T> const & B) {
			mat4_t<T> mat;
			mat.data[0][0] = A.data[0][0] * B.data[0][0] + A.data[0][1] * B.data[1][0] + A.data[0][2] * B.data[2][0] + A.data[0][3] * B.data[3][0];
			mat.data[0][1] = A.data[0][0] * B.data[0][1] + A.data[0][1] * B.data[1][1] + A.data[0][2] * B.data[2][1] + A.data[0][3] * B.data[3][1];
			mat.data[0][2] = A.data[0][0] * B.data[0][2] + A.data[0][1] * B.data[1][2] + A.data[0][2] * B.data[2][2] + A.data[0][3] * B.data[3][2];
			mat.data[0][3] = A.data[0][0] * B.data[0][3] + A.data[0][1] * B.data[1][3] + A.data[0][2] * B.data[2][3] + A.data[0][3] * B.data[3][3];
			mat.data[1][0] = A.data[1][0] * B.data[0][0] + A.data[1][1] * B.data[1][0] + A.data[1][2] * B.data[2][0] + A.data[1][3] * B.data[3][0];
			mat.data[1][1] = A.data[1][0] * B.data[0][1] + A.data[1][1] * B.data[1][1] + A.data[1][2] * B.data[2][1] + A.data[1][3] * B.data[3][1];
			mat.data[1][2] = A.data[1][0] * B.data[0][2] + A.data[1][1] * B.data[1][2] + A.data[1][2] * B.data[2][2] + A.data[1][3] * B.data[3][2];
			mat.data[1][3] = A.data[1][0] * B.data[0][3] + A.data[1][1] * B.data[1][3] + A.data[1][2] * B.data[2][3] + A.data[1][3] * B.data[3][3];
			mat.data[2][0] = A.data[2][0] * B.data[0][0] + A.data[2][1] * B.data[1][0] + A.data[2][2] * B.data[2][0] + A.data[2][3] * B.data[3][0];
			mat.data[2][1] = A.data[2][0] * B.data[0][1] + A.data[2][1] * B.data[1][1] + A.data[2][2] * B.data[2][1] + A.data[2][3] * B.data[3][1];
			mat.data[2][2] = A.data[2][0] * B.data[0][2] + A.data[2][1] * B.data[1][2] + A.data[2][2] * B.data[2][2] + A.data[2][3] * B.data[3][2];
			mat.data[2][3] = A.data[2][0] * B.data[0][3] + A.data[2][1] * B.data[1][3] + A.data[2][2] * B.data[2][3] + A.data[2][3] * B.data[3][3];
			mat.data[3][0] = A.data[3][0] * B.data[0][0] + A.data[3][1] * B.data[1][0] + A.data[3][2] * B.data[2][0] + A.data[3][3] * B.data[3][0];
			mat.data[3][1] = A.data[3][0] * B.data[0][1] + A.data[3][1] * B.data[1][1] + A.data[3][2] * B.data[2][1] + A.data[3][3] * B.data[3][1];
			mat.data[3][2] = A.data[3][0] * B.data[0][2] + A.data[3][1] * B.data[1][2] + A.data[3][2] * B.data[2][2] + A.data[3][3] * B.data[3][2];
			mat.data[3][3] = A.data[3][0] * B.data[0][3] + A.data[3][1] * B.data[1][3] + A.data[3][2] * B.data[2][3] + A.data[3][3] * B.data[3][3];
			return mat;
		}
		
		constexpr mat4_t<T> & operator = (mat4_t<T> const & other) = default;
		constexpr mat4_t<T> & operator = (mat4_t<T> && other) = default;
		
		constexpr mat4_t<T> operator * (mat4_t<T> const & other) const {
			return mat4_t<T>::multiply(*this, other);
		}
		
		constexpr mat4_t<T> & operator *= (mat4_t<T> const & other) {
			*this = mat4_t<T>::multiply(*this, other);
			return *this;
		}
		
		template <typename U> constexpr mat4_t<T> & operator = (mat4_t<U> const & other) {
			data[0][0] = other.data[0][0];
			data[0][1] = other.data[0][1];
			data[0][2] = other.data[0][2];
			data[0][3] = other.data[0][3];
			data[1][0] = other.data[1][0];
			data[1][1] = other.data[1][1];
			data[1][2] = other.data[1][2];
			data[1][3] = other.data[1][3];
			data[2][0] = other.data[2][0];
			data[2][1] = other.data[2][1];
			data[2][2] = other.data[2][2];
			data[2][3] = other.data[2][3];
			data[3][0] = other.data[3][0];
			data[3][1] = other.data[3][1];
			data[3][2] = other.data[3][2];
			data[3][3] = other.data[3][3];
			return *this;
		}
		
		constexpr T * operator [] (size_t i) { return data[i]; }
		constexpr T const * operator [] (size_t i) const { return data[i]; }
		
		constexpr operator T const * () const { return &data[0][0]; }
		
#if BRASSICA_PRINT_FUNCTIONS == 1
		std::string to_string() const {
			std::string out = "M4[";
			for (int x = 0; x < 4; x++) {
				if (x > 0) out += ", ";
				out += "[";
				for (int y = 0; y < 4; y++) {
					if (y > 0) out += ", ";
					out += std::to_string(data[x][y]);
				}
				out += "]";
			}
			out += "]";
			return out;
		}
#endif
	};
	
//================================================================================================
//------------------------------------------------------------------------------------------------
//================================================================================================

	template <typename T> inline vec3_t<T> operator * (quaternion_t<T> const & q, vec3_t<T> const & v) {
		vec3_t<T> qv {q[0], q[1], q[2]};
		vec3_t<T> w1 = vec3_t<T>::cross(v, qv) * static_cast<T>(2);
		return v + w1 * q[3] + vec3_t<T>::cross(w1, qv);
	}
	template <typename T> inline vec3_t<T> operator * (vec3_t<T> const & v, quaternion_t<T> const & q) {
		return q * v;
	}
	template <typename T> inline vec3_t<T> & operator *= (vec3_t<T> & v, quaternion_t<T> const & q) {
		v = q * v;
	}
	
	template <typename T> inline vec4_t<T> operator * (mat4_t<T> const & m, vec4_t<T> const & v) {
		return vec4_t {
			v[0] * m[0][0] + v[1] * m[1][0] + v[2] * m [2][0] + v[3] * m[3][0],
			v[0] * m[0][1] + v[1] * m[1][1] + v[2] * m [2][1] + v[3] * m[3][1],
			v[0] * m[0][2] + v[1] * m[1][2] + v[2] * m [2][2] + v[3] * m[3][2],
			v[0] * m[0][3] + v[1] * m[1][3] + v[2] * m [2][3] + v[3] * m[3][3],
		};
	}
	
	template <typename T> inline vec4_t<T> operator * (vec4_t<T> const & v, mat4_t<T> const & m) {
		return vec4_t {
			v[0] * m[0][0] + v[1] * m[0][1] + v[2] * m [0][2] + v[3] * m[0][3],
			v[0] * m[1][0] + v[1] * m[1][1] + v[2] * m [1][2] + v[3] * m[1][3],
			v[0] * m[2][0] + v[1] * m[2][1] + v[2] * m [2][2] + v[3] * m[2][3],
			v[0] * m[3][0] + v[1] * m[3][1] + v[2] * m [3][2] + v[3] * m[3][3],
		};
	}
	
//================================================================================================
//------------------------------------------------------------------------------------------------
//================================================================================================'

	namespace la {
		
		template <typename T> struct plane_t;
		
		template <typename T> struct ray_t {
			
			vec3_t<T> origin;
			vec3_t<T> direction;
			
			ray_t(vec3_t<T> const & origin, vec3_t<T> const & direction) : origin(origin), direction(direction) {}
			ray_t(ray_t const &) = default;
			ray_t(ray_t &&) = default;
			static inline ray_t<T> from_points(vec3_t<T> start, vec3_t<T> end) {
				vec3_t<T> dir = end - start;
				dir.normalize();
				return {start, dir};
			}
			
			vec3_t<T> intersect(plane_t<T> const & p) {
				T d = vec3_t<T>::dot(p.normal * p.dist - origin, p.normal) / vec3_t<T>::dot(direction, p.normal);
				return origin + direction * d;
			}
			
			bool intersect(plane_t<T> const & p, vec3_t<T> & intersect_out) {
				T d1 = vec3_t<T>::dot(direction, p.normal);
				if (std::abs(d1) <= std::numeric_limits<T>::epsilon) return false;
				T d2 = vec3_t<T>::dot(p.normal * p.dist - origin, p.normal) / d1;
				intersect_out = origin + direction * d2;
				return true;
			}
			
			#if BRASSICA_PRINT_FUNCTIONS == 1
					std::string to_string() const {
						return "RAY[" + 
						origin.to_string() + ", " + 
						direction.to_string() + "]";
					}
			#endif
		};
		
		template <typename T> struct plane_t {
			
			vec3_t<T> normal;
			T dist;
			
			plane_t(vec3_t<T> const & normal, T const & dist) : normal(normal), dist(dist) {}
			plane_t(plane_t const &) = default;
			plane_t(plane_t &&) = default;
			plane_t(vec3_t<T> const & A, vec3_t<T> const & B, vec3_t<T> const & C) : normal { vec3_t<T>::cross(A - B, A - C).normalized() }, dist { vec3_t<T>::dot(A, normal) } {
				if (dist < 0) {
					normal = -normal;
					dist = -dist;
				}
			}
			
			inline plane_t & operator = (plane_t const &) = default;
			inline plane_t & operator = (plane_t &&) = default;
			
			#if BRASSICA_PRINT_FUNCTIONS == 1
					std::string to_string() const {
						return "PLANE[" + 
						normal.to_string() + ", " + 
						std::to_string(dist) + "]";
					}
			#endif
		};
		
	}

//================================================================================================
//------------------------------------------------------------------------------------------------
//================================================================================================

	template <typename T> struct pascal_triangle {
		std::vector<std::vector<T>> rows {{1}};
		
		pascal_triangle() = default;
		inline pascal_triangle(size_t init) { for (size_t i = 1; i < init + 1; i++) gen_row(); }
		
		std::vector<T> const & row(size_t row) {
			for (size_t i = rows.size(); i < row + 1; i++) gen_row();
			return rows[row];
		}
		
	protected:
		void gen_row() {
			auto & last_row = rows.back();
			std::vector<T> new_row {static_cast<T>(1)};
			new_row.resize(rows.size());
			for (size_t j = 1; j < rows.size(); j++) new_row[j] = last_row[j-1] + last_row[j];
			new_row.push_back(static_cast<T>(1));
			rows.push_back(new_row);
		}
	};
	
	template <typename T, typename U> std::vector<T> sum_normalize(std::vector<U> const & vec) {
		work_uint_t sum = std::accumulate<std::vector<U>::const_iterator, work_uint_t>(vec.begin(), vec.end(), 0);
		std::vector<T> ret {};
		for (T const & v : vec) { ret.push_back(v / sum); }
		return ret;
	}

//================================================================================================
//------------------------------------------------------------------------------------------------
//================================================================================================

}

#if BRASSICA_PRINT_FUNCTIONS == 1
namespace std {
	template <typename T> std::string to_string(asterid::brassica::vec2_t<T> const & v) {
		return v.to_string();
	}
	template <typename T> std::string to_string(asterid::brassica::vec3_t<T> const & v) {
		return v.to_string();
	}
	template <typename T> std::string to_string(asterid::brassica::vec4_t<T> const & v) {
		return v.to_string();
	}
	template <typename T> std::string to_string(asterid::brassica::rect_t<T> const & v) {
		return v.to_string();
	}
	template <typename T> std::string to_string(asterid::brassica::quaternion_t<T> const & v) {
		return v.to_string();
	}
	template <typename T> std::string to_string(asterid::brassica::mat3_t<T> const & v) {
		return v.to_string();
	}
	template <typename T> std::string to_string(asterid::brassica::mat4_t<T> const & v) {
		return v.to_string();
	}
	template <typename T> std::string to_string(asterid::brassica::la::ray_t<T> const & v) {
		return v.to_string();
	}
	template <typename T> std::string to_string(asterid::brassica::la::plane_t<T> const & v) {
		return v.to_string();
	}
}
#endif

#if BRASSICA_HASH_FUNCTIONS == 1
namespace std {
	template <typename T> struct hash<asterid::brassica::vec2_t<T>> {
		size_t operator () (asterid::brassica::vec2_t<T> const & v) const {
			return std::hash(v[0]) + std::hash(v[1]);
		}
	};
	template <typename T> struct hash<asterid::brassica::vec3_t<T>> {
		size_t operator () (asterid::brassica::vec3_t<T> const & v) const {
			return std::hash(v[0]) + std::hash(v[1]) + std::hash(v[2]);
		}
	};
	template <typename T> struct hash<asterid::brassica::vec4_t<T>> {
		size_t operator () (asterid::brassica::vec4_t<T> const & v) const {
			return std::hash(v[0]) + std::hash(v[1]) + std::hash(v[2]) + std::hash(v[3]);
		}
	};
	template <typename T> struct hash<asterid::brassica::rect_t<T>> {
		size_t operator () (asterid::brassica::rect_t<T> const & v) const {
			return std::hash(v.origin) + std::hash(v.extents);
		}
	};
	template <typename T> struct hash<asterid::brassica::quaternion_t<T>> {
		size_t operator () (asterid::brassica::quaternion_t<T> const & v) const {
			return std::hash(v[0]) + std::hash(v[1]) + std::hash(v[2]) + std::hash(v[3]);
		}
	};
	template <typename T> struct hash<asterid::brassica::mat3_t<T>> {
		size_t operator () (asterid::brassica::mat3_t<T> const & v) const {
			size_t h = 0;
			for (size_t i = 0; i < 3; i++) for (size_t j = 0; j < 3; j++)
				h += std::hash(v[i][j]);
			return h;
		}
	};
	template <typename T> struct hash<asterid::brassica::mat4_t<T>> {
		size_t operator () (asterid::brassica::mat4_t<T> const & v) const {
			size_t h = 0;
			for (size_t i = 0; i < 4; i++) for (size_t j = 0; j < 4; j++)
				h += std::hash(v[i][j]);
			return h;
		}
	};
}
#endif
