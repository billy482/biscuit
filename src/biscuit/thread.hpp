#ifndef __BISCUIT_THREAD_HPP__
#define __BISCUIT_THREAD_HPP__

#include <condition_variable>
#include <mutex>

namespace Biscuit {
	class ThreadPrivate;

	class Thread {
		public:
			Thread(const Thread& thread) = delete;
			virtual ~Thread() = default;

			inline bool is_finished() const {
				return this->m_is_finished;
			}
			inline bool is_running() const {
				return this->m_is_running;
			}
			void start();
			void wait();

		protected:
			Thread();

			virtual void run() = 0;

		private:
			volatile bool m_is_finished = false;
			volatile bool m_is_running = false;
			volatile ThreadPrivate * m_thread_private = nullptr;
			std::mutex m_lock;
			std::condition_variable_any m_wait;

			friend class ThreadPrivate;
	};
}

#endif