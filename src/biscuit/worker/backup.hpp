#ifndef __BISCUIT_WORKER_BACKUP_HPP__
#define __BISCUIT_WORKER_BACKUP_HPP__

#include <QtCore/QRunnable>

namespace YAML {
	class Node;
}

namespace Biscuit {
	namespace Worker {
		class Backup : public QRunnable {
			public:
				Backup();
				virtual ~Backup() = default;

				static int do_backup(const YAML::Node& config);
				virtual void run();
		};
	}
}

#endif