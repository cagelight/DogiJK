#include "tr_common.hh"

#include <jxl/decode.h>
#include <jxl/encode.h>

#include <span>

#define JXLD(exec, test) m_stat = exec; if (m_stat != test) { Com_Printf("JXLReader: unexpected status code (%i) returned from " #exec " while trying to load \"%s\"", m_stat, filename); goto ERROR; };
#define JXLDE(exec) m_stat = exec; if (m_stat != JXL_DEC_SUCCESS) { Com_Printf("JXLReader: unexpected status code (%i) returned from " #exec " while trying to load \"%s\"", m_stat, filename); goto ERROR; };
#define JXLE(exec, test) m_stat = exec; if (m_stat != test) { Com_Printf("JXLWriter: unexpected status code (%i) returned from " #exec " while trying to write \"%s\"", m_stat, filename); return {}; };
#define JXLEE(exec) m_stat = exec; if (m_stat != JXL_ENC_SUCCESS) { Com_Printf("JXLWriter: unexpected status code (%i) returned from " #exec " while trying to write \"%s\"", m_stat, filename); return {}; };

struct JXLReader {
	
	void read(char const * filename, std::span<uint8_t const> input, unsigned char * * data, int * width, int * height) {
		auto sig = JxlSignatureCheck(input.data(), input.size());
		if (sig != JXL_SIG_CODESTREAM && sig != JXL_SIG_CONTAINER) return;
		
		size_t buffer_size = 0;
		
		m_dec = JxlDecoderCreate(nullptr);
		assert(m_dec);
		
		JXLDE(JxlDecoderSetInput(m_dec, input.data(), input.size()));
		JXLDE(JxlDecoderSubscribeEvents(m_dec, JXL_DEC_BASIC_INFO | JXL_DEC_FULL_IMAGE));
		JXLD(JxlDecoderProcessInput(m_dec), JXL_DEC_BASIC_INFO);
		JXLDE(JxlDecoderGetBasicInfo(m_dec, &m_info));
		
		*width = m_info.xsize;
		*height = m_info.ysize;
		
		JXLD(JxlDecoderProcessInput(m_dec), JXL_DEC_NEED_IMAGE_OUT_BUFFER);
		
		static constexpr JxlPixelFormat PFMT {
			.num_channels = 4,
			.data_type = JXL_TYPE_UINT8,
			.endianness = JXL_NATIVE_ENDIAN,
			.align = 0
		};
		
		JXLDE(JxlDecoderImageOutBufferSize(m_dec, &PFMT, &buffer_size));
		
		*data = reinterpret_cast<uint8_t *>(Z_Malloc(buffer_size, TAG_TEMP_WORKSPACE, 0));
		
		JXLDE(JxlDecoderSetImageOutBuffer(m_dec, &PFMT, *data, buffer_size));
		JXLD(JxlDecoderProcessInput(m_dec), JXL_DEC_FULL_IMAGE);
		
		return;
		
		ERROR:
		if (data) Z_Free(data);
		return;
	}
	
	~JXLReader() {
		if (m_dec) JxlDecoderDestroy(m_dec);
	}
	
private:
	JxlDecoder * m_dec = nullptr;
	JxlDecoderStatus m_stat;
	JxlBasicInfo m_info;
};

void LoadJXL(char const * filename, unsigned char * * data, int * width, int * height) {
	
	FS_Reader fs { filename };
	if (!fs.valid() || fs.size() <= 0) return;
	
	JXLReader r;
	r.read(filename, {fs.data(), static_cast<size_t>(fs.size())}, data, width, height);
}

struct JXLWriter {
	
	std::vector<uint8_t> encode(char const * filename, std::span<uint8_t> buf, size_t width, size_t height, float quality, int effort) {
		std::vector<uint8_t> data;
		
		m_enc = JxlEncoderCreate(nullptr);
		assert(m_enc);
		
		m_opt = JxlEncoderOptionsCreate(m_enc, nullptr);
		JXLEE(JxlEncoderOptionsSetDistance(m_opt, quality));
		JXLEE(JxlEncoderOptionsSetLossless(m_opt, quality == 0 ? JXL_TRUE : JXL_FALSE));
		JXLEE(JxlEncoderOptionsSetEffort(m_opt, effort));

		static constexpr JxlPixelFormat PFMT {
			.num_channels = 3,
			.data_type = JXL_TYPE_UINT8,
			.endianness = JXL_NATIVE_ENDIAN,
			.align = 0
		};

		m_info.xsize = width;
		m_info.ysize = height;
		m_info.bits_per_sample = 8;
		m_info.num_color_channels = 3;
		m_info.intensity_target = 255;
		m_info.orientation = JXL_ORIENT_FLIP_VERTICAL;
		if (quality == 0)
			m_info.uses_original_profile = JXL_TRUE;
		JXLEE(JxlEncoderSetBasicInfo(m_enc, &m_info));

		JxlColorEncoding color_profile;
		JxlColorEncodingSetToSRGB(&color_profile, JXL_FALSE);
		JXLEE(JxlEncoderSetColorEncoding(m_enc, &color_profile));
		JXLEE(JxlEncoderAddImageFrame(m_opt, &PFMT, buf.data(), buf.size()));
		JxlEncoderCloseInput(m_enc);
		
		data.resize(524288); // 512 KiB
		uint8_t * dcur = data.data();
		size_t avail = data.size();
		
		while (true) {
			m_stat = JxlEncoderProcessOutput(m_enc, &dcur, &avail);
			switch (m_stat) {
				case JXL_ENC_SUCCESS:
					data.resize(data.size() - avail);
					return data;
				case JXL_ENC_NEED_MORE_OUTPUT: {
					size_t offs = dcur - data.data();
					avail += data.size();
					data.resize(data.size() * 2);
					dcur = data.data() + offs;
					continue;
				}
				default:
					return {};
			}
		}
		
	}
	
	~JXLWriter() {
		if (m_enc) JxlEncoderDestroy(m_enc);
	}
	
private:
	JxlEncoder * m_enc = nullptr;
	JxlEncoderOptions * m_opt = nullptr;
	JxlEncoderStatus m_stat;
	JxlBasicInfo m_info {};
};

bool RE_SaveJXL( char const * filename, byte * buf, size_t width, size_t height, float quality, int effort ) {
	
	if (quality < 0) quality = 0;
	if (quality > 15) quality = 15;
	
	if (effort < 3) effort = 3;
	if (effort > 9) effort = 9;
	
	struct fwrite_t {
		fwrite_t(char const * filename) { fp = ri.FS_FOpenFileWrite( filename, qtrue ); }
		~fwrite_t() { if (is_valid()) ri.FS_FCloseFile( fp );  }
		bool is_valid() { return fp; }
		void write(uint8_t const * data, size_t len) { ri.FS_Write(data, len, fp); }
	private:
		fileHandle_t fp;
	} file { filename };
	
	if (!file.is_valid()) return false;
	
	std::vector<uint8_t> flip;
	flip.resize(width * height * 3);
	for (size_t i = 0; i < height; i++)
		memcpy( flip.data() + i * width * 3, buf + (height - i - 1) * width * 3, width * 3 );
	
	JXLWriter w;
	auto data = w.encode(filename, flip, width, height, quality, effort);
	
	if (!data.size()) return false;
	file.write(data.data(), data.size());
	return true;
} 
