#ifndef __BISCUIT_SOURCE_HPP__
#define __BISCUIT_SOURCE_HPP__

#include <QtCore/QFileInfo>
#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QStack>
#include <QtCore/QString>
#include <QtCore/QRegularExpression>

namespace YAML {
	class Node;
}

class QIODevice;

namespace Biscuit {
	class Source {
		public:
			Source(const QString& path);
			Source(const Source& source);
			Source(Source&& source);

			static QList<Source>& get();
			QFileInfo next();
			QIODevice * open(const QFileInfo& file);
			void parse(const YAML::Node& node);
			static void parse(const QString& path, const YAML::Node& node);

			Source& operator=(const Source& source);
			Source& operator=(Source&& source);

		private:
			QMutex m_lock;
			QFileInfo m_root;
			QStack<QFileInfoList> m_paths;
			QList<QRegularExpression> m_include_pattern;
			QList<QRegularExpression> m_exclude_pattern;
			QList<QString> m_exclude_path;
			static QList<Source> ms_sources;
	};
}

#endif