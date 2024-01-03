#ifndef __BISCUIT_DB_FILEID_HPP__
#define __BISCUIT_DB_FILEID_HPP__

#include "sql-result.hpp"

namespace Biscuit {
	namespace Db {
		class FileId : public SqlResult {
			public:
				FileId(SqlStatus status = SqlStatus::error);
				FileId(SqlStatus status, const QVariant& value);
				FileId(SqlStatus status, QVariant&& value);
				FileId(const FileId& file_id);
				FileId(FileId&& file_id);

				FileId& operator =(const FileId& file_id);
				FileId& operator =(FileId&& file_id);
		};
	}
}

#endif
