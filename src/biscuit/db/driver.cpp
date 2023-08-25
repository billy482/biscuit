#include <yaml-cpp/yaml.h>

#include "driver.hpp"
#include "sqlite/driver.hpp"

using namespace Biscuit::Db;
using Biscuit::Db::Sqlite::SQliteDriver;
using YAML::Node;

Driver * Driver::ms_instance = nullptr;

Driver::Driver(const QString& name) : m_name(name) {}

Driver::~Driver() {
	if (this == Driver::ms_instance)
		Driver::ms_instance = nullptr;
}


bool Driver::configure(const Node& node) {
	const Node& driver = node["driver"];
	if (not driver.IsScalar())
		return false;

	QString str_driver(driver.as<std::string>().c_str());
	if (str_driver == "sqlite")
		Driver::ms_instance = SQliteDriver::configure(node);

	return true;
}