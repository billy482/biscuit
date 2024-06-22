/***************************************************************************\
*                         __    _                 _ __                      *
*                        / /_  (_)___________  __(_) /_                     *
*                       / __ \/ / ___/ ___/ / / / / __/                     *
*                      / /_/ / (__  ) /__/ /_/ / / /_                       *
*                     /_.___/_/____/\___/\__,_/_/\__/                       *
*                                                                           *
*  -----------------------------------------------------------------------  *
*  This file is a part of biscuit                                           *
*                                                                           *
*  biscuit is free software; you can redistribute it and/or                 *
*  modify it under the terms of the GNU General Public License              *
*  as published by the Free Software Foundation; either version 3           *
*  of the License, or (at your option) any later version.                   *
*                                                                           *
*  This program is distributed in the hope that it will be useful,          *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*  GNU General Public License for more details.                             *
*                                                                           *
*  You should have received a copy of the GNU General Public License        *
*  along with this program; if not, write to the Free Software              *
*  Foundation, Inc., 51 Franklin Street, Fifth Floor,                       *
*  Boston, MA  02110-1301, USA.                                             *
*                                                                           *
*  You should have received a copy of the GNU General Public License        *
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
*                                                                           *
*  -----------------------------------------------------------------------  *
*  Copyright (C) 2024, Guillaume Clercin <guillaume.clercin@billy482.net>   *
\***************************************************************************/

#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QIODevice>
#include <QtCore/QList>
#include <QtCore/QTextStream>
#include <QtCore/QThreadPool>
#include <QtCore/QTime>
#include <QtCore/QWaitCondition>
#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include "backup.hpp"
#include "../db/connection.hpp"
#include "../db/driver.hpp"
#include "../host.hpp"
#include "../key.hpp"
#include "../options.hpp"
#include "../source/file-info.hpp"
#include "../source/source.hpp"

using namespace Biscuit::Worker;
using Biscuit::Db::Connection;
using Biscuit::Source::FileInfo;
using Biscuit::Source::Source;
using DbConnection = Biscuit::Db::Connection;
using DbDriver = Biscuit::Db::Driver;

uint16_t Backup::ms_ids = 0;
uint16_t Backup::ms_block_size = 1024;
const struct Biscuit::Checksum * Backup::ms_checksum = nullptr;


Backup::Backup(const Db::BackupId& backup_id) : QRunnable(), m_id(Backup::ms_ids++), m_backup_id(backup_id) {
	this->setAutoDelete(false);
}

Backup::Backup(const Backup& backup) : QRunnable(), m_id(Backup::ms_ids++), m_backup_id(backup.m_backup_id) {
	this->setAutoDelete(false);
}


int Backup::do_backup(const YAML::Node& params, const struct Option& options) {
	auto logger = spdlog::get("core");

	const YAML::Node& backup = params["backup"];
	if (backup.IsDefined() and backup.IsMap()) {
		const YAML::Node& block_size = backup["block_size"];
		if (block_size.IsDefined() and block_size.IsScalar()) {
			uint32_t new_block_size = block_size.as<uint16_t>();
			if (new_block_size != 0 and (new_block_size & (new_block_size - 1)) == 0) {
				Backup::ms_block_size = new_block_size;
				logger->info("Backup: using {} as block size", Backup::ms_block_size);
			} else {
				logger->error("Backup: wrong value of block size {}, should be a power of two", new_block_size);
				return 1;
			}
		}

		const YAML::Node& algo = backup["checksum"];
		if (algo.IsDefined() and algo.IsScalar()) {
			bool found = false;
			QString algo_name = QString::fromStdString(algo.as<std::string>());
			const Checksum * checksum = Biscuit::Checksum::find(algo_name, found);
			if (found) {
				Backup::ms_checksum = checksum;
				logger->info("Backup: using {} as checksum", checksum->name);
			} else {
				logger->error("Backup: unknown checksum {}", algo_name.toUtf8().data());
				return 1;
			}
		}
	}

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


	QElapsedTimer timer;

	logger->info("Starting backup");
	timer.start();

	Db::BackupId backup_id = connection->start_backup();
	if (backup_id.is_found()) {
		QThreadPool pool;

		const int nb_workers = pool.maxThreadCount();
		QList<Backup> workers(nb_workers, Backup(backup_id));
		for (Backup& worker : workers)
			pool.start(&worker);

		if (options.progress) {
			bool displayed = false;
			QTextStream console(stdout);
			while (not pool.waitForDone(1000)) {
				if (displayed)
					console << "\x1b[0G\x1b[" << nb_workers << "A\x1b[K\x1b[0J";
				else
					displayed = true;

				for (int i = 0; i < nb_workers; i++) {
					Backup& worker = workers[i];
					worker.m_lock.lock();
					console << "#" << i + 1 << '|' << worker.m_current_file << ": " << worker.m_current_path;
					if (worker.m_current_size > 0)
						console << ", " << worker.m_current_position << " / " << worker.m_current_size << " = " << QString::number(static_cast<double>(100 * worker.m_current_position) / worker.m_current_size, 'f', 2) << "%";
					console << Qt::endl;
					worker.m_lock.unlock();
				}
			}

			console << "\x1b[0G\x1b[" << nb_workers << "A\x1b[K\x1b[0J";
		} else
			pool.waitForDone();

		if (connection->finish_backup(backup_id))
			logger->info("Backup completed");
		else
			logger->error("Error while finishing backup");
	} else
		logger->error("Error while starting backup");

	QTime clock = QTime::fromMSecsSinceStartOfDay(timer.elapsed());
	logger->info("Time spent: {}", clock.toString("HH:mm:ss.zzz").toUtf8().data());

	delete connection;
	driver->close();

	return 0;
}

void Backup::run() {
	std::shared_ptr<spdlog::logger> logger = spdlog::get("core");

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

		for (FileInfo file_info = source->next(this->m_id, Backup::ms_ids); not file_info.is_invalid(); file_info = source->next(this->m_id, Backup::ms_ids)) {
			logger->debug("Backup: checking file: {}", file_info.path().toUtf8().data());

			static uint64_t current_file = 1;

			this->m_lock.lock();
			this->m_current_file = current_file++;
			this->m_current_path = file_info.path();
			this->m_current_position = 0;
			this->m_current_size = 0;
			this->m_lock.unlock();

			Db::FileId file_id;
			if (file_info.is_file()) {
				if (connection->is_newer_or_not_exists(file_info, host_id)) {
					file_id = connection->insert_file(file_info, host_id);
					if (file_id.is_error()) {
						logger->error("Backup: error while inserting file: {}", file_info.path().toUtf8().data());
						continue;
					}

					this->m_lock.lock();
					this->m_current_position = 0;
					this->m_current_size = file_info.file_size();
					this->m_lock.unlock();

					QIODevice * file_stream = source->open(file_info, this->m_id);

					QByteArray buffer = file_stream->read(Backup::ms_block_size);
					for (quint32 sequence = 0; buffer.size() > 0; sequence++) {
						this->m_lock.lock();
						this->m_current_position += buffer.size();
						this->m_lock.unlock();

						QByteArray digest = QCryptographicHash::hash(buffer, Backup::ms_checksum->value);

						static QHash<QByteArray, QByteArray> cache;
						static QMutex l;
						static QWaitCondition w;

						l.lock();
						while (cache.contains(digest))
							w.wait(&l);
						cache[digest] = buffer;
						l.unlock();

						// find block into db and insert it if not found then retrieve its id
						Db::BlockId block_id = connection->get_block(digest, Backup::ms_checksum->name, key_id);
						if (block_id.is_error()) {
							logger->error("Backup: error get block: #{} {}", sequence, digest.toBase64().data());

							l.lock();
							cache.remove(digest);
							w.notify_all();
							l.unlock();

							break;
						} else if (not block_id.is_found()) {
							QByteArray encrypted = key.encrypt(buffer);
							block_id = connection->insert_block(encrypted, digest, Backup::ms_checksum->name, key_id);
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

						buffer = file_stream->read(Backup::ms_block_size);
					}

					delete file_stream;
				} else {
					file_id = connection->get_file(file_info, host_id);
					if (file_id.is_error()) {
						logger->error("Backup: error while getting directory: {}", file_info.path().toUtf8().data());
						continue;
					}
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
			QByteArray digest = QCryptographicHash::hash(metadata, Backup::ms_checksum->value);
			Db::MetadataId metadata_id = connection->get_metadata(digest, Backup::ms_checksum->name);
			if (metadata_id.is_error()) {
				logger->error("Backup: error while getting metadata: {}", file_info.path().toUtf8().data());
				continue;
			} else if (not metadata_id.is_found()) {
				metadata_id = connection->insert_metadata(metadata, digest, Backup::ms_checksum->name);
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

	this->m_lock.lock();
	this->m_current_path = "idle";
	this->m_current_position = 0;
	this->m_current_size = 0;
	this->m_lock.unlock();
}
