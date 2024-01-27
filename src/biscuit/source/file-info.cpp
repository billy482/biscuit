#include <QtCore/QFileInfo>

#include "file-info.hpp"

using namespace Biscuit::Source;

FileInfo::FileInfo(const QString& path, const QDateTime& modified_time, FileType type, uint64_t file_size, const QJsonObject& metadata) : m_path(path), m_modified_time(modified_time), m_type(type), m_file_size(file_size), m_metadata(metadata), m_is_invalid(false) {}

FileInfo::FileInfo(const QFileInfo& info, const QJsonObject& metadata) : m_path(info.absoluteFilePath()), m_modified_time(info.lastModified()), m_type(FileInfo::from(info)), m_file_size(info.size()), m_metadata(metadata), m_is_invalid(not info.exists()) {}

FileInfo::FileInfo(const QFileInfo& info, QJsonDocument&& metadata) : m_path(info.absoluteFilePath()), m_modified_time(info.lastModified()), m_type(FileInfo::from(info)), m_file_size(info.size()), m_metadata(std::move(metadata)), m_is_invalid(not info.exists()) {}

FileInfo::FileInfo(const FileInfo& info) : m_path(info.m_path), m_modified_time(info.m_modified_time), m_type(info.m_type), m_file_size(info.m_file_size), m_metadata(info.m_metadata), m_is_invalid(info.m_is_invalid) {}


FileType FileInfo::from(const QFileInfo& file_info) {
	if (file_info.isFile())
		return FileType::File;
	else if (file_info.isDir())
		return FileType::Directory;
	else
		return FileType::Unknown;
}


FileInfo& FileInfo::operator=(const FileInfo& info) {
	this->m_path = info.m_path;
	this->m_modified_time = info.m_modified_time;
	this->m_type = info.m_type;
	this->m_file_size = info.m_file_size;
	this->m_metadata = info.m_metadata;
	this->m_is_invalid = info.m_is_invalid;
	return *this;
}
