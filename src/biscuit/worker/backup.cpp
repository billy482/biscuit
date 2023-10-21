#include <QtCore/QCryptographicHash>
#include <QtCore/QIODevice>
#include <spdlog/spdlog.h>

#include "backup.hpp"
#include "../db/connection.hpp"
#include "../db/driver.hpp"
#include "../source/file-info.hpp"
#include "../source/source.hpp"

using namespace Biscuit::Worker;
using Biscuit::Db::Connection;
using Biscuit::Source::FileInfo;
using Biscuit::Source::Source;
using DbConnection = Biscuit::Db::Connection;
using DbDriver = Biscuit::Db::Driver;

Backup::Backup() : QRunnable() {
	this->setAutoDelete(false);
}


int Backup::do_backup() {


	return 0;
}

void Backup::run() {
	auto logger = spdlog::get("core");

	DbDriver * driver = DbDriver::get();
	if (driver == nullptr) {
		logger->critical("Backup: error while getting database driver");
		return;
	}

	DbConnection * connection = driver->open();
	if (connection == nullptr) {
		logger->critical("Backup: error while opening database (driver: {})", driver->name().toUtf8().data());
		return;
	}

	for (Source::Source * source = Source::Source::first_source(); source != nullptr; source = source->next_source()) {
		for (FileInfo file_info = source->next(); not file_info.is_invalid(); file_info = source->next()) {
			connection->is_newer_or_exists(file_info);

			QIODevice * file_stream = source->open(file_info);

			QByteArray buffer = file_stream->read(4096);
			for (quint32 sequence = 0; buffer.size() > 0; sequence++) {
				QByteArray digest = QCryptographicHash::hash(buffer, QCryptographicHash::Sha1);
				// find block into db and insert it if not found then retrieve its id

				buffer = file_stream->read(4096);
			}
		}
	}
}