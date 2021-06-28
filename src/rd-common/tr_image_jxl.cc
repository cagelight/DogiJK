#include "tr_common.hh"

#include <jxl/decode.h>

#include <span>

#define JXLD(exec, test) m_stat = exec; if (m_stat != test) { Com_Printf("JXLReader: unexpected status code (%i) returned from " #exec " while trying to load \"%s\"", m_stat, filename); goto ERROR; };
#define JXLDE(exec) m_stat = exec; if (m_stat != JXL_DEC_SUCCESS) { Com_Printf("JXLReader: unexpected status code (%i) returned from " #exec " while trying to load \"%s\"", m_stat, filename); goto ERROR; };

struct JXLReader {
	
	void read(char const * filename, std::span<uint8_t const> input, unsigned char * * data, int * width, int * height) {
		auto sig = JxlSignatureCheck(input.data(), input.size());
		if (sig != JXL_SIG_CODESTREAM && sig != JXL_SIG_CONTAINER) return;
		
		size_t buffer_size = 0;
		
		m_dec = JxlDecoderCreate(nullptr);
		if (!m_dec) return;
		
		JXLDE(JxlDecoderSetInput(m_dec, input.data(), input.size()));
		JXLDE(JxlDecoderSubscribeEvents(m_dec, JXL_DEC_BASIC_INFO | JXL_DEC_FRAME | JXL_DEC_FULL_IMAGE));
		JXLD(JxlDecoderProcessInput(m_dec), JXL_DEC_BASIC_INFO);
		JXLDE(JxlDecoderGetBasicInfo(m_dec, &m_info));
		
		*width = m_info.xsize;
		*height = m_info.ysize;
		
		JXLD(JxlDecoderProcessInput(m_dec), JXL_DEC_FRAME);
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

bool RE_SaveJXL( char const * filename, byte * buf, size_t width, size_t height, int quality ) {
	// TODO
	return false;
} 
