#include <thread>

#include "thread.hpp"

namespace Biscuit {
	class ThreadPrivate {
		public:
			static void main_loop(ThreadPrivate * thread);
			static void start(Thread * thread);

		private:
			ThreadPrivate(Thread * thread);

			Thread * m_thread;
			static ThreadPrivate * ms_first;
			static ThreadPrivate * ms_last;
			ThreadPrivate * m_next = nullptr;

			std::thread m_thread_inner;
			std::mutex m_lock;
			std::condition_variable_any m_wait;
	};
}

using namespace Biscuit;

Thread::Thread() {}


void Thread::start() {
	if (not this->m_is_running and not this->m_is_finished)
		ThreadPrivate::start(this);
}

void Thread::wait() {
	this->m_lock.lock();
	if (this->m_is_running)
		this->m_wait.wait(this->m_lock);
	this->m_lock.unlock();
}



ThreadPrivate * ThreadPrivate::ms_first = nullptr;
ThreadPrivate * ThreadPrivate::ms_last = nullptr;

ThreadPrivate::ThreadPrivate(Thread * thread) : m_thread(thread), m_thread_inner(ThreadPrivate::main_loop, this) {}


void ThreadPrivate::main_loop(ThreadPrivate * self) {
	self->m_lock.lock();

	while (self->m_thread != nullptr) {
		Thread * th = self->m_thread;
		self->m_lock.unlock();

		th->m_lock.lock();
		th->m_thread_private = self;
		th->m_is_running = true;
		th->m_is_finished = false;
		th->m_lock.unlock();

		self->m_thread->run();

		th->m_lock.lock();
		th->m_is_running = false;
		th->m_is_finished = true;
		self->m_thread = nullptr;
		th->m_wait.notify_all();
		th->m_lock.unlock();

		self->m_lock.lock();
		self->m_wait.wait(self->m_lock);
	}

	self->m_lock.unlock();
}

void ThreadPrivate::start(Thread * thread) {
	static std::mutex l;
	l.lock();

	ThreadPrivate * ptr = ThreadPrivate::ms_first;
	while (ptr != nullptr) {
		ptr->m_lock.lock();
		bool is_free = ptr->m_thread == nullptr;
		ptr->m_lock.unlock();

		if (is_free)
			break;
		else
			ptr = ptr->m_next;
	}

	if (ptr == nullptr) {
		ptr = new ThreadPrivate(thread);
		if (ThreadPrivate::ms_first == nullptr)
			ThreadPrivate::ms_first = ThreadPrivate::ms_last;
		else {
			ThreadPrivate::ms_last->m_next = ptr;
			ThreadPrivate::ms_last = ptr;
		}
	} else {
		ptr->m_lock.lock();
		ptr->m_thread = thread;
		ptr->m_wait.notify_one();
		ptr->m_lock.unlock();
	}

	l.unlock();
}