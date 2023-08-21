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

namespace Biscuit {
	class Source {
		public:
			Source(const QString& path);

			QFileInfo next();
			void parse(const YAML::Node& node);

		private:
			QMutex m_lock;
			QFileInfo m_root;
			QStack<QFileInfoList> m_paths;
			QList<QRegularExpression> m_include_pattern;
			QList<QRegularExpression> m_exclude_pattern;
			QList<QString> m_exclude_path;
	};
}

#endif