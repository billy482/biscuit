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
	if (backup_id.is_found()) {
		Backup backup(backup_id);
		backup.run();

		if (connection->finish_backup(backup_id))
			logger->info("Backup completed");
		else
			logger->error("Error while finishing backup");
	} else
		logger->error("Error while starting backup");

	delete connection;
	driver->close();

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

	Key& key = Key::get();
	Db::KeyId key_id = connection->synchronize_key(key);
	if (key_id.is_error()) {
		logger->critical("Backup: error while synchronizing key");
		return;
	}

	for (Source::Source * source = Source::Source::first_source(); source != nullptr; source = source->next_source()) {
		const Host& host = source->host();
		Db::HostId host_id = connection->synchronize_host(host);
		if (host_id.is_error()) {
			logger->error("Backup: failed to synchronize host: {}", host.hostname().toUtf8().data());
			continue;
		}

		for (FileInfo file_info = source->next(); not file_info.is_invalid(); file_info = source->next()) {
			logger->debug("Backup: checking file: {}", file_info.path().toUtf8().data());

			Db::FileId file_id;
			if (file_info.is_file()) {
				if (connection->is_newer_or_not_exists(file_info, host_id)) {
					file_id = connection->insert_file(file_info, host_id);
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
						Db::BlockId block_id = connection->get_block(digest, hash_algo, key_id);
						if (block_id.is_error()) {
							logger->error("Backup: error get block: #{} {}", sequence, digest.toBase64().data());

							l.lock();
							cache.remove(digest);
							w.notify_all();
							l.unlock();

							break;
						} else if (not block_id.is_found()) {
							QByteArray encrypted = key.encrypt(buffer);
							block_id = connection->insert_block(encrypted, digest, hash_algo, key_id);
							if (block_id.is_error()) {
								logger->error("Backup: error get block: #{} {}", sequence, digest.toBase64().data());

								l.lock();
								cache.remove(digest);
								w.notify_all();
								l.unlock();

								break;
							}
						}

						l.lock();
						cache.remove(digest);
						w.notify_all();
						l.unlock();

						if (not connection->link_file_to_block(file_id, block_id, sequence)) {
							logger->error("Backup: error while linking file to block: #{}", sequence);
							break;
						}

						buffer = file_stream->read(4096);
					}

					delete file_stream;
				}
			} else if (file_info.is_dir()) {
				file_id = connection->get_file(file_info, host_id);
				if (file_id.is_error()) {
					logger->error("Backup: error while getting directory: {}", file_info.path().toUtf8().data());
					continue;
				} else if (not file_id.is_found()) {
					file_id = connection->insert_file(file_info, host_id);
					if (file_id.is_error()) {
						logger->error("Backup: error while inserting directory: {}", file_info.path().toUtf8().data());
						continue;
					}
				}

			} else
				continue;


			QByteArray metadata = file_info.metadata().toJson(QJsonDocument::Compact);
			QByteArray digest = QCryptographicHash::hash(metadata, QCryptographicHash::Sha1);
			Db::MetadataId metadata_id = connection->get_metadata(digest, "sha1");
			if (metadata_id.is_error()) {
				logger->error("Backup: error while getting metadata: {}", file_info.path().toUtf8().data());
				continue;
			} else if (not metadata_id.is_found()) {
				metadata_id = connection->insert_metadata(metadata, digest, "sha1");
				if (metadata_id.is_error()) {
					logger->error("Backup: error while getting metadata: {}", file_info.path().toUtf8().data());
					continue;
				}
			}

			if (not connection->link_file_to_backup(file_id, m_backup_id, metadata_id))
				logger->error("Backup: error linking file to backup: {}", file_info.path().toUtf8().data());
		}
	}

	delete connection;
}
