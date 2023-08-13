#ifndef __BISCUIT_DIGEST_HPP__
#define __BISCUIT_DIGEST_HPP__

#include <string>

#include "block.hpp"

namespace Biscuit {
	class Digest : public Block {
		public:
			Digest() = default;
			Digest(uint8_t * buffer, uint16_t buffer_size, bool acquire_buffer = false);
			Digest(const uint8_t * buffer, uint16_t buffer_size);
			Digest(const Digest& digest);
			Digest(Digest&& digest);
			virtual ~Digest() = default;

			std::string base64() const;
			std::string hex() const;

			Digest& operator=(const Digest& digest);
			Digest& operator=(Digest&& digest);
	};
}

#endif