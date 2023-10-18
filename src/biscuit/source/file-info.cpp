#include <QtCore/QFileInfo>

#include "file-info.hpp"

using namespace Biscuit::Source;

FileInfo::FileInfo() : m_path(), m_modified_time(), m_is_invalid(true) {}

FileInfo::FileInfo(const QString& path, const QDateTime& modified_time) : m_path(path), m_modified_time(modified_time), m_is_invalid(false) {}

FileInfo::FileInfo(const QFileInfo& info) : m_path(info.absolutePath()), m_modified_time(info.lastModified()), m_is_invalid(not info.exists()) {}

FileInfo::FileInfo(const FileInfo& info) : m_path(info.m_path), m_modified_time(info.m_modified_time), m_is_invalid(info.m_is_invalid) {}


FileInfo& FileInfo::operator=(const FileInfo& info) {
	this->m_path = info.m_path;
	this->m_modified_time = info.m_modified_time;
	this->m_is_invalid = info.m_is_invalid;
	return *this;
}

FileInfo& FileInfo::operator=(const QFileInfo& info) {
	this->m_path = info.absolutePath();
	this->m_modified_time = info.lastModified();
	this->m_is_invalid = not info.exists();
	return *this;
}