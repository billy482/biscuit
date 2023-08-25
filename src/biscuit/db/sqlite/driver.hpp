#ifndef __BISCUIT_DB_SQLITE_DRIVER_HPP__
#define __BISCUIT_DB_SQLITE_DRIVER_HPP__

#include <QtCore/QFileInfo>

#include "../driver.hpp"

namespace Biscuit {
	namespace Db {
		namespace Sqlite {
			class SQliteDriver : public Driver {
				public:
					virtual ~SQliteDriver() = default;

					static SQliteDriver * configure(const YAML::Node& node);
					virtual Connection * open();

				private:
					SQliteDriver(const QFileInfo& path);

					QFileInfo m_path;
			};
		}
	}
}

#endif