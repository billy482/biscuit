#include <QtCore/QDir>
#include <string>
#include <yaml-cpp/yaml.h>

#include "source.hpp"

using namespace Biscuit;
using YAML::Node;

Source::Source(const QString& path) : m_root(path) {
	this->m_paths.push(QDir(path).entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name | QDir::LocaleAware));
}


QFileInfo Source::next() {
	this->m_lock.lock();

	while (not this->m_paths.isEmpty()) {
		QFileInfoList& files = this->m_paths.top();
		if (files.size() == 0) {
			this->m_paths.pop();
			continue;
		}

		QFileInfo file = files.first();
		files.pop_front();

		if (file.isFile()) {
			if (this->m_include_pattern.size() > 0) {
				bool has_matched = false;
				for (auto iter = this->m_include_pattern.begin(); iter != this->m_include_pattern.end(); iter++) {
					QRegularExpressionMatch match = iter->match(file.fileName());
					if (match.hasMatch()) {
						has_matched = true;
						break;
					}
				}

				if (not has_matched)
					continue;
			}

			if (this->m_exclude_pattern.size() > 0) {
				bool has_matched = false;
				for (auto iter = this->m_exclude_pattern.begin(); iter != this->m_exclude_pattern.end(); iter++) {
					QRegularExpressionMatch match = iter->match(file.absolutePath());
					if (match.hasMatch()) {
						has_matched = true;
						break;
					}
				}

				if (has_matched)
					continue;
			}
		}

		if (this->m_exclude_path.size() > 0) {
			bool has_matched = false;
			for (auto iter = this->m_exclude_path.begin(); iter != this->m_exclude_path.end(); iter++) {
				QString& path = *iter;

				if (path.startsWith('/')) {
					QString sub_path = file.absoluteFilePath().mid(this->m_root.absoluteFilePath().length());
					if (sub_path == path) {
						has_matched = true;
						break;
					}
				} else if (path == file.fileName()) {
					has_matched = true;
					break;
				}
			}

			if (has_matched)
				continue;
		}
		
		if (file.isDir())
			this->m_paths.push(QDir(file.absoluteFilePath()).entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name | QDir::LocaleAware));

		this->m_lock.unlock();
		return file;
	}

	this->m_lock.unlock();
	return QFileInfo();
}

void Source::parse(const Node& node) {
	if (not node.IsMap())
		return;

	const Node& include_patterns = node["include_patterns"];
	if (include_patterns.IsSequence()) {
		for (YAML::const_iterator iter = include_patterns.begin(); iter != include_patterns.end(); iter++) {
			std::string str_pattern = iter->as<std::string>();
			this->m_include_pattern.append(QRegularExpression::fromWildcard(QString(str_pattern.c_str())));
		}
	}

	const Node& exclude_paths = node["exclude"];
	if (exclude_paths.IsSequence()) {
		for (YAML::const_iterator iter = exclude_paths.begin(); iter != exclude_paths.end(); iter++) {
			std::string str_pattern = iter->as<std::string>();
			this->m_exclude_path.append(QString(str_pattern.c_str()));
		}
	}

	const Node& exclude_patterns = node["exclude_patterns"];
	if (exclude_patterns.IsSequence()) {
		for (YAML::const_iterator iter = exclude_patterns.begin(); iter != exclude_patterns.end(); iter++) {
			std::string str_pattern = iter->as<std::string>();
			this->m_exclude_pattern.append(QRegularExpression::fromWildcard(QString(str_pattern.c_str())));
		}
	}
}