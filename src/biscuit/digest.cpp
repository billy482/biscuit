#include <iomanip>
#include <sstream>
#include <utility>

#include "digest.hpp"

using namespace Biscuit;

Digest::Digest(const uint8_t * buffer, uint16_t buffer_size) : Block(buffer, buffer_size) {}

Digest::Digest(const Digest& digest) : Block(digest) {}

Digest::Digest(Digest&& digest) : Block(std::move(digest)) {}


std::string Digest::base64() const {
	static char base64[] = {
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
		'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
		'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
		'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
		'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', '+', '/'
	};

	std::ostringstream result;

	char buffer[5];
	buffer[4] = '\0';

	for (uint16_t pos = 0; pos < this->m_buffer_size; pos += 3) {
		buffer[0] = base64[this->m_buffer[pos] >> 2];

		uint8_t character = (this->m_buffer[pos] & 0x03) << 4;
		if (pos + 1 < this->m_buffer_size)
			character |= (this->m_buffer[pos + 1] >> 4);
		buffer[1] = base64[character];

		if (pos + 1 < this->m_buffer_size) {
			character = (this->m_buffer[pos + 1] & 0x0F) << 2;
			if (pos + 2 < this->m_buffer_size)
				character |= this->m_buffer[pos + 2] >> 6;
			buffer[2] = base64[character];
		}
		else
			buffer[2] = '=';

		if (pos + 2 < this->m_buffer_size)
			buffer[3] = base64[this->m_buffer[pos + 2] & 0x3F];
		else
			buffer[3] = '=';

		result << buffer;
	}

	return result.str();
}

std::string Digest::hex() const {
	std::ostringstream buffer;

	buffer << std::setfill('0');
	for (uint16_t i = 0; i < this->m_buffer_size; i++)
		buffer << std::hex << std::setw(2) << static_cast<uint16_t>(this->m_buffer[i]);

	return buffer.str();
}


Digest& Digest::operator=(const Digest& digest) {
	Block::operator=(digest);
	return *this;
}

Digest& Digest::operator=(Digest&& digest) {
	Block::operator=(std::move(digest));
	return *this;
}