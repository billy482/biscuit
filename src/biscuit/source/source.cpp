#include <QtCore/QDir>
#include <string>
#include <yaml-cpp/yaml.h>

#include "file.hpp"
#include "source.hpp"

using namespace Biscuit::Source;
using YAML::Node;


Source * Source::ms_first = nullptr;
Source * Source::ms_last = nullptr;

Source::~Source() {}


Source * Source::first_source() {
	return Source::ms_first;
}

bool Source::parse(const Node& config) {
	if (not config.IsMap())
		return false;

	for (YAML::const_iterator iter = config.begin(); iter != config.end(); iter++) {
		const std::string path = iter->first.as<std::string>();
		Source * src = File::configure(path.c_str(), iter->second);

		if (src == nullptr)
			return false;

		if (Source::ms_first == nullptr)
			Source::ms_first = Source::ms_last = src;
		else {
			src->m_previous = Source::ms_last;
			Source::ms_last->m_next = src;
			Source::ms_last = src;
		}
	}

	return true;
}