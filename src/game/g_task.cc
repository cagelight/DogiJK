#include "g_local.hh"
#include "moodycamel/blockingconcurrentqueue.h"

using GQueueType = moodycamel::BlockingConcurrentQueue<GTaskType>;
static std::unique_ptr<GQueueType> g_task_queue;

void G_Task_Init() {
	g_task_queue = std::make_unique<GQueueType>();
}

void G_Task_Shutdown() {
	g_task_queue.reset();
}

void G_Task_Run() {
	GTaskType task;
	while (g_task_queue->try_dequeue(task))
		task();
}

void G_Task_Enqueue(GTaskType && task) {
	g_task_queue->enqueue(std::forward<GTaskType &&>(task));
}
