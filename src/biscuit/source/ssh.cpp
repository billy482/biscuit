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

#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>
#include <QtCore/QByteArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QThread>
#include <sstream>
#include <string.h>
#include <sys/socket.h>
#include <yaml-cpp/yaml.h>

#include "file-info.hpp"
#include "ssh.hpp"
#include "../util.hpp"

using namespace Biscuit::Source;
using YAML::Node;

QHash<QString,QString> Ssh::ms_credential;


Ssh::Ssh(const QString& hostname, const QString& path) : Source(hostname), m_host(hostname), m_path(path), m_logger(spdlog::get("ssh")) {}

Ssh::~Ssh() {
	this->do_disconnection();
}


Ssh * Ssh::configure(const Node& file) {
	bool has_error = false;

	const Node& options = file["options"];
	const Node& ssh_options = options["ssh"];

	const Node& ssh_host = ssh_options["host"];
	if (not ssh_host.IsDefined() or not ssh_host.IsScalar())
		has_error = true;

	const Node& ssh_user = ssh_options["user"];
	if (not ssh_user.IsDefined() or not ssh_user.IsScalar())
		has_error = true;

	const Node& path = file["path"];
	if (not path.IsDefined() or not path.IsScalar())
		has_error = true;

	if (has_error)
		return nullptr;

	const Node& identity_file = ssh_options["identity_file"];
	const Node& use_agent = ssh_options["use_ssh_agent"];

	const QString hostname(ssh_host.as<std::string>().c_str());
	const QString filename(path.as<std::string>().c_str());

	Ssh * ssh = new Ssh(hostname, filename);
	ssh->m_user = QString(ssh_user.as<std::string>().c_str());
	ssh->configure_options(file);

	if (identity_file.IsDefined() and identity_file.IsScalar())
		ssh->m_identity_file = identity_file.as<std::string>().c_str();

	if (use_agent.IsDefined() and use_agent.IsScalar())
		ssh->m_use_ssh_agent = use_agent.as<bool>();

	if (ssh->do_connection()) {
		ssh->do_disconnection();
		return ssh;
	} else {
		delete ssh;
		return nullptr;
	}

}

bool Ssh::do_connection() {
	this->m_session = libssh2_session_init_ex(nullptr, nullptr, nullptr, this);
	if (this->m_session == nullptr) {
		this->m_logger->error("libssh2_session_init() => error");
		return false;
	}

	// libssh2_trace(this->m_session, 0x1FF);
	libssh2_session_set_blocking(this->m_session, 1);

	struct sockaddr_in ip_v4;
	struct sockaddr_in6 ip_v6;
	QByteArray host = this->m_host.hostname().toUtf8();

	bool connected = false;
	if (inet_pton(AF_INET6, host, &ip_v6.sin6_addr) == 1) {
		ip_v6.sin6_family = AF_INET6;
		ip_v6.sin6_port = htons(this->m_port);

		this->m_logger->info("Connecting to {}:{}", static_cast<const char *>(host), this->m_port);

		this->m_socket = socket(AF_INET6, SOCK_STREAM, 0);
		if (this->m_socket < 0)
			this->m_logger->error("Error while creating socket because {}", strerror(errno));
		else if (connect(this->m_socket, reinterpret_cast<const struct sockaddr *>(&ip_v6), sizeof(ip_v6)) != 0)
			this->m_logger->error("Error while connecting to {}:{} because {}", static_cast<const char *>(host), this->m_port, strerror(errno));
		else {
			connected = true;
			this->m_logger->info("Connected to {}:{}", static_cast<const char *>(host), this->m_port);
		}
	} else if (inet_pton(AF_INET, host, &ip_v4.sin_addr) == 1) {
		ip_v4.sin_family = AF_INET;
		ip_v4.sin_port = htons(this->m_port);

		this->m_logger->info("Connecting to {}:{}", static_cast<const char *>(host), this->m_port);

		this->m_socket = socket(AF_INET, SOCK_STREAM, 0);
		if (this->m_socket < 0)
			this->m_logger->error("Error while creating socket because {}", strerror(errno));
		else if (connect(this->m_socket, reinterpret_cast<const struct sockaddr *>(&ip_v4), sizeof(ip_v4)) != 0)
			this->m_logger->error("Error while connecting to {}:{} because {}", static_cast<const char *>(host), this->m_port, strerror(errno));
		else {
			connected = true;
			this->m_logger->info("Connected to {}:{}", static_cast<const char *>(host), this->m_port);
		}
	} else {
		struct addrinfo hints, * res;
		std::memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;

		this->m_logger->info("Resolving {]", static_cast<const char *>(host));
		int error = getaddrinfo(host, nullptr, &hints, &res);
		if (error != 0)
			this->m_logger->error("Error while resolving {} because {}", static_cast<const char *>(host), gai_strerror(error));
		else {
			for (struct addrinfo * ptr = res; ptr != nullptr and not connected; ptr = ptr->ai_next) {
				char buffer[40];
				if (inet_ntop(ptr->ai_family, ptr->ai_addr, buffer, 40) != nullptr)
					this->m_logger->info("Connecting to {}:{}", buffer, this->m_port);
				else
					this->m_logger->info("Connecting to {}:{}", static_cast<const char *>(host), this->m_port);

				this->m_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
				if (this->m_socket < 0) {
					this->m_logger->error("Error while creating socket because {}", strerror(errno));
					break;
				}

				if (connect(this->m_socket, ptr->ai_addr, ptr->ai_addrlen) == 0) {
					connected = true;
					this->m_logger->info("Connected to {}:{}", static_cast<const char *>(host), this->m_port);
				} else {
					close(this->m_socket);
					this->m_socket = -1;
					this->m_logger->error("Error while connecting to {}:{} because {}", static_cast<const char *>(host), this->m_port, strerror(errno));
				}
			}

			freeaddrinfo(res);
		}
	}

	if (not connected) {
		this->do_disconnection();
		return false;
	}

	bool authenticated = false;
	int error = libssh2_session_handshake(this->m_session, this->m_socket);
	if (error != 0) {
		this->log_error(LIBSSH2_METHOD_KEX, "key exchange");
		this->log_error(LIBSSH2_METHOD_HOSTKEY, "host key");
		this->log_error(LIBSSH2_METHOD_CRYPT_CS, "crypt cs");
		this->log_error(LIBSSH2_METHOD_MAC_CS, "mac cs");
		this->log_error(LIBSSH2_METHOD_COMP_CS, "commpression cs");

		this->do_disconnection();
		return false;
	}

	{
		QByteArray finger_print(libssh2_hostkey_hash(this->m_session, LIBSSH2_HOSTKEY_HASH_SHA1), 20);
		this->m_logger->debug("finger print (base64): {}", static_cast<const char *>(finger_print.toBase64()));
		this->m_logger->debug("finger print (hex): {}", static_cast<const char *>(finger_print.toHex()));
	}

	QByteArray user_name = this->m_user.toUtf8();
	if (this->m_use_ssh_agent) {
		LIBSSH2_AGENT * agent = libssh2_agent_init(this->m_session);
		int rc = libssh2_agent_connect(agent);
		if (rc == 0) {
			this->m_logger->info("connected to agent");

			rc = libssh2_agent_list_identities(agent);
			if (rc != 0)
				this->log_error();
			else {
				struct libssh2_agent_publickey * identity = nullptr, * previous_identity = nullptr;

				for (;;) {
					rc = libssh2_agent_get_identity(agent, &identity, previous_identity);
					if (rc == 1)
						break;
					if (rc < 0) {
						this->log_error();
						break;
					}

					rc = libssh2_agent_userauth(agent, user_name, identity);
					if (rc == 0) {
						authenticated = true;
						this->m_logger->info("connected");
						break;
					}

					previous_identity = identity;
				}
			}
		}
		libssh2_agent_free(agent);
	}

	if (authenticated)
		return true;

	char * user_auth_list = libssh2_userauth_list(this->m_session, user_name, user_name.size());

	if (not this->m_identity_file.isNull() and strstr(user_auth_list, "publickey") != nullptr) {
		const QByteArray identity_file = this->m_identity_file.toUtf8();
		const QByteArray identity_file_pub = (this->m_identity_file + ".pub").toUtf8();
		const QString prompt = QString("%1@%2").arg(this->m_user, this->m_identity_file);
		QString password;
		QByteArray password2;

		if (Ssh::ms_credential.contains(prompt)) {
			password = Ssh::ms_credential[prompt];
			password2 = password.toUtf8();
		}

		error = libssh2_userauth_publickey_fromfile(this->m_session, user_name, identity_file_pub, identity_file, password2.size() > 0 ? password2 : nullptr);
		if (error != 0) {
			password = get_password(prompt);
			password2 = password.toUtf8();

			error = libssh2_userauth_publickey_fromfile(this->m_session, user_name, identity_file_pub, identity_file, password2);
			if (error == 0) {
				Ssh::ms_credential[prompt] = password;
				authenticated = true;
			}
		} else
			authenticated = true;
	}

	if (authenticated)
		return true;

	if (strstr(user_auth_list, "password") != nullptr) {
		const QString prompt = QString("%1@%2").arg(this->m_user, this->m_identity_file);
		QString password;

		if (Ssh::ms_credential.contains(prompt))
			password = Ssh::ms_credential[prompt];
		else
			password = get_password(prompt);

		error = libssh2_userauth_password(this->m_session, user_name, static_cast<const char *>(password.toUtf8()));
		for (int i = 0; i < 2 and error == LIBSSH2_ERROR_AUTHENTICATION_FAILED; i++) {
			password = get_password(prompt);
			error = libssh2_userauth_password(this->m_session, user_name, static_cast<const char *>(password.toUtf8()));
		}
		if (error == 0)
			authenticated = true;

	}

	return authenticated;
}

void Ssh::do_disconnection() {
	if (this->m_session != nullptr) {
		libssh2_session_free(this->m_session);
		this->m_session = nullptr;
	}
	close(this->m_socket);
	this->m_socket = -1;
}

void Ssh::init() {
	libssh2_init(0);
}

void Ssh::log_error() {
	char * error_message = nullptr;
	int error_len = 0;
	libssh2_session_last_error(this->m_session, &error_message, &error_len, 1);
	this->m_logger->error(error_message);
}

void Ssh::log_error(int type, const char * message) {
	const char ** algo = nullptr;
	int rc = libssh2_session_supported_algs(this->m_session, type, &algo);
	if (rc > 0) {
		std::stringstream buffer;
		for (int i = 0; i < rc; i++) {
			if (i > 0)
				buffer << ", ";
			buffer << '\'' << algo[i] << '\'';
		}
		libssh2_free(this->m_session, algo);
		this->m_logger->debug("supported {}: {}", message, buffer.str());
	} else
		this->m_logger->error("Error while listing {}", message);
}

FileInfo Ssh::next(uint16_t worker, uint16_t total_workers) {
	this->m_lock.lock();

	if (this->m_session == nullptr and not this->do_connection()) {
		this->m_lock.unlock();
		return FileInfo();
	}

	bool start = false;
	if (this->m_channels == nullptr) {
		this->m_channels = new LIBSSH2_SFTP*[total_workers];
		start = true;
	}
	if (this->m_channels[worker] == nullptr)
		this->m_channels[worker] = libssh2_sftp_init(this->m_session);

	if (start and not this->scan_directory(this->m_channels[worker], this->m_path)) {
		this->m_lock.unlock();
		return FileInfo();
	}

	while (not this->m_paths.isEmpty()) {
		QList<FileInfo>& files = this->m_paths.top();
		if (files.size() == 0) {
			this->m_paths.pop();
			continue;
		}

		FileInfo file = files.first();
		files.pop_front();

		if (file.is_dir()) {
			this->scan_directory(this->m_channels[worker], file.path());
		}

		this->m_lock.unlock();

		return file;
	}

	this->m_lock.unlock();
	return FileInfo();
}

QIODevice * Ssh::open(const FileInfo& file, uint16_t worker) {
	QByteArray path = file.path().toUtf8();
	LIBSSH2_SFTP_HANDLE * handle = libssh2_sftp_open(this->m_channels[worker], path, LIBSSH2_FXF_READ, 0);
	if (handle != nullptr)
		return new SshFile(handle);
	else
		return nullptr;
}

bool Ssh::scan_directory(LIBSSH2_SFTP * channel, const QString& path) {
	const QByteArray byte_path = path.toUtf8();
	LIBSSH2_SFTP_HANDLE * sftp_handle = libssh2_sftp_opendir(channel, byte_path);
	if (sftp_handle == nullptr)
		return false;

	char mem[512], long_entry[512];
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	QList<FileInfo> files;

	for (int rc = 1; rc > 0;) {
		rc = libssh2_sftp_readdir_ex(sftp_handle, mem, sizeof(mem), long_entry, sizeof(long_entry), &attrs);
		if (rc > 0) {
			if (mem[0] == '.' and (mem[1] == '\0' or (mem[1] == '.' and mem[2] == '\0')))
				continue;

			QString file = QString("%1/%2").arg(path != "/" ? path : "", mem);

			QJsonObject md_common;
			md_common.insert("size", QJsonValue(static_cast<qint64>(attrs.filesize)));

			QJsonObject md_unix;
			md_unix.insert("owner", QJsonValue(static_cast<qint64>(attrs.uid)));
			md_unix.insert("group", QJsonValue(static_cast<qint64>(attrs.gid)));

			QJsonObject metadata;
			metadata.insert("common", md_common);
			metadata.insert("unix", md_unix);

			FileType type = FileType::Unknown;
			if (S_ISREG(attrs.permissions))
				type = FileType::File;
			else if (S_ISDIR(attrs.permissions))
				type = FileType::Directory;
			else if (S_ISLNK(attrs.permissions))
				type = FileType::SymLink;

			files << FileInfo(file, QDateTime::fromSecsSinceEpoch(attrs.mtime), type, attrs.filesize, QJsonDocument(metadata));
		} else if (rc < 0) {
			libssh2_sftp_closedir(sftp_handle);
			return false;
		}
	}

	libssh2_sftp_closedir(sftp_handle);

	if (files.size() > 0) {
		std::sort(files.begin(), files.end());
		this->m_paths.push(files);
	}
	
	return true;
}



Ssh::SshFile::SshFile(LIBSSH2_SFTP_HANDLE * sftp_handle) : QIODevice(), m_handle(sftp_handle) {
	this->setOpenMode(QIODeviceBase::ReadOnly);
}

Ssh::SshFile::~SshFile() {
	this->close();
}


void Ssh::SshFile::close() {
	if (this->m_handle != nullptr)
		libssh2_sftp_close_handle(this->m_handle);
	this->m_handle = nullptr;
}

qint64 Ssh::SshFile::readData(char * data, qint64 max_size) {
	if (this->m_handle != nullptr)
		return libssh2_sftp_read(this->m_handle, data, max_size);
	else
		return -1;
}

qint64 Ssh::SshFile::writeData(const char *data, qint64 max_size) {
	if (this->m_handle != nullptr)
		return libssh2_sftp_write(this->m_handle, data, max_size);
	else
		return -1;
}
