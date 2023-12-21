#ifndef __BISCUIT_HOST_HPP__
#define __BISCUIT_HOST_HPP__

#include <QtCore/QString>

namespace Biscuit {
	class Host {
		public:
			Host();
			Host(const QString& hostname);
			~Host() = default;

			inline const QString& hostname() const {
				return this->m_hostname;
			}
			static Host& localhost();

		private:
			QString m_hostname;
	};
}

#endif
