#include "key-id.hpp"

using namespace Biscuit::Db;

KeyId::KeyId(SqlStatus status) : SqlResult(status) {}

KeyId::KeyId(SqlStatus status, const QVariant& value) : SqlResult(status, value) {}

KeyId::KeyId(SqlStatus status, QVariant&& value) : SqlResult(status, std::move(value)) {}

KeyId::KeyId(const KeyId& key_id) : SqlResult(key_id) {}

KeyId::KeyId(KeyId&& key_id) : SqlResult(key_id) {}


KeyId& KeyId::operator=(const KeyId& key_id) {
	this->copy(key_id);
	return *this;
}

KeyId& KeyId::operator=(KeyId&& key_id) {
	this->move(std::move(key_id));
	return *this;
}

