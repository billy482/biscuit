#ifndef __BISCUIT_DB_METADATAID_HPP__
#define __BISCUIT_DB_METADATAID_HPP__

#include "sql-result.hpp"

namespace Biscuit {
	namespace Db {
		class MetadataId : public SqlResult {
			public:
				MetadataId(SqlStatus status = SqlStatus::error);
				MetadataId(SqlStatus status, const QVariant& value);
				MetadataId(SqlStatus status, QVariant&& value);
				MetadataId(const MetadataId& metadata_id);
				MetadataId(MetadataId&& metadata_id);

				MetadataId& operator =(const MetadataId& metadata_id);
				MetadataId& operator =(MetadataId&& metadata_id);
		};
	}
}

#endif
