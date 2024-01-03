#ifndef __BISCUIT_DB_KEYID_HPP__
#define __BISCUIT_DB_KEYID_HPP__

#include "sql-result.hpp"

namespace Biscuit {
	namespace Db {
		class KeyId : public SqlResult {
			public:
				KeyId(SqlStatus status = SqlStatus::error);
				KeyId(SqlStatus status, const QVariant& value);
				KeyId(SqlStatus status, QVariant&& value);
				KeyId(const KeyId& key_id);
				KeyId(KeyId&& key_id);

				KeyId& operator =(const KeyId& key_id);
				KeyId& operator =(KeyId&& key_id);
		};
	}
}

#endif
