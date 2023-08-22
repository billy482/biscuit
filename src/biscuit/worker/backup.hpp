#ifndef __BISCUIT_SOURCE_HPP__
#define __BISCUIT_SOURCE_HPP__

#include <QtCore/QRunnable>

namespace Biscuit {
	namespace Worker {
		class Backup : public QRunnable {
			public:
				Backup();
				virtual ~Backup() = default;

				virtual void run();
		};
	}
}

#endif