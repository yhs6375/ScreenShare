#include "HubContext.h"
#include "SocketImpl.h"


namespace HS {
	namespace Socket {
		HubContext::HubContext(unsigned short threadCount) {
			ThreadContexts.push_back(std::make_shared<ThreadContext>());
		}
		std::shared_ptr<ThreadContext> HubContext::getnextContext() { return ThreadContexts[(m_nextService++ % ThreadContexts.size())]; }
		void HubContext::beginInflate()
		{
			InflateBufferSize = 0;
			free(InflateBuffer);
			InflateBuffer = nullptr;
		}
		std::tuple<unsigned char *, size_t> HubContext::returnemptyinflate()
		{
			unsigned char *p = nullptr;
			size_t o = 0;
			return std::make_tuple(p, o);
		}
		std::tuple<unsigned char *, unsigned int> HubContext::Inflate(unsigned char *data, size_t data_len)
		{
			InflationStream.next_in = (Bytef *)data;
			InflationStream.avail_in = data_len;

			int err;
			do {
				InflationStream.next_out = (Bytef *)TempInflateBuffer.get();
				InflationStream.avail_out = LARGE_BUFFER_SIZE;
				err = ::inflate(&InflationStream, Z_FINISH);
				if (!InflationStream.avail_in) {
					break;
				}
				auto growsize = LARGE_BUFFER_SIZE - InflationStream.avail_out;
				InflateBufferSize += growsize;
				auto beforesize = InflateBufferSize;
				InflateBuffer = static_cast<unsigned char *>(realloc(InflateBuffer, InflateBufferSize));
				if (!InflateBuffer) {
					return returnemptyinflate();
				}
				memcpy(InflateBuffer + beforesize, TempInflateBuffer.get(), growsize);
			} while (err == Z_BUF_ERROR && InflateBufferSize <= MaxPayload);

			inflateReset(&InflationStream);

			if ((err != Z_BUF_ERROR && err != Z_OK) || InflateBufferSize > MaxPayload) {
				return returnemptyinflate();
			}
			if (InflateBufferSize > 0) {
				auto growsize = LARGE_BUFFER_SIZE - InflationStream.avail_out;
				InflateBufferSize += growsize;
				auto beforesize = InflateBufferSize;
				InflateBuffer = static_cast<unsigned char *>(realloc(InflateBuffer, InflateBufferSize));
				if (!InflateBuffer) {
					return returnemptyinflate();
				}
				memcpy(InflateBuffer + beforesize, TempInflateBuffer.get(), growsize);
				return std::make_tuple(InflateBuffer, InflateBufferSize);
			}
			return std::make_tuple(TempInflateBuffer.get(), LARGE_BUFFER_SIZE - (size_t)InflationStream.avail_out);
		}
		void HubContext::endInflate() { beginInflate(); }
	}
}