#include <QtCore/QFileInfo>

#include "file-info.hpp"

using namespace Biscuit::Source;

FileInfo::FileInfo(const QString& path, const QDateTime& modified_time, bool is_file, uint64_t file_size) : m_path(path), m_modified_time(modified_time), m_is_file(is_file), m_file_size(file_size), m_is_invalid(false) {}

FileInfo::FileInfo(const QFileInfo& info) : m_path(info.absoluteFilePath()), m_modified_time(info.lastModified()), m_is_file(info.isFile()), m_file_size(info.size()), m_is_invalid(not info.exists()) {}

FileInfo::FileInfo(const FileInfo& info) : m_path(info.m_path), m_modified_time(info.m_modified_time), m_is_file(info.m_is_file), m_file_size(info.m_file_size), m_is_invalid(info.m_is_invalid) {}


FileInfo& FileInfo::operator=(const FileInfo& info) {
	this->m_path = info.m_path;
	this->m_modified_time = info.m_modified_time;
	this->m_is_file = info.m_is_file;
	this->m_file_size = info.m_file_size;
	this->m_is_invalid = info.m_is_invalid;
	return *this;
}

FileInfo& FileInfo::operator=(const QFileInfo& info) {
	this->m_path = info.absoluteFilePath();
	this->m_modified_time = info.lastModified();
	this->m_is_file = info.isFile();
	this->m_file_size = info.size();
	this->m_is_invalid = not info.exists();
	return *this;
}