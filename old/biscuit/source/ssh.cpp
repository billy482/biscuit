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

#include <yaml-cpp/yaml.h>

#include "file-info.hpp"
#include "ssh.hpp"
#include "../util.hpp"

using namespace Biscuit::Source;
using YAML::Node;

QHash<QString,QString> Ssh::ms_credential;


Ssh::Ssh(const QString& hostname, const QString& path) : Source(hostname), m_host(hostname), m_path(path), m_logger(spdlog::get("ssh")) {}

Ssh::~Ssh() {
	if (this->m_sessions != nullptr) {
		for (uint16_t i = 0; i < this->m_nb_sessions; i++)
			if (this->m_sessions[i] != nullptr) {
				this->m_sessions[i]->do_disconnection();
				delete this->m_sessions[i];
			}
		delete [] this->m_sessions;
		this->m_sessions = nullptr;
		this->m_nb_sessions = 0;
	}
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

	SshSession session(*ssh);
	if (session.do_connection()) {
		session.do_disconnection();
		return ssh;
	} else {
		delete ssh;
		return nullptr;
	}
}

void Ssh::init() {
	libssh2_init(0);
}

FileInfo Ssh::next(uint16_t worker, uint16_t total_workers) {
	this->m_lock.lock();

	bool start = false;
	if (this->m_sessions == nullptr) {
		this->m_sessions = new SshSession*[total_workers];
		this->m_nb_sessions = total_workers;
		start = true;
	}

	if (this->m_sessions[worker] == nullptr) {
		this->m_sessions[worker] = new SshSession(*this);
		if (not this->m_sessions[worker]->do_connection()) {
			delete this->m_sessions[worker];
			this->m_sessions[worker] = nullptr;
			this->m_lock.unlock();
			return FileInfo();
		}

		if (not this->m_sessions[worker]->start_ftp_session()) {
			delete this->m_sessions[worker];
			this->m_sessions[worker] = nullptr;
			this->m_lock.unlock();
			return FileInfo();
		}
	}

	if (start and not this->m_sessions[worker]->scan_directory(this->m_path)) {
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
			this->m_sessions[worker]->scan_directory(file.path());
		}

		this->m_lock.unlock();

		return file;
	}

	this->m_lock.unlock();
	return FileInfo();
}

QIODevice * Ssh::open(const FileInfo& file, uint16_t worker) {
	if (this->m_sessions == nullptr or this->m_sessions[worker] == nullptr)
		return nullptr;
	else
		return this->m_sessions[worker]->open(file);
}
