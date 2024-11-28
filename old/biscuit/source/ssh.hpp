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

#ifndef __BISCUIT_SOURCE_SSH_HPP__
#define __BISCUIT_SOURCE_SSH_HPP__

#include <libssh2.h>
#include <libssh2_sftp.h>
#include <QtCore/QIODevice>
#include <QtCore/QList>
#include <QtCore/QStack>
#include <spdlog/spdlog.h>

#include "source.hpp"
#include "../host.hpp"

typedef struct _LIBSSH2_SESSION LIBSSH2_SESSION;

namespace Biscuit {
	namespace Source {
		class Ssh : public Source {
			public:
				virtual ~Ssh();

				static Ssh * configure(const YAML::Node& file);
				virtual FileInfo next(uint16_t worker, uint16_t total_workers) override;
				virtual QIODevice * open(const FileInfo& file, uint16_t worker) override;

			private:
				class SshSession {
					public:
						SshSession(Ssh& ssh);
						~SshSession();

						bool do_connection();
						void do_disconnection();
						QIODevice * open(const FileInfo& file);
						bool scan_directory(const QString &path);
						bool start_ftp_session();

					private:
						void log_error();
						void log_error(int type, const char * message);

						Ssh& m_ssh;
						int m_socket = -1;
						LIBSSH2_SESSION * m_session = nullptr;
						LIBSSH2_SFTP * m_channel = nullptr;
				};

				class SshFile : public QIODevice {
					public:
						SshFile(LIBSSH2_SFTP_HANDLE * sftp_handle);
						virtual ~SshFile();

						virtual void close() override;

					protected:
						virtual qint64 readData(char * data, qint64 max_size) override;
						virtual qint64 writeData(const char * data, qint64 max_size) override;

					private:
						LIBSSH2_SFTP_HANDLE * m_handle;
				};

				Ssh(const QString& hostname, const QString& path);

				static void init() __attribute__((constructor));

				int m_connection_timeout = 10000;
				Host m_host;
				uint16_t m_port = 22;
				QString m_user;
				bool m_use_ssh_agent = false;
				QString m_identity_file;
				QString m_path;
				static QHash<QString,QString> ms_credential;
				SshSession ** m_sessions = nullptr;
				uint16_t m_nb_sessions = 0;
				QStack<QList<FileInfo>> m_paths;
				std::shared_ptr<spdlog::logger> m_logger;
		};
	}
}

#endif
