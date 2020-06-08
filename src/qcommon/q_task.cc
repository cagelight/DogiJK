#include "q_task.hh"
#include "moodycamel/blockingconcurrentqueue.h"

using QueueType = moodycamel::BlockingConcurrentQueue<TaskCore::Task>;

struct TaskCore::PrivateData {
	QueueType queue;
	std::vector<std::thread> workers;
	std::atomic_bool run_sem {true};
};

TaskCore::TaskCore(uint worker_count) : m_worker_count { worker_count }, m_data { new PrivateData } {
	for (uint i = 0; i < worker_count; i++) {
		m_data->workers.emplace_back([this](){
			while (m_data->run_sem) {
				Task task;
				if (m_data->queue.wait_dequeue_timed(task, std::chrono::milliseconds(50))) {
					task();
				}
			}
		});
	}
}

TaskCore::~TaskCore() {
	m_data->run_sem.store(false);
	for (auto & th : m_data->workers) th.join();
}

void TaskCore::enqueue_direct(Task && task) {
	m_data->queue.enqueue(std::forward<Task &&>(task));
}
