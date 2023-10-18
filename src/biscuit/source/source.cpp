#include <QtCore/QDir>
#include <string>
#include <yaml-cpp/yaml.h>

#include "source.hpp"

using namespace Biscuit::Source;
using YAML::Node;


Source * Source::ms_first = nullptr;
Source * Source::ms_last = nullptr;

Source::~Source() {}


Source * Source::first_source() {
	return Source::ms_first;
}

/*
QIODevice * Source::open(const QFileInfo& file) {
	QFile * new_file = new QFile(file.absoluteFilePath());
	if (new_file->open(QIODevice::ReadOnly))
		return new_file;
	else {
		delete new_file;
		return nullptr;
	}
}
*/