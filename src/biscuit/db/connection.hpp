#ifndef __BISCUIT_DB_CONNECTION_HPP__
#define __BISCUIT_DB_CONNECTION_HPP__

namespace Biscuit {
	namespace Source {
		class FileInfo;
	}

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
				virtual bool is_newer_or_not_exists(const Source::FileInfo& file_info) = 0;

			protected:
				Connection(Driver& driver);

			private:
				Driver& m_driver;
		};
	}
}

#endif