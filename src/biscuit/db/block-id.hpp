#ifndef __BISCUIT_DB_BLOCKID_HPP__
#define __BISCUIT_DB_BLOCKID_HPP__

#include "sql-result.hpp"

namespace Biscuit {
	namespace Db {
		class BlockId : public SqlResult {
			public:
				BlockId(SqlStatus status = SqlStatus::error);
				BlockId(SqlStatus status, const QVariant& value);
				BlockId(SqlStatus status, QVariant&& value);
				BlockId(const BlockId& block_id);
				BlockId(BlockId&& block_id);

				BlockId& operator =(const BlockId& block_id);
				BlockId& operator =(BlockId&& block_id);
		};
	}
}

#endif
