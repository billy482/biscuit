#include <QtCore/QDir>
#include <string>
#include <yaml-cpp/yaml.h>

#include "source.hpp"

using namespace Biscuit;
using YAML::Node;

QList<Source> Source::ms_sources;

Source::Source(const QString& path) : m_root(path) {
	this->m_paths.push(QDir(path).entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name | QDir::LocaleAware));
}

Source::Source(const Source& source) : m_root(source.m_root), m_paths(source.m_paths), m_include_pattern(source.m_include_pattern), m_exclude_pattern(source.m_exclude_pattern), m_exclude_path(source.m_exclude_path) {}

Source::Source(Source&& source) : m_root(source.m_root), m_paths(std::move(source.m_paths)), m_include_pattern(std::move(source.m_include_pattern)), m_exclude_pattern(std::move(source.m_exclude_pattern)), m_exclude_path(std::move(source.m_exclude_path)) {}


QList<Source>& Source::get() {
	return Source::ms_sources;
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

QIODevice * Source::open(const QFileInfo& file) {
	QFile * new_file = new QFile(file.absoluteFilePath());
	if (new_file->open(QIODevice::ReadOnly))
		return new_file;
	else {
		delete new_file;
		return nullptr;
	}
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

void Source::parse(const QString& path, const YAML::Node& node) {
	Source::ms_sources << Source(path);
	Source::ms_sources.last().parse(node);
}


Source& Source::operator=(const Source& source) {
	this->m_root = source.m_root;
	this->m_paths = source.m_paths;
	this->m_include_pattern = source.m_include_pattern;
	this->m_exclude_pattern = source.m_exclude_pattern;
	this->m_exclude_path = source.m_exclude_path;
	return *this;
}

Source& Source::operator=(Source&& source) {
	this->m_root = source.m_root;
	this->m_paths = std::move(source.m_paths);
	this->m_include_pattern = std::move(source.m_include_pattern);
	this->m_exclude_pattern = std::move(source.m_exclude_pattern);
	this->m_exclude_path = std::move(source.m_exclude_path);
	return *this;
}