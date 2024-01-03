#include "host-id.hpp"

using namespace Biscuit::Db;

HostId::HostId(SqlStatus status) : SqlResult(status) {}

HostId::HostId(SqlStatus status, const QVariant& value) : SqlResult(status, value) {}

HostId::HostId(SqlStatus status, QVariant&& value) : SqlResult(status, std::move(value)) {}

HostId::HostId(const HostId& host_id) : SqlResult(host_id) {}

HostId::HostId(HostId&& host_id) : SqlResult(host_id) {}


HostId& HostId::operator=(const HostId& host_id) {
	this->copy(host_id);
	return *this;
}

HostId& HostId::operator=(HostId&& host_id) {
	this->move(std::move(host_id));
	return *this;
}
