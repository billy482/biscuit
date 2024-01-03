#include "backup-id.hpp"

using namespace Biscuit::Db;

BackupId::BackupId(SqlStatus status) : SqlResult(status) {}

BackupId::BackupId(SqlStatus status, const QVariant& value) : SqlResult(status, value) {}

BackupId::BackupId(SqlStatus status, QVariant&& value) : SqlResult(status, std::move(value)) {}

BackupId::BackupId(const BackupId& backup_id) : SqlResult(backup_id) {}

BackupId::BackupId(BackupId&& backup_id) : SqlResult(backup_id) {}


BackupId& BackupId::operator=(const BackupId& backup_id) {
	this->copy(backup_id);
	return *this;
}

BackupId& BackupId::operator=(BackupId&& backup_id) {
	this->move(std::move(backup_id));
	return *this;
}

