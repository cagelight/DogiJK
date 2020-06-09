#pragma once
#include "q_shared.hh"

#include <future>
#include <functional>

struct TaskCore {
	
	using Task = std::packaged_task<void()>;
	
	static uint default_worker_count() {
		uint hwc = std::thread::hardware_concurrency();
		if (hwc < 2) return 2;
		return hwc;
	}
	
	TaskCore(uint worker_count = default_worker_count());
	~TaskCore();
	
	TaskCore(TaskCore const &) = delete;
	
	inline uint worker_count() const { return m_worker_count; }
	
	void enqueue_direct(Task &&);
	
	template <typename T, typename R = typename std::result_of<T()>::type>
	std::future<R> enqueue(T func) {
		std::promise<R> promise;
		std::future<R> future = promise.get_future();
		Task task = Task { [pr = std::move(promise), func]() mutable {
			if constexpr(std::is_void<R>()) {
				func();
				pr.set_value();
			} else
				pr.set_value(func());
			}};
		this->enqueue_direct(std::move(task));
		return future;
	}
	
	template <typename T, typename R = typename std::result_of<T()>::type>
	R enqueue_wait(T const & func) {
		return enqueue<T, R>(func).get();
	}
	
	template <typename T, typename R = typename std::result_of<T()>::type>
	std::vector<std::future<R>> enqueue_fill(T const & func, uint tasks) {
		std::vector<std::future<R>> futures;
		for (uint i = 0; i < tasks; i++)
			futures.emplace_back(enqueue<T, R>(func));
		return futures;
	}
	
	template <typename T, typename R = typename std::result_of<T()>::type>
	std::vector<std::future<R>> enqueue_fill(T const & func) {
		return enqueue_fill<T, R>(func, m_worker_count);
	}
	
	template <typename T>
	void enqueue_fill_wait(T const & func, uint tasks) {
		for (auto & future : enqueue_fill<T, void>(func, tasks)) future.get();
	}
	
	template <typename T>
	void enqueue_fill_wait(T const & func) {
		enqueue_fill_wait<T>(func, m_worker_count);
	}
	
private:
	uint m_worker_count;
	struct PrivateData;
	std::unique_ptr<PrivateData> m_data;
};
