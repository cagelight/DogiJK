#include "bg_crypto.hh"

static constexpr uint8_t hmask = 0b11110000;
static constexpr uint8_t lmask = 0b00001111;

std::string dogicrypt::hex(uint8_t const * data, size_t len) {
	std::string ret {};
	ret.resize(len * 2);
	for (size_t i = 0; i < len; i++) {
		char vh = (data[i] & hmask) >> 4;
		char vl =  data[i] & lmask;
		ret[i*2]   = vh > 9 ? 55 + vh : 48 + vh;
		ret[i*2+1] = vl > 9 ? 55 + vl : 48 + vl;
	}
	return ret;
}

std::string dogicrypt::hexlow(uint8_t const * data, size_t len) {
	std::string ret {};
	ret.resize(len * 2);
	for (size_t i = 0; i < len; i++) {
		char vh = (data[i] & hmask) >> 4;
		char vl =  data[i] & lmask;
		ret[i*2]   = vh > 9 ? 87 + vh : 48 + vh;
		ret[i*2+1] = vl > 9 ? 87 + vl : 48 + vl;
	}
	return ret;
}
