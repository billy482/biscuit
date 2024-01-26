#ifndef __BISCUIT_DB_SQLITE_DRIVER_HPP__
#define __BISCUIT_DB_SQLITE_DRIVER_HPP__

#include <QtCore/QFileInfo>

#include "../driver.hpp"

typedef struct sqlite3 sqlite3;

namespace Biscuit {
	namespace Db {
		namespace Sqlite {
			class SqliteDriver : public Driver {
				public:
					virtual ~SqliteDriver();

					static SqliteDriver * configure(const YAML::Node& node);
					virtual Connection * open();

				private:
					SqliteDriver(const QFileInfo& path);

					bool create_db(sqlite3 * db);

	 				sqlite3 * m_connection = nullptr;
					QFileInfo m_path;
			};
		}
	}
}

#endif
