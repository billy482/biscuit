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

#include <clipp.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <yaml-cpp/yaml.h>

#include "string.hpp"

/*
#include "db/connection.hpp"
#include "db/driver.hpp"
#include "key.hpp"
#include "options.hpp"
#include "source/source.hpp"
#include "worker/backup.hpp"
*/

namespace Biscuit {
	enum class modes {backup, exit, help, restore};

	spdlog::level::level_enum find_log_level(const String& level);
	modes parse_arg(int argc, char * argv[]);

	static YAML::Node configuration;

	// static struct Option options;
}

using Biscuit::String;


spdlog::level::level_enum Biscuit::find_log_level(const String& level) {
	static struct levels {
		String name;
		spdlog::level::level_enum value;
	} levels[] = {
		{ "trace", spdlog::level::trace },
		{ "debug", spdlog::level::debug },
		{ "info", spdlog::level::info },
		{ "warn", spdlog::level::warn },
		{ "error", spdlog::level::err },
		{ "critical", spdlog::level::critical },
		{ "off", spdlog::level::off },

		{ String(), spdlog::level::off }
	};

	for (struct levels * ptr = levels; not ptr->name.is_null(); ptr++)
		if (level == ptr->name)
			return ptr->value;

	return spdlog::level::off;
}

Biscuit::modes Biscuit::parse_arg(int argc, char * argv[]) {
	using namespace clipp;

	std::shared_ptr<spdlog::sinks::ringbuffer_sink_mt> ring_buffer_sink = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(128);
	std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>("logger_name", ring_buffer_sink);
	logger->set_level(spdlog::level::trace);

	logger->info("Starting biscuit");
	logger->info("Parsing parameters");

	modes mode = modes::backup;

	clipp::group backup = (
		"backup options" % (
			command("backup").set(mode, modes::backup),
			option("-h", "--help").set(mode, modes::help) % "Show help"
			// option("-p", "--progress").set(options.progress) % "Display progression"
		)
	);

	clipp::parameter help = (
		command("help").set(mode, modes::help)
	);

	clipp::group restore = (
		"restore options" % (
			command("restore").set(mode, modes::restore),
			option("-h", "--help").set(mode, modes::help) % "Show help"
		)
	);

	clipp::group cli = (
		(backup | help | restore)
	);

	if (parse(argc, argv, cli)) {
		if (mode == modes::help) {
			std::cout << make_man_page(cli, "biscuit");
			return mode;
		}

		bool failed = true;
		logger->debug("Logging \"biscuit.yaml\"");

		try {
			Biscuit::configuration = YAML::LoadFile("biscuit.yaml");
			const YAML::Node& node_log = Biscuit::configuration["log"];

			if (node_log.IsDefined() and node_log.IsMap()) {
				const YAML::Node& node_path = node_log["path"];

				if (node_path.IsDefined() and node_path.IsScalar()) {
					std::string str_path = node_path.as<std::string>();
					std::shared_ptr<spdlog::sinks::daily_file_sink_mt> daily_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(str_path, 0, 0);

					const YAML::Node& node_level = node_log["levels"];
					if (node_level.IsDefined() and node_level.IsMap()) {
						for (const char * module: {"core", "database", "ssh"}) {
							const YAML::Node& node_module = node_level[module];
							spdlog::level::level_enum level = spdlog::level::warn;
							if (node_module.IsDefined() and node_module.IsScalar())
								level = find_log_level(node_module.as<std::string>().c_str());

							std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>(module, daily_sink);
							logger->set_level(level);
							spdlog::register_logger(logger);
						}

						spdlog::flush_every(std::chrono::seconds(1));
					}
				}
			}

			failed = false;
		} catch (const YAML::BadFile &ex) {
			logger->critical("Error while loading {}", "biscuit.yaml");
		} catch (const YAML::ParserException& ex) {
			logger->critical("Error while loading {}", "biscuit.yaml");
		}

		if (failed) {
			std::shared_ptr<spdlog::sinks::stderr_color_sink_mt> sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
			std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>("core", sink);
			logger->set_level(spdlog::level::trace);

			for (const spdlog::details::log_msg_buffer& msg : ring_buffer_sink->last_raw())
				logger->log(msg.time, msg.source, msg.level, msg.payload);
		} else {
			std::shared_ptr<spdlog::logger> logger = spdlog::get("core");
			for (const spdlog::details::log_msg_buffer& msg : ring_buffer_sink->last_raw())
				logger->log(msg.time, msg.source, msg.level, msg.payload);
		}

		/*
		const YAML::Node& node_db = Biscuit::configuration["database"];
		if (not Db::Driver::configure(node_db)) {
			logger->critical("Failed to configure database driver");
			return modes::exit;
		}

		const YAML::Node& node_sources = Biscuit::configuration["sources"];
		if (not Source::Source::parse(node_sources)) {
			logger->critical("Failed to configure sources");
			return modes::exit;
		}

		const YAML::Node& node_key = Biscuit::configuration["key"];
		if (not Key::configure(node_key)) {
			logger->critical("Failed to configure key");
			return modes::exit;
		}
		*/
		return mode;
	} else {
		std::cout << make_man_page(cli, "biscuit");
		return modes::exit;
	}
}

int main(int argc, char * argv[]) {
	switch (Biscuit::parse_arg(argc, argv)) {
		case Biscuit::modes::backup:
			// return Biscuit::Worker::Backup::do_backup(Biscuit::configuration, Biscuit::options);
			return 0;

		case Biscuit::modes::exit:
			return 1;

		case Biscuit::modes::help:
			return 0;

		case Biscuit::modes::restore:
			return 0;
	}
}
