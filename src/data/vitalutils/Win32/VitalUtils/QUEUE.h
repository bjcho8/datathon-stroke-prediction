#pragma once
#include <list>
#include <functional>

using namespace std;
#pragma warning (disable: 4786)
#pragma warning (disable: 4748)
#pragma warning (disable: 4103)

// ��Ƽ ������ safe list
// std::list�� thread safe ���� ����
// ť�� �����͸� �����ϰ� �־��ٸ� ť ������ �ش� �����Ͱ� ����Ű�� ��ü�� �������� �ʴ´�. �˾Ƽ� �����ؾ���`
template <typename T>
class Queue {
private:
	list<T> m_list;
	CCriticalSection m_cs;

public:
	Queue() = default;
	virtual ~Queue() { Clear();}

private:
	Queue(const Queue&) = delete;
	Queue& operator = (const Queue&) = delete;

public:
	void Clear() {
		m_cs.Lock();
		m_list.clear();
		m_cs.Unlock();
	}

	// Enqueue
	void Push(const T& newval) {
		m_cs.Lock();
		m_list.push_back(newval);
		m_cs.Unlock();
	}

	void RemoveIf(std::function<bool (const T&)> tester) {
		for (auto i = m_list.begin(); i != m_list.end(); ) {
			if (tester(*i)) {
				m_cs.Lock();
				i = m_list.erase(i);
				m_cs.Unlock();
			} else {
				++i;
			}
		}
	}

	// Dequeue, pass a pointer by reference
	bool Pop(T& val) {
		m_cs.Lock();
		if (m_list.empty()) {
			m_cs.Unlock();
			return false;
		}
		
		auto it = m_list.begin();
		val = *it;
		m_list.pop_front();
		m_cs.Unlock();

		return true;
	}

	// for accurate sizes change the code to use the Interlocked functions calls
	long GetSize() { return m_list.size(); }
	bool IsEmpty() { return m_list.empty(); }
};
