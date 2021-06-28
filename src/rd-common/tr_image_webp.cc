#include "tr_common.hh"

// SUPERSEDED BY JXL

/*
#include "webp/decode.h"
#include "webp/encode.h"

void LoadWEBP(char const * filename, unsigned char ** data, int * width, int * height) {
	
	FS_Reader fs {filename};
	if (!fs.valid()) return;
	
	if (!WebPGetInfo(fs.data(), fs.size(), width, height)) return;
	
	
	//WebPBitstreamFeatures features {};
	//VP8StatusCode code = WebPGetFeatures(fs.data(), fs.size(), &features);
	//if (code != VP8_STATUS_OK) return;
	
	
	size_t buffer_size = (*width) * (*height) * 4;
	*data = (byte *)Z_Malloc(buffer_size, TAG_TEMP_WORKSPACE, qfalse);
	
	if (!WebPDecodeRGBAInto(fs.data(), fs.size(), *data, buffer_size, 4 * (*width))) {
		Z_Free(*data);
		*data = nullptr;
	}
}

bool RE_SaveWEBP( char const * filename, byte * buf, size_t width, size_t height, int quality ) {
	
	struct fwrite_t {
		fwrite_t(char const * filename) { fp = ri.FS_FOpenFileWrite( filename, qtrue ); }
		~fwrite_t() { if (is_valid()) ri.FS_FCloseFile( fp );  }
		bool is_valid() { return fp; }
		void write(uint8_t const * data, size_t len) { ri.FS_Write(data, len, fp); }
	private:
		fileHandle_t fp;
	} file { filename };
	
	uint8_t * out;
	size_t len = 0;
	
	if (quality < 0) {
		len = WebPEncodeLosslessRGB(buf, width, height, width * 3, &out);
	} else {
		float qf = qm::clamp<float>(quality, 0, 100);
		len = WebPEncodeRGB(buf, width, height, width * 3, qf, &out);
	}
	
	if (!len) {
		if (out) WebPFree(out);
		return false;
	}
	
	file.write(out, len);
	WebPFree(out);
	return true;
} 
*/
