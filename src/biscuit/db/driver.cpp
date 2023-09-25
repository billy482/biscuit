#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include "driver.hpp"
#include "sqlite/driver.hpp"

using namespace Biscuit::Db;
using Biscuit::Db::Sqlite::SqliteDriver;
using YAML::Node;

Driver * Driver::ms_instance = nullptr;

Driver::Driver(const QString& name) : m_name(name) {}

Driver::~Driver() {
	if (this == Driver::ms_instance)
		Driver::ms_instance = nullptr;
}


bool Driver::configure(const Node& node) {
	auto logger = spdlog::get("database");
	const Node& driver = node["driver"];
	if (not driver.IsScalar()) {
		logger->error("Error: driver is not a scalar");
		return false;
	}

	QString str_driver(driver.as<std::string>().c_str());
	if (str_driver == "sqlite") {
		logger->debug("Configuring sqlite driver");
		Driver::ms_instance = SqliteDriver::configure(node);
	} else {
		logger->error("Error: driver \"{}\" not available", str_driver.toUtf8().data());
		return false;
	}

	return true;
}

Driver * Driver::get() {
	return Driver::ms_instance;
}