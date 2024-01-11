#include "block-id.hpp"

using namespace Biscuit::Db;

BlockId::BlockId(SqlStatus status) : SqlResult(status) {}

BlockId::BlockId(SqlStatus status, const QVariant& value) : SqlResult(status, value) {}

BlockId::BlockId(SqlStatus status, QVariant&& value) : SqlResult(status, std::move(value)) {}

BlockId::BlockId(const BlockId& block_id) : SqlResult(block_id) {}

BlockId::BlockId(BlockId&& block_id) : SqlResult(block_id) {}


BlockId& BlockId::operator=(const BlockId& block_id) {
	this->copy(block_id);
	return *this;
}

BlockId& BlockId::operator=(BlockId&& block_id) {
	this->move(std::move(block_id));
	return *this;
}

