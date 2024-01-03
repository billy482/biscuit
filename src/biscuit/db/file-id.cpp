#include "file-id.hpp"

using namespace Biscuit::Db;

FileId::FileId(SqlStatus status) : SqlResult(status) {}

FileId::FileId(SqlStatus status, const QVariant& value) : SqlResult(status, value) {}

FileId::FileId(SqlStatus status, QVariant&& value) : SqlResult(status, std::move(value)) {}

FileId::FileId(const FileId& file_id) : SqlResult(file_id) {}

FileId::FileId(FileId&& file_id) : SqlResult(file_id) {}


FileId& FileId::operator=(const FileId& file_id) {
	this->copy(file_id);
	return *this;
}

FileId& FileId::operator=(FileId&& file_id) {
	this->move(std::move(file_id));
	return *this;
}

