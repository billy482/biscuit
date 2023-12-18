#include <QtCore/QCryptographicHash>
#include <QtCore/QHash>
#include <QtCore/QIODevice>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include "backup.hpp"
#include "../db/connection.hpp"
#include "../db/driver.hpp"
#include "../key.hpp"
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


int Backup::do_backup(const YAML::Node&) {
	// TODO: use a thread pool

	if (not Key::get().open_for_encrypt()) {
		auto logger = spdlog::get("core");
		logger->critical("Failed to open keys");
		return 1;
	}
	
	Backup backup;
	backup.run();

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

	static QMutex l;
	l.lock();
	Key& key = Key::get();
	connection->synchronize_key(key);

	l.unlock();

	for (Source::Source * source = Source::Source::first_source(); source != nullptr; source = source->next_source()) {
		for (FileInfo file_info = source->next(); not file_info.is_invalid(); file_info = source->next()) {
			// TODO: update status

			logger->debug("Backup: checking file: {}", file_info.path().toLocal8Bit().data());

			if (connection->is_newer_or_not_exists(file_info) and file_info.is_file() and file_info.file_size() > 0) {
				QIODevice * file_stream = source->open(file_info);

				QByteArray buffer = file_stream->read(4096);
				for (quint32 sequence = 0; buffer.size() > 0; sequence++) {
					QByteArray digest = QCryptographicHash::hash(buffer, QCryptographicHash::Sha1);

					static QHash<QByteArray, QByteArray> cache;
					static QWaitCondition w;

					l.lock();
					while (cache.contains(digest))
						w.wait(&l);
					cache[digest] = buffer;
					l.unlock();

					// find block into db and insert it if not found then retrieve its id

					buffer = file_stream->read(4096);
				}
			}
		}
	}
}
