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
#include "../host.hpp"
#include "../key.hpp"
#include "../source/file-info.hpp"
#include "../source/source.hpp"

using namespace Biscuit::Worker;
using Biscuit::Db::Connection;
using Biscuit::Source::FileInfo;
using Biscuit::Source::Source;
using DbConnection = Biscuit::Db::Connection;
using DbDriver = Biscuit::Db::Driver;

Backup::Backup(const Db::BackupId& backup_id) : QRunnable(), m_backup_id(backup_id) {
	this->setAutoDelete(false);
}


int Backup::do_backup(const YAML::Node&) {
	// TODO: use a thread pool
	auto logger = spdlog::get("core");

	if (not Key::get().open_for_encrypt()) {
		logger->critical("Failed to open keys");
		return 1;
	}

	DbDriver * driver = DbDriver::get();
	if (driver == nullptr) {
		logger->critical("Backup: error while getting database driver");
		return 2;
	}

	DbConnection * connection = driver->open();
	if (connection == nullptr) {
		logger->critical("Backup: error while opening database (driver: {})", driver->name().toUtf8().data());
		return 3;
	}

	Key& key = Key::get();
	Db::KeyId key_id = connection->synchronize_key(key);
	if (key_id.is_error()) {
		logger->critical("Backup: error while synchronizing key");
		return 4;
	}
	
	Db::BackupId backup_id = connection->start_backup();
	if (backup_id.status() == Db::SqlStatus::has_result) {
		Backup backup(backup_id);
		backup.run();
	}

	delete connection;

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

	// Key& key = Key::get();
	for (Source::Source * source = Source::Source::first_source(); source != nullptr; source = source->next_source()) {
		const Host& host = source->host();
		Db::HostId host_id = connection->synchronize_host(host);
		if (host_id.status() != Db::SqlStatus::has_result) {
			logger->error("Backup: failed to synchronize host: {}", host.hostname().toUtf8().data());
			continue;
		}

		for (FileInfo file_info = source->next(); not file_info.is_invalid(); file_info = source->next()) {
			logger->debug("Backup: checking file: {}", file_info.path().toUtf8().data());

			if (file_info.is_file()) {
				if (connection->is_newer_or_not_exists(file_info, host_id)) {
					Db::FileId file_id = connection->insert_file(file_info, host_id);
					if (file_id.is_error()) {
						logger->error("Backup: error while inserting file: {}", file_info.path().toUtf8().data());
						continue;
					}

					QIODevice * file_stream = source->open(file_info);

					QByteArray buffer = file_stream->read(4096);
					for (quint32 sequence = 0; buffer.size() > 0; sequence++) {
						QByteArray digest = QCryptographicHash::hash(buffer, QCryptographicHash::Sha1);

						static QHash<QByteArray, QByteArray> cache;
						static QMutex l;
						static QWaitCondition w;

						l.lock();
						while (cache.contains(digest))
							w.wait(&l);
						cache[digest] = buffer;
						l.unlock();

						// find block into db and insert it if not found then retrieve its id
						static const QString hash_algo = "sha1";
						/*if (not connection->has_block(digest, hash_algo, key)) {

						}*/

						l.lock();
						cache.remove(digest);
						w.notify_all();
						l.unlock();

						buffer = file_stream->read(4096);

					}

					delete file_stream;
				}
			}
		}
	}

	delete connection;
}
