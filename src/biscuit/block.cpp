#include <cstring>

#include "block.hpp"

using namespace Biscuit;

Block::Block(const uint8_t * buffer, uint16_t buffer_size) {
	this->m_buffer = new uint8_t[buffer_size];
	std::memcpy(this->m_buffer, buffer, buffer_size);
	this->m_buffer_size = buffer_size;
}

Block::Block(const Block& block) {
	this->m_buffer = new uint8_t[block.m_buffer_size];
	std::memcpy(this->m_buffer, block.m_buffer, block.m_buffer_size);
	this->m_buffer_size = block.m_buffer_size;
}

Block::Block(Block&& block) {
	this->m_buffer = block.m_buffer;
	this->m_buffer_size = block.m_buffer_size;
	block.m_buffer = nullptr;
}

Block::~Block() {
	if (this->m_buffer != nullptr)
		delete [] this->m_buffer;
}


Block& Block::operator=(const Block& block) {
	if (this->m_buffer_size > block.m_buffer_size) {
		delete [] this->m_buffer;
		this->m_buffer = new uint8_t[block.m_buffer_size];
	}
	std::memcpy(this->m_buffer, block.m_buffer, block.m_buffer_size);
	this->m_buffer_size = block.m_buffer_size;
	return *this;
}

Block& Block::operator=(Block&& block) {
	delete [] this->m_buffer;
	this->m_buffer = block.m_buffer;
	this->m_buffer_size = block.m_buffer_size;
	block.m_buffer = nullptr;
	return *this;
}