#ifndef __BISCUIT_DB_SQLRESULT_HPP__
#define __BISCUIT_DB_SQLRESULT_HPP__

#include <QtCore/QVariant>

namespace Biscuit {
	namespace Db {
		enum class SqlStatus {
			error,
			not_found,
			has_result
		};

		class SqlResult {
			public:
				inline bool is_error() const {
					return this->m_status == SqlStatus::error;
				}
				inline SqlStatus status() const {
					return this->m_status;
				}
				inline const QVariant& value() const {
					return this->m_value;
				}

			protected:
				SqlResult(SqlStatus status = SqlStatus::error);
				SqlResult(SqlStatus status, const QVariant& value);
				SqlResult(SqlStatus status, QVariant&& value);
				SqlResult(const SqlResult& result);
				SqlResult(SqlResult&& result);

				void copy(const SqlResult& src);
				void move(SqlResult&& src);

				SqlStatus m_status;
				QVariant m_value;
		};
	}
}

#endif
