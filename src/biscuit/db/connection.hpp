#ifndef __BISCUIT_DB_CONNECTION_HPP__
#define __BISCUIT_DB_CONNECTION_HPP__

namespace Biscuit {
	namespace Db {
		class Driver;

		class Connection {
			public:
				virtual bool connected() = 0;
				inline Driver& driver() {
					return this->m_driver;
				}
				inline const Driver& driver() const {
					return this->m_driver;
				}

			protected:
				Connection(Driver& driver);

			private:
				Driver& m_driver;
		};
	}
}

#endif