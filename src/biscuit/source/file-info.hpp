#ifndef __BISCUIT_SOURCE_FILEINFO_HPP__
#define __BISCUIT_SOURCE_FILEINFO_HPP__

#include <QtCore/QDateTime>
#include <QtCore/QString>

class QFileInfo;

namespace Biscuit {
	namespace Source {
		class FileInfo {
			public:
				FileInfo() = default;
				FileInfo(const QString& path, const QDateTime& modified_time, bool is_file, uint64_t file_size);
				FileInfo(const QFileInfo& info);
				FileInfo(const FileInfo& info);

				inline uint64_t file_size() const {
					return this->m_file_size;
				}
				inline bool is_file() const {
					return this->m_is_file;
				}
				inline bool is_invalid() const {
					return this->m_is_invalid;
				}
				inline const QDateTime& modified_time() const {
					return this->m_modified_time;
				}
				inline const QString& path() const {
					return this->m_path;
				}

				FileInfo& operator =(const FileInfo& info);
				FileInfo& operator =(const QFileInfo& info);

			private:
				QString m_path;
				QDateTime m_modified_time;
				bool m_is_file = false;
				uint64_t m_file_size = 0;
				bool m_is_invalid = true;
		};
	}
}

#endif