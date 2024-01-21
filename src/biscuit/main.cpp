#include <clipp.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <yaml-cpp/yaml.h>

#include "db/connection.hpp"
#include "db/driver.hpp"
#include "key.hpp"
#include "source/source.hpp"
#include "worker/backup.hpp"

namespace Biscuit {
	enum class modes {backup, exit, help, restore};

	spdlog::level::level_enum find_log_level(const std::string& level);
	modes parse_arg(int argc, char * argv[]);

	YAML::Node configuration;
}


spdlog::level::level_enum Biscuit::find_log_level(const std::string& level) {
	static struct levels {
		const char * name;
		spdlog::level::level_enum value;
	} levels[] = {
		{ "trace", spdlog::level::trace },
		{ "debug", spdlog::level::debug },
		{ "info", spdlog::level::info },
		{ "warn", spdlog::level::warn },
		{ "error", spdlog::level::err },
		{ "critical", spdlog::level::critical },
		{ "off", spdlog::level::off },

		{ nullptr, spdlog::level::off }
	};

	for (struct levels * ptr = levels; ptr->name != nullptr; ptr++)
		if (level == ptr->name)
			return ptr->value;

	return spdlog::level::off;
}

Biscuit::modes Biscuit::parse_arg(int argc, char * argv[]) {
	using namespace clipp;

	auto ring_buffer_sink = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(128);
	auto logger = std::make_shared<spdlog::logger>("logger_name", ring_buffer_sink);
	logger->set_level(spdlog::level::trace);

	logger->info("Starting biscuit");
	logger->info("Parsing parameters");

	modes mode = modes::backup;

	auto backup = (
		"backup options" % in_sequence(
			command("backup").set(mode, modes::backup),
			option("-h", "--help").set(mode, modes::help) % "Show help"
		)
	);

	auto help = (
		command("help").set(mode, modes::help)
	);

	auto restore = (
		"restore options" % in_sequence(
			command("restore").set(mode, modes::restore),
			option("-h", "--help").set(mode, modes::help) % "Show help"
		)
	);

	auto cli = (
		(backup | help | restore)
	);

	if (parse(argc, argv, cli)) {
		if (mode == modes::help)
			std::cout << make_man_page(cli, "biscuit");
		else {
			logger->debug("Logging \"biscuit.yaml\"");
			bool failed = true;
			try {
				Biscuit::configuration = YAML::LoadFile("biscuit.yaml");
				const YAML::Node& node_log = Biscuit::configuration["log"];

				if (node_log.IsMap()) {
					const YAML::Node& node_path = node_log["path"];

					if (node_path.IsScalar()) {
						std::string str_path = node_path.as<std::string>();
						auto daily_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(str_path, 0, 0);

						const YAML::Node& node_level = node_log["levels"];
						for (const char * module: {"core", "database", "ssh"}) {
							const YAML::Node& node_module = node_level[module];
							spdlog::level::level_enum level = spdlog::level::warn;
							if (node_module.IsScalar())
								level = find_log_level(node_module.as<std::string>());

							auto logger = std::make_shared<spdlog::logger>(module, daily_sink);
							logger->set_level(level);
							spdlog::register_logger(logger);
						}

						spdlog::flush_every(std::chrono::seconds(1));
					}
				}

				failed = false;
			} catch (const YAML::BadFile &ex) {
				logger->critical("Error while loading {}", "biscuit.yaml");
			} catch (const YAML::ParserException& ex) {
				logger->critical("Error while loading {}", "biscuit.yaml");
			}

			if (failed) {
				auto sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
				auto logger = std::make_shared<spdlog::logger>("core", sink);
				logger->set_level(spdlog::level::trace);

				for (const spdlog::details::log_msg_buffer& msg : ring_buffer_sink->last_raw())
					logger->log(msg.time, msg.source, msg.level, msg.payload);
			} else {
				auto logger = spdlog::get("core");
				for (const spdlog::details::log_msg_buffer& msg : ring_buffer_sink->last_raw())
					logger->log(msg.time, msg.source, msg.level, msg.payload);
			}

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
		}
		return mode;
	} else {
		std::cout << make_man_page(cli, "biscuit");
		return modes::exit;
	}
}

int main(int argc, char * argv[]) {
	switch (Biscuit::parse_arg(argc, argv)) {
		case Biscuit::modes::backup:
			return Biscuit::Worker::Backup::do_backup(Biscuit::configuration);

		case Biscuit::modes::exit:
			return 1;

		case Biscuit::modes::help:
			return 0;

		case Biscuit::modes::restore:
			return 0;
	}
}
