#include "../sznet/ds/MinHeap.h"
#include <stdio.h>

#if SZ_COMPILER_MSVC
#	pragma comment(lib, "sznet.lib")
#endif

using namespace sznet;

// 最小堆元素对象
struct mytimeval
{
	long tv_sec;
	long tv_usec;
};
struct event : public MinHeapbaseElem
{
	mytimeval ev_timeout;
};

// 随机event
void set_random_timeout(event* ev)
{
	ev->ev_timeout.tv_sec = rand();
	ev->ev_timeout.tv_usec = rand();
}

// 比较函数
int event_greater(event* l, event* r)
{
	if (l->ev_timeout.tv_sec == r->ev_timeout.tv_sec)
	{
		if (l->ev_timeout.tv_usec == r->ev_timeout.tv_usec)
		{
			return 0;
		}
		else if (l->ev_timeout.tv_usec > r->ev_timeout.tv_usec)
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}
	if (l->ev_timeout.tv_sec > r->ev_timeout.tv_sec)
	{
		return 1;
	}
	else
	{
		return -1;
	}
}

// 辅助打打印函数
void event_getstr(event* e, char* buffer, size_t size)
{
	snprintf(buffer, size, "[%u(index):%d(sec):%d(usec)]", e->min_heap_idx, e->ev_timeout.tv_sec, e->ev_timeout.tv_usec);
}

int main()
{
	event* inserted[1024];
	MinHeap<event> MinHeap(&event_greater);

	for (int i = 0; i < 1024; ++i)
	{
		inserted[i] = (event*)malloc(sizeof(event));
		set_random_timeout(inserted[i]);
		MinHeap.Push(inserted[i]);
	}

	MinHeap.CheckHeap();
	assert(MinHeap.Size() == 1024);
	// MinHeap.LevelPrint(event_getstr);

	int sum = 512;
	int rst = 0;
	while (sum > 0)
	{
		uint32_t index = rand() % 1024;
		if (index < MinHeap.Size() && inserted[index]->min_heap_idx != -1)
		{
			rst = MinHeap.Erase(inserted[index]);
			assert(rst == 0);
			--sum;
		}
	}
	MinHeap.CheckHeap();
	assert(MinHeap.Size() == 512);

	sum = 500;
	event* e = nullptr;
	while (sum > 0)
	{
		e = MinHeap.Pop();
		assert(e != nullptr);
		--sum;
	}

	MinHeap.CheckHeap();
	assert(MinHeap.Size() == 12);
	MinHeap.LevelPrint(event_getstr);

	for (int i = 0; i < 1024; ++i)
	{
		free(inserted[i]);
	}

	return 0;
}