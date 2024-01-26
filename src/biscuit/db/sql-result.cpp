#include "sql-result.hpp"

using namespace Biscuit::Db;

SqlResult::SqlResult(SqlStatus status) : m_status(status), m_value() {}

SqlResult::SqlResult(SqlStatus status, const QVariant& value) : m_status(status), m_value(value) {}

SqlResult::SqlResult(SqlStatus status, QVariant&& value) : m_status(status), m_value(std::move(value)) {}

SqlResult::SqlResult(const SqlResult& result) : m_status(result.m_status), m_value(result.m_value) {}

SqlResult::SqlResult(SqlResult&& result) : m_status(result.m_status), m_value(std::move(result.m_value)) {}


void SqlResult::copy(const SqlResult& src) {
	this->m_status = src.m_status;
	this->m_value = src.m_value;
}

void SqlResult::move(SqlResult&& src) {
	this->m_status = src.m_status;
	this->m_value = std::move(src.m_value);
}
