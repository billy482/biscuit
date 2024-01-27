#ifndef __BISCUIT_SOURCE_FILEINFO_HPP__
#define __BISCUIT_SOURCE_FILEINFO_HPP__

#include <QtCore/QDateTime>
#include <QtCore/QJsonDocument>
#include <QtCore/QString>

class QFileInfo;

namespace Biscuit {
	namespace Source {
		enum class FileType {
			File,
			Directory,
			Unknown
		};

		class FileInfo {
			public:
				FileInfo() = default;
				FileInfo(const QString& path, const QDateTime& modified_time, FileType type, uint64_t file_size, const QJsonObject& metadata);
				FileInfo(const QFileInfo& info, const QJsonObject& metadata);
				FileInfo(const QFileInfo& info, QJsonDocument&& metadata);
				FileInfo(const FileInfo& info);

				static FileType from(const QFileInfo& file_info);
				inline uint64_t file_size() const {
					return this->m_file_size;
				}
				inline bool is_dir() const {
					return this->m_type == FileType::Directory;
				}
				inline bool is_file() const {
					return this->m_type == FileType::File;
				}
				inline bool is_invalid() const {
					return this->m_is_invalid;
				}
				inline const QJsonDocument& metadata() const {
					return this->m_metadata;
				}
				inline const QDateTime& modified_time() const {
					return this->m_modified_time;
				}
				inline const QString& path() const {
					return this->m_path;
				}
				inline FileType type() const {
					return this->m_type;
				}

				FileInfo& operator =(const FileInfo& info);

			private:
				QString m_path;
				QDateTime m_modified_time;
				FileType m_type;
				uint64_t m_file_size = 0;
				QJsonDocument m_metadata;
				bool m_is_invalid = true;
		};
	}
}

#endif
