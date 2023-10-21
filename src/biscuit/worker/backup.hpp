#ifndef __BISCUIT_WORKER_BACKUP_HPP__
#define __BISCUIT_WORKER_BACKUP_HPP__

#include <QtCore/QRunnable>

namespace Biscuit {
	namespace Worker {
		class Backup : public QRunnable {
			public:
				Backup();
				virtual ~Backup() = default;

				static int do_backup();
				virtual void run();
		};
	}
}

#endif