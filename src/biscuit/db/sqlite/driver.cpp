#include <yaml-cpp/yaml.h>

#include "driver.hpp"

using namespace Biscuit::Db::Sqlite;
using Biscuit::Db::Connection;
using YAML::Node;

SQliteDriver::SQliteDriver(const QFileInfo& path) : Driver("sqlite"), m_path(path) {}


SQliteDriver * SQliteDriver::configure(const Node& node) {
	const Node& path = node["path"];
	if (not path.IsScalar())
		return nullptr;

	return new SQliteDriver(QFileInfo(path.as<std::string>().c_str()));
}

Connection * SQliteDriver::open() {
	return nullptr;
}