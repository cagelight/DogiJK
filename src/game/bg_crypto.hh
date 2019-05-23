#pragma once

#include "bg_local.hh"

namespace dogicrypt {
	
	namespace meta {
	
		template <size_t BITS>
		struct sha3_t;
		
		template <size_t BITS>
		struct block_t;
		
		template <size_t BITS>
		struct blockchain_t;
	
	}
	
	using block_t = meta::block_t<256>;
	using blockchain_t = meta::blockchain_t<256>;
	
	void crypto_test();
	
	std::string hex(uint8_t const * data, size_t len);
	std::string hexlow(uint8_t const * data, size_t len);
	
	template <typename T, typename V = typename T::value_type, typename std::enable_if_t<std::is_pod<V>::value && !std::is_pointer<V>::value>* = nullptr>
	static std::string hex(T const & cnt) { 
		return hex(cnt.data(), cnt.size());
	}
	
	template <typename T, typename V = typename T::value_type, typename std::enable_if_t<std::is_pod<V>::value && !std::is_pointer<V>::value>* = nullptr>
	static std::string hexlow(T const & cnt) { 
		return hexlow(cnt.data(), cnt.size());
	}
};

template <size_t BITS>
struct dogicrypt::meta::block_t {
	
	using hash_t = sha3_t<BITS>;
	using digest_t = typename hash_t::array_t;
	
private:
	
	digest_t digest;
	digest_t digest_parent;
	
	std::vector<uint8_t> data;
};

template <size_t BITS>
struct dogicrypt::meta::blockchain_t {
	
	using block_t = meta::block_t<BITS>;
	
	blockchain_t() = default;
	
private:
	
	std::vector<block_t> blocks;
};

template <size_t BITS>
struct dogicrypt::meta::sha3_t {
	
	static constexpr uint16_t mdlen = BITS / 8;
	static_assert(mdlen * 8 == BITS);
	
	static constexpr size_t bytes_len = 200;
	static_assert(bytes_len == (bytes_len / 8) * 8);
	
	using array_t = std::array<uint8_t, mdlen>;
	
    union {
        std::array<uint8_t, bytes_len> bytes;
        std::array<uint64_t, bytes_len / 8> words;
    };
	
    uint16_t pt, rsiz;
	
	constexpr sha3_t() : bytes(), pt(0), rsiz(bytes_len - 2 * mdlen) {}
	
	constexpr void update(uint8_t const * data, size_t len) {
		int j = pt;
		for (size_t i = 0; i < len; i++) {
			bytes[j++] ^= ((const uint8_t *) data)[i];
			if (j >= rsiz) {
				sha3_keccakf();
				j = 0;
			}
		}
		pt = j;
	}
	
	constexpr void finalize(uint8_t * md) {
		bytes[pt] ^= 0x06;
		bytes[rsiz - 1] ^= 0x80;
		sha3_keccakf();
		
		for (uint16_t i = 0; i < mdlen; i++) {
			md[i] = bytes[i];
		}
	}
	
	constexpr array_t finalize() {
		array_t md {};
		finalize(md.data());
		return md;
	}
	
	template <typename T, typename V = typename T::value_type, typename std::enable_if_t<std::is_pod<V>::value && !std::is_pointer<V>::value>* = nullptr>
	static constexpr array_t compute(T const & cnt) { 
		return compute(reinterpret_cast<uint8_t const *>(cnt.data()), cnt.size() * sizeof(V));
	}
	static constexpr array_t compute(std::string const & str) { 
		return compute(reinterpret_cast<uint8_t const *>(str.data()), str.size());
	}
	static constexpr array_t compute(uint8_t const * data, size_t len) {
		sha3_t sha3;
		sha3.update(data, len);
		return sha3.finalize();
	}
	
private:
	
	constexpr uint64_t rot64(uint64_t const & x, uint64_t const & y) {
		return (((x) << (y)) | ((x) >> (64 - (y))));
	}
	
	constexpr void sha3_keccakf() {
		
		constexpr size_t keccakf_rounds = 24;
		
		// constants
		constexpr std::array<uint64_t, 24> keccakf_rndc {
			0x0000000000000001, 0x0000000000008082, 0x800000000000808a,
			0x8000000080008000, 0x000000000000808b, 0x0000000080000001,
			0x8000000080008081, 0x8000000000008009, 0x000000000000008a,
			0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
			0x000000008000808b, 0x800000000000008b, 0x8000000000008089,
			0x8000000000008003, 0x8000000000008002, 0x8000000000000080,
			0x000000000000800a, 0x800000008000000a, 0x8000000080008081,
			0x8000000000008080, 0x0000000080000001, 0x8000000080008008
		};
		
		constexpr std::array<int, 24> keccakf_rotc {
			1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14,
			27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44
		};
		constexpr std::array<int, 24> keccakf_piln {
			10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4,
			15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1
		};

		// variables
		int i = 0, j = 0;
		uint64_t t = 0, bc[5] {};

		// actual iteration
		for (size_t r = 0; r < keccakf_rounds; r++) {

			// Theta
			for (i = 0; i < 5; i++)
				bc[i] = words[i] ^ words[i + 5] ^ words[i + 10] ^ words[i + 15] ^ words[i + 20];

			for (i = 0; i < 5; i++) {
				t = bc[(i + 4) % 5] ^ rot64(bc[(i + 1) % 5], 1);
				for (j = 0; j < 25; j += 5)
					words[j + i] ^= t;
			}

			// Rho Pi
			t = words[1];
			for (i = 0; i < 24; i++) {
				j = keccakf_piln[i];
				bc[0] = words[j];
				words[j] = rot64(t, keccakf_rotc[i]);
				t = bc[0];
			}

			//  Chi
			for (j = 0; j < 25; j += 5) {
				for (i = 0; i < 5; i++)
					bc[i] = words[j + i];
				for (i = 0; i < 5; i++)
					words[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
			}

			//  Iota
			words[0] ^= keccakf_rndc[r];
		}
	}
};
