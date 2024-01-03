#ifndef __BISCUIT_DB_HOSTID_HPP__
#define __BISCUIT_DB_HOSTID_HPP__

#include "sql-result.hpp"

namespace Biscuit {
	namespace Db {
		class HostId : public SqlResult {
			public:
				HostId(SqlStatus status = SqlStatus::error);
				HostId(SqlStatus status, const QVariant& value);
				HostId(SqlStatus status, QVariant&& value);
				HostId(const HostId& host_id);
				HostId(HostId&& host_id);

				HostId& operator =(const HostId& host_id);
				HostId& operator =(HostId&& host_id);
		};
	}
}

#endif
