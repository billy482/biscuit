#ifndef __BISCUIT_BLOCK_HPP__
#define __BISCUIT_BLOCK_HPP__

#include <cstdint>

namespace Biscuit {
	class Block {
		public:
			Block() = default;
			Block(const uint8_t * buffer, uint16_t buffer_size);
			Block(const Block& block);
			Block(Block&& block);
			virtual ~Block();

			inline const uint8_t * buffer() const {
				return this->m_buffer;
			}
			inline uint16_t buffer_size() const {
				return this->m_buffer_size;
			}
			inline bool is_valid() const {
				return this->m_buffer_size > 0;
			}

			Block& operator=(const Block& block);
			Block& operator=(Block&& block);

		protected:
			uint8_t * m_buffer = nullptr;
			uint16_t m_buffer_size = 0;
	};
}

#endif