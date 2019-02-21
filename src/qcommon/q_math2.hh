#pragma once

#include "q_shared.hh"
#include "q_math.hh"

namespace qm {
	
	struct vec3_t {
		
		using data_t = std::array<float, 3>;
		data_t data {0, 0, 0};
		
		// ================================
		
		inline constexpr vec3_t () = default;
		inline constexpr vec3_t ( ::vec3_t const & v ) : data { v[0], v[1], v[2] } {}
		inline constexpr vec3_t ( float x, float y, float z ) : data { x, y, z } {}
		
		inline constexpr vec3_t ( vec3_t const & ) = default;
		inline constexpr vec3_t ( vec3_t && ) = default;
		
		// ================================
		
		inline data_t::const_iterator begin () const { return data.begin(); }
		inline data_t::iterator begin () { return data.begin(); }
		inline data_t::const_iterator end () const { return data.end(); }
		inline data_t::iterator end () { return data.end(); }
		
		// ================================
		
		inline constexpr float * ptr () { return data.data(); }
		inline constexpr float const * ptr () const { return data.data(); }
		
		// ================================
		
		static inline constexpr float dot ( vec3_t const & A, vec3_t const & B ) {
			return A.data[0] * B.data[0] + A.data[1] * B.data[1] + A.data[2] * B.data[2];
		}
		
		inline constexpr float dot ( vec3_t const & other ) {
			return dot ( *this, other );
		}
		
		// ================================
		
		static inline constexpr vec3_t cross ( vec3_t const & A, vec3_t const & B ) {
			return {
				A.data[1] * B.data[2] - A.data[2] * B.data[1],
				A.data[2] * B.data[0] - A.data[0] * B.data[2],
				A.data[0] * B.data[1] - A.data[1] * B.data[0],
			};
		}
		
		inline constexpr vec3_t cross ( vec3_t const & other ) {
			return cross( *this, other );
		}
		
		// ================================
		
		inline constexpr float magnitude () {
			return std::sqrt( data[0] * data[0] + data[1] * data[1] + data[2] * data[2] );
		}
		
		inline constexpr float magnitude_squared () {
			return data[0] * data[0] + data[1] * data[1] + data[2] * data[2];
		}
		
		inline constexpr float normalize () {
		
			float length = magnitude();
			if ( !length ) return 0;
			
			float ilen = 1.0f / length;
			data[0] *= ilen;
			data[1] *= ilen;
			data[2] *= ilen;
			
			return length;
			
		}
		
		// ================================
		
		inline constexpr vec3_t operator + ( float v ) const {
			return {
				data[0] + v,
				data[1] + v,
				data[2] + v,
			};
		}
		
		inline constexpr vec3_t operator + ( vec3_t const & v ) const {
			return {
				data[0] + v[0],
				data[1] + v[1],
				data[2] + v[2],
			};
		}
		
		inline constexpr vec3_t & operator += ( float v ) {
			data[0] += v;
			data[1] += v;
			data[2] += v;
			return *this;
		}
		
		inline constexpr vec3_t & operator += ( vec3_t const & v ) {
			data[0] += v[0];
			data[1] += v[1];
			data[2] += v[2];
			return *this;
		}
		
		// ================================
		
		inline constexpr vec3_t operator - ( float v ) const {
			return {
				data[0] - v,
				data[1] - v,
				data[2] - v,
			};
		}
		
		inline constexpr vec3_t operator - ( vec3_t const & v ) const {
			return {
				data[0] - v[0],
				data[1] - v[1],
				data[2] - v[2],
			};
		}
		
		inline constexpr vec3_t & operator -= ( float v ) {
			data[0] -= v;
			data[1] -= v;
			data[2] -= v;
			return *this;
		}
		
		inline constexpr vec3_t & operator -= ( vec3_t const & v ) {
			data[0] -= v[0];
			data[1] -= v[1];
			data[2] -= v[2];
			return *this;
		}
		
		// ================================
		
		inline constexpr vec3_t operator * ( float v ) const {
			return {
				data[0] * v,
				data[1] * v,
				data[2] * v,
			};
		}
		
		inline constexpr vec3_t & operator *= ( float v ) {
			data[0] *= v;
			data[1] *= v;
			data[2] *= v;
			return *this;
		}
		
		// ================================
		
		inline constexpr vec3_t operator / ( float v ) const {
			return {
				data[0] / v,
				data[1] / v,
				data[2] / v,
			};
		}
		
		inline constexpr vec3_t & operator /= ( float v ) {
			data[0] /= v;
			data[1] /= v;
			data[2] /= v;
			return *this;
		}
		
		// ================================
		
		inline constexpr bool operator == ( vec3_t const & other ) const {
			return data[0] == other[0] && data[1] == other[1] && data[2] == other[2];
		}
		
		inline constexpr vec3_t & operator = ( vec3_t const & other ) {
			data = other.data;
			return *this;
		}
		
		// ================================
		
		inline constexpr float & operator [] ( size_t i ) { return data[i]; }
		inline constexpr float const & operator [] ( size_t i ) const { return data[i]; }
	};
	
}
