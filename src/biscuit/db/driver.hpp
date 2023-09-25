#ifndef __BISCUIT_DB_DRIVER_HPP__
#define __BISCUIT_DB_DRIVER_HPP__

#include <QtCore/QString>

namespace YAML {
	class Node;
}

namespace Biscuit {
	namespace Db {
		class Connection;

		class Driver {
			public:
				virtual ~Driver();

				static bool configure(const YAML::Node& node);
				static Driver * get();
				inline const QString& name() const {
					return this->m_name;
				}
				virtual Connection * open() = 0;

			protected:
				Driver(const QString& name);

			private:
				static Driver * ms_instance;
				QString m_name;
		};
	}
}

#endif