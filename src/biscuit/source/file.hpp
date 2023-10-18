#ifndef __BISCUIT_SOURCE_FILE_HPP__
#define __BISCUIT_SOURCE_FILE_HPP__

#include <QtCore/QFileInfo>
#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QRegularExpression>
#include <QtCore/QStack>

#include "source.hpp"

namespace Biscuit {
	namespace Source {
		class File : public Source {
			public:
				static File * configure(const QString& path, const YAML::Node& node);
				virtual QIODevice * open(const FileInfo& file) override;
				virtual FileInfo next() override;

			private:
				File(const QString& path);

				QMutex m_lock;
				QFileInfo m_root;
				QStack<QFileInfoList> m_paths;
				QList<QRegularExpression> m_include_pattern;
				QList<QRegularExpression> m_exclude_pattern;
				QList<QString> m_exclude_path;
		};
	}
}

#endif