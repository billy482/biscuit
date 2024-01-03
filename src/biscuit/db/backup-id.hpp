#ifndef __BISCUIT_DB_BACKUPID_HPP__
#define __BISCUIT_DB_BACKUPID_HPP__

#include "sql-result.hpp"

namespace Biscuit {
	namespace Db {
		class BackupId : public SqlResult {
			public:
				BackupId(SqlStatus status = SqlStatus::error);
				BackupId(SqlStatus status, const QVariant& value);
				BackupId(SqlStatus status, QVariant&& value);
				BackupId(const BackupId& backup_id);
				BackupId(BackupId&& backup_id);

				BackupId& operator =(const BackupId& backup_id);
				BackupId& operator =(BackupId&& backup_id);
		};
	}
}

#endif
