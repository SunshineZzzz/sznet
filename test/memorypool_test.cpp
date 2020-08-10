// ���Զ��߳��·ǹ̶��ڴ���䣬�����ӿڲ���

#include <vector>
#include <atomic>
#include <iostream>
#include <algorithm>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "../sznet/thread/Thread.h"
// ע�����бȽ�ϵͳmalloc��memory pool������
#include "../sznet/memory/MemoryPool.h"

#if SZ_COMPILER_MSVC
#	pragma comment(lib, "sznet.lib")
#endif

using namespace sznet;

// ����ENABLE_SHOW����ڲ���Ϣ �Ἣ���Ӱ������
#define ENABLE_SHOW
// HARD_MODEģʽ���ӽ���������ͷ��ڴ���龰
#define HARD_MODE
// �Ƿ����������������ڴ�
#define USE_RETAIN_MEMORY

/* -------- �������ݲ��� -------- */
// �ڴ�ع����ÿ���ڴ���С
#define MAX_MEM_SIZE (2 * GB)
// �ڴ�ع����ÿ���ڴ���С
#define MEM_SIZE (uint64_t)(0.3 * GB)
// ��������
#define DATA_N (50000)
// ÿ���������ߴ�
#define DATA_MAX_SIZE (16 * KB)
// �ܲ��Դ���
#define MAX_N (3)
/* -------- �������ݲ��� -------- */

#ifdef _SZNET_MEMORY_MEMORY_POOL_H_
#	define My_Malloc(x) MemoryPoolAlloc(mp, x)
#	define My_Free(x) MemoryPoolFree(mp, x)
#	define My_Malloc_TEST(x) MemoryPoolAlloc(mp, x, false)
#else
#	define KB (unsigned long long) (1 << 10)
#	define MB (unsigned long long) (1 << 20)
#	define GB (unsigned long long) (1 << 30)
#	define My_Malloc(x) malloc(x)
#	define My_Free(x) free(x)
#	define My_Malloc_TEST(x) malloc(x)
#endif

#if (defined _SZNET_MEMORY_MEMORY_POOL_H_)
void SHOW(const char* x, MemoryPool* mp)
{
	std::cout << "============ " << std::this_thread::get_id() << " ============\n";
	mem_size_t mlist_cnt = 0;
	mem_size_t free_cnt = 0;
	mem_size_t alloc_cnt = 0;
	printf("-> %s\n->> Memory Usage: %.4lf\n->> Memory Usage(prog): %.4lf\n", x, MemoryPoolGetUsage(mp), MemoryPoolGetProgUsage(mp));
	get_memory_list_count(mp, &mlist_cnt);
	printf("->> [memorypool_list_count] mlist(%llu)\n", mlist_cnt);
	_MP_Memory* mlist = mp->mlist;
	while (mlist) 
	{
		get_memory_info(mp, mlist, &free_cnt, &alloc_cnt);
		printf("->>> id: %d [list_count] free_list(%llu) alloc_list(%llu)\n", get_memory_id(mlist), free_cnt, alloc_cnt);
		mlist = mlist->next;
	}
	std::cout << "============ " << std::this_thread::get_id() << " ============\n\n" << std::endl;
}
#endif

// �ܹ������ڴ��С
std::atomic_uint64_t total_size = 0;
// ��ʱ����������¼����ڴ��С
uint32_t cur_size = 0;
// ������ӡ�Ļ�����
sz_mutex_t mutex;
// �����ڴ�ڵ�
struct Node 
{
	// �����ڴ��С
	uint32_t size;
	// �ڴ�buffer
	char* data;
	// ʵ��С�����������������
	bool operator<(const Node& n) const 
	{
		return size < n.size;
	}
};

// ���һ������maxnȡģ
unsigned int random_uint(unsigned int maxn) 
{
	// 0 to RAND_MAX(32767)
	auto r = rand();
	unsigned int ret = abs(r) % maxn;
	return ret > 0 ? ret : 32;
}

// �̺߳����������ڴ�
void* test_fn(void* arg) 
{
	Node* mem = new Node[DATA_N];
#ifdef _SZNET_MEMORY_MEMORY_POOL_H_
	MemoryPool* mp = (MemoryPool*)arg;
	mem_size_t cur_total_size = 0;
#else
	unsigned long long cur_total_size = 0;
#endif

#if (defined _SZNET_MEMORY_MEMORY_POOL_H_) && (defined ENABLE_SHOW)
	sz_mutex_lock(&mutex);
	SHOW("Alloc Before1: ", mp);
	sz_mutex_unlock(&mutex);
#endif

	for (int i = 0; i < DATA_N; ++i) 
	{
		cur_size = random_uint(DATA_MAX_SIZE);
		cur_total_size += cur_size;
		mem[i].data = (char*)My_Malloc(cur_size);
		if (mem[i].data == nullptr) 
		{
			printf("Memory overflow!\n");
			exit(0);
		}
		mem[i].size = cur_size;
		*(char*)mem[i].data = '\0';
	}
	// �����һ�������ڴ��ͷ�˳�� ģ��ʵ��������ͷ��ڴ�
	std::sort(mem, mem + DATA_N);

#if (defined _SZNET_MEMORY_MEMORY_POOL_H_) && (defined ENABLE_SHOW)
	sz_mutex_lock(&mutex);
	SHOW("Alloc Before2: ", mp);
	sz_mutex_unlock(&mutex);
#endif

#if (defined _SZNET_MEMORY_MEMORY_POOL_H_) && (defined USE_RETAIN_MEMORY)
	// ʣ���ڴ�����е��ڴ������
	std::vector<char*> vChars;
	std::vector<mem_size_t> vSizes;
	char* pMem = nullptr;
	_MP_Memory* temp1 = mp->mlist;
	while (temp1)
	{
		_MP_Chunk* temp2 = temp1->free_list;
		if (!temp2)
		{
			temp1 = temp1->next;
			continue;
		}
		if (temp2->is_free == 1)
		{
			mem_size_t cur_size = temp2->alloc_mem - sizeof(_mp_chunk) - sizeof(_mp_chunk*);
			pMem = (char*)My_Malloc_TEST(cur_size);
			if (pMem)
			{
				cur_total_size += cur_size;
				vChars.push_back(pMem);
				vSizes.push_back(cur_size);
			}
		}
		else
		{
			assert(0);
		}
		temp1 = temp1->next;
	}
	for (size_t i = 0; i < vChars.size(); ++i)
	{
		My_Free(vChars[i]);
		cur_total_size -= vSizes[i];
	}
#endif

#ifdef HARD_MODE
	// �ͷ�ǰһ�������ڴ棬�����ǰһ���Ѿ��������
	for (int i = 0; i < DATA_N / 2; ++i)
	{
		My_Free(mem[i].data);
		cur_total_size -= mem[i].size;
	}
	// ���·���ǰһ����ڴ�
	for (int i = 0; i < DATA_N / 2; ++i) 
	{
		cur_size = random_uint(DATA_MAX_SIZE);
		cur_total_size += cur_size;
		mem[i].data = (char*)My_Malloc(cur_size);
		if (mem[i].data == nullptr) 
		{
			printf("Memory overflow!\n");
			exit(0);
		}
		mem[i].size = cur_size;
		*(char*)mem[i].data = '\0';
	}
	std::sort(mem, mem + DATA_N);
#endif

#if (defined _SZNET_MEMORY_MEMORY_POOL_H_) && (defined ENABLE_SHOW)
	sz_mutex_lock(&mutex);
	SHOW("Free Before: ", mp);
	sz_mutex_unlock(&mutex);
#endif

	for (int i = 0; i < DATA_N; ++i) 
	{
		My_Free(mem[i].data);
		mem[i].data = nullptr;
	}
	printf("\n");

	sz_mutex_lock(&mutex);
	// ȫ�ַ����ڴ��ۼ�
	total_size += cur_total_size;
#ifdef _SZNET_MEMORY_MEMORY_POOL_H_
#ifdef ENABLE_SHOW
	SHOW("Free After: ", mp);
#endif
	printf("Memory Pool Size: %.4lf MB\n", (double)mp->total_mem_pool_size / 1024 / 1024);
#endif
	printf("Total Usage Size: %.4lf MB\n", (double)total_size / 1024 / 1024);

	sz_mutex_unlock(&mutex);

	delete[] mem;
	return nullptr;
}

int main() 
{
	sztool_memcheck();

	sz_mutex_init(&mutex);

	srand((unsigned)time(nullptr));
	clock_t start, finish;
	double total_time;
	start = clock();

	// ����ϵͳmalloc���ڴ��ʵ��
#ifndef _SZNET_MEMORY_MEMORY_POOL_H_
	printf("System malloc:\n");
#else
	printf("Memory Pool:\n");
	MemoryPool* mp = MemoryPoolInit(MAX_MEM_SIZE, MEM_SIZE);
#endif


	// ��һ��ִ��
#if (defined _SZNET_MEMORY_MEMORY_POOL_H_)
	auto t1 = std::thread(test_fn, mp);
	auto t2 = std::thread(test_fn, mp);
	auto t3 = std::thread(test_fn, mp);
#else
	int null = 0;
	auto t1 = std::thread(test_fn, &null);
	auto t2 = std::thread(test_fn, &null);
	auto t3 = std::thread(test_fn, &null);
#endif
	t1.join();
	t2.join();
	t3.join();

	// �ڶ���ִ��
	printf("\n>\n>\n>\n\n");

	total_size = 0;

#ifdef _SZNET_MEMORY_MEMORY_POOL_H_
	t1 = std::thread(test_fn, mp);
	// t2 = std::thread(test_fn, mp);
	// t3 = std::thread(test_fn, mp);
#else
	t1 = std::thread(test_fn, &null);
	// t2 = std::thread(test_fn, &null);
	// t3 = std::thread(test_fn, &null);
#endif
	t1.join();
	// t2.join();
	// t3.join();

#ifdef _SZNET_MEMORY_MEMORY_POOL_H_
	MemoryPoolDestroy(mp);
#endif

	sz_mutex_fini(&mutex);

	finish = clock();
	total_time = (double)(finish - start) / CLOCKS_PER_SEC;
	printf("\nTotal time: %f seconds.\n", total_time);

	// use mp: Total time: 1.109000 seconds.
	// use sys: Total time: 1.940000 seconds.
	return 0;
}