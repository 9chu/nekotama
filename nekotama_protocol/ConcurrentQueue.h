#pragma once
#include <mutex>
#include <queue>
#include <condition_variable>

#include <StringFormat.h>

namespace nekotama
{
	/// @brief 并发队列
	template<class T>
	class ConcurrentQueue
	{
	private:
		std::queue<T> m_Queue;
		std::mutex m_Mutex;
		std::condition_variable m_CV;
	public:
		bool IsEmpty()const
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			return the_queue.empty();
		}
		void Clear()
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			while (!m_Queue.empty())
				m_Queue.pop();
		}
		void Push(T&& data)
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			m_Queue.push(data);
			lock.unlock();
			m_CV.notify_one();
		}
		void Push(const T& data)
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			m_Queue.push(data);
			lock.unlock();
			m_CV.notify_one();
		}
		bool TryPop(T& data)
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			if (m_Queue.empty())
				return false;
			data = m_Queue.front();
			m_Queue.pop();
			return true;
		}
		void Pop(T& data)
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			while (m_Queue.empty())
			{
				m_CV.wait(lock);
			}
			data = m_Queue.front();
			m_Queue.pop();
		}
		void Peek(T& data)
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			while (m_Queue.empty())
			{
				m_CV.wait(lock);
			}
			data = m_Queue.front();
		}
	};
}
