#ifndef __BISCUIT_WORKER_BACKUP_HPP__
#define __BISCUIT_WORKER_BACKUP_HPP__

#include <QtCore/QRunnable>

namespace YAML {
	class Node;
}

namespace Biscuit {
	namespace Db {
		class BackupId;
	}

	namespace Worker {
		class Backup : public QRunnable {
			public:
				Backup(const Db::BackupId& backup_id);
				virtual ~Backup() = default;

				static int do_backup(const YAML::Node& config);
				virtual void run();

			private:
				const Db::BackupId& m_backup_id;
		};
	}
}

#endif
