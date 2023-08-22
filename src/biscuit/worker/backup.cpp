#include "backup.hpp"

using namespace Biscuit::Worker;

Backup::Backup() : QRunnable() {
	this->setAutoDelete(false);
}


void Backup::run() {}