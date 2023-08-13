#ifndef __BISCUIT_CHECKSUM_HPP__
#define __BISCUIT_CHECKSUM_HPP__

#include <cstdint>

typedef struct evp_md_ctx_st EVP_MD_CTX;

namespace Biscuit {
	class Block;
	class Digest;

	enum ChecksumName {
		MD5 = 1,
		SHA1,
		SHA256,
		SHA512,

		Invalid = 0
	};

	class Checksum {
		public:
			Checksum(ChecksumName name = ChecksumName::SHA1);
			Checksum(const Block& block, ChecksumName name = ChecksumName::SHA1);
			~Checksum();

			Digest digest();
			inline bool is_valid() const {
				return this->m_valid;
			}
			inline ChecksumName name() const {
				return this->m_name;
			}
			void update(const Block& block);
			void update(const void * buffer, uint16_t buffer_size);

		private:
			EVP_MD_CTX * m_ctx = nullptr;
			ChecksumName m_name;
			bool m_valid = false;
	};
}

#endif