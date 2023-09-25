#include <clipp.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <yaml-cpp/yaml.h>

#include "db/connection.hpp"
#include "db/driver.hpp"
#include "source.hpp"

namespace Biscuit {
	bool configure_log(const YAML::Node& node);
	spdlog::level::level_enum find_log_level(const std::string& level);
}


bool Biscuit::configure_log(const YAML::Node& node) {
	const YAML::Node& node_path = node["path"];
	if (node_path.IsScalar()) {
		std::string str_path = node_path.as<std::string>();
		auto daily_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(str_path, 0, 0);

		const YAML::Node& node_level = node["levels"];
		for (const char * module: {"core", "database", "ssh"}) {
			const YAML::Node& node_module = node_level[module];
			spdlog::level::level_enum level = spdlog::level::warn;
			if (node_module.IsScalar())
				level = find_log_level(node_module.as<std::string>());

			auto logger = std::make_shared<spdlog::logger>(module, daily_sink);
			logger->set_level(level);
			spdlog::register_logger(logger);
		}

		spdlog::flush_every(std::chrono::seconds(5));
	}

	return true;
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

int main(int, char *[]) {
	/*
	using namespace clipp;

	enum class modes {backup, help, restore};
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
		switch (mode) {
			case modes::backup:
				break;

			case modes::help:
				std::cout << make_man_page(cli, "biscuit");
				break;

			case modes::restore:
				break;
		}
	} else
		std::cout << make_man_page(cli, "biscuit");
	*/

	try {
		spdlog::debug("Starting biscuit");
		spdlog::debug("Logging \"biscuit.yaml\"");

		YAML::Node node = YAML::LoadFile("biscuit.yaml");
		spdlog::debug("\"biscuit.yaml\" loaded");

		spdlog::debug("Configuring logs");
		if (not Biscuit::configure_log(node["log"])) {
			spdlog::debug("Error while configuring logs");
			return 1;
		}

		auto logger = spdlog::get("core");
		logger->debug("Loading database");
		Biscuit::Db::Driver::configure(node["database"]);
		Biscuit::Db::Driver * driver = Biscuit::Db::Driver::get();
		Biscuit::Db::Connection * connection = driver->open();
		connection->connected();

		const YAML::Node& sources = node["sources"];

		for (auto iter : sources) {
			std::string key = iter.first.as<std::string>();
			const YAML::Node& value = sources[key];

			Biscuit::Source src(QString(key.c_str()));
			src.parse(value);

			for (;;) {
				QFileInfo file_info = src.next();
				qDebug() << file_info;
				if (not file_info.exists())
					break;
			}
		}
	} catch (const YAML::ParserException& ex) {
		std::cout << ex.what() << std::endl;
	} catch (const YAML::BadFile& ex) {
		std::cout << ex.what() << std::endl;
	}

	return 0;
}