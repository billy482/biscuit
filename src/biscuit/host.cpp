#include <QtCore/QSysInfo>

#include "host.hpp"

using namespace Biscuit;

Host::Host() {
	this->m_hostname = QSysInfo::machineHostName();
}

Host::Host(const QString& hostname) : m_hostname(hostname) {}


Host& Host::localhost() {
	static Host localhost;
	return localhost;
}
