#ifndef __BISCUIT_SOURCE_FILEINFO_HPP__
#define __BISCUIT_SOURCE_FILEINFO_HPP__

#include <QtCore/QDateTime>
#include <QtCore/QString>

class QFileInfo;

namespace Biscuit {
	namespace Source {
		class FileInfo {
			public:
				FileInfo();
				FileInfo(const QString& path, const QDateTime& modified_time);
				FileInfo(const QFileInfo& info);
				FileInfo(const FileInfo& info);

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
				bool m_is_invalid;
		};
	}
}

#endif