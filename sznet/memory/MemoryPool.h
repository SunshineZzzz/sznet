#ifndef _SZNET_MEMORY_MEMORYPOOL_H_
#define _SZNET_MEMORY_MEMORYPOOL_H_

#include "../base/NonCopyable.h"
#include "../thread/Thread.h"
#include <stdlib.h>
#include <string.h>

namespace sznet
{

#define mem_size_t uint64_t
#define KB (mem_size_t)(1 << 10)
#define MB (mem_size_t)(1 << 20)
#define GB (mem_size_t)(1 << 30)

#define MP_CHUNKHEADER sizeof(struct _mp_chunk)
#define MP_CHUNKEND sizeof(struct _mp_chunk*)
#define MP_ALIGN_SIZE(_n) ((_n + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1))

// 内存块
// Chunk是向内存池申请任意长度的内存时需要加进去的内存管理信息(头部加chunk，尾部加*chunk)
typedef struct _mp_chunk 
{
	// 所管理内存大小
	mem_size_t alloc_mem;
	// 上一块
	struct _mp_chunk* prev;
	// 下一块
	struct _mp_chunk* next;
	// 1表示内存没有使用，0表示使用中
	int is_free;
} _MP_Chunk;

// 内存
// 申请系统内存进行管理
typedef struct _mp_mem_pool_list 
{
	// 系统内存起始位置
	char* start;
	// 标记ID
	int id;
	// 可以分配的内存大小
	mem_size_t mem_pool_size;
	// 已经分配的内存大小
	mem_size_t alloc_mem;
	// 除去cookie已经分分配内存大小
	mem_size_t alloc_prog_mem;
	// 空闲内存链表
	_MP_Chunk* free_list;
	// 分配内存链表
	_MP_Chunk* alloc_list;
	// 下一个内存对象
	struct _mp_mem_pool_list* next;
} _MP_Memory;

// 内存池
typedef struct _mp_mem_pool 
{
	// 是否在多线程中使用
	bool bThread;
	// 自增ID
	int last_id;
	// 是否可以扩展内存
	int auto_extend;
	// 内存对象一次性最多可以管理的内存大小
	mem_size_t mem_pool_size;
	// 所有内存对象总共可用内存大小
	mem_size_t total_mem_pool_size;
	// 内存池字节数上限
	mem_size_t max_mem_pool_size;
	// 内存对象链表
	struct _mp_mem_pool_list* mlist;
	// 多线程需要用到的互斥量
	sz_mutex_t lock;
} MemoryPool;

// 内部工具函数(调试用)
// 所有Memory的数量
void get_memory_list_count(MemoryPool* mp, mem_size_t* mlist_len);
// 每个Memory的统计信息，空闲内存个数，被使用内存个数
void get_memory_info(MemoryPool* mp, _MP_Memory* mm, mem_size_t* free_list_len, mem_size_t* alloc_list_len);
// 指定内存对象的ID
int get_memory_id(_MP_Memory* mm);

// 内存池API
// 初始化
MemoryPool* MemoryPoolInit(mem_size_t maxmempoolsize, mem_size_t mempoolsize, bool bThread = true);
// 分配
void* MemoryPoolAlloc(MemoryPool* mp, mem_size_t wantsize, bool align = true);
// 释放
int MemoryPoolFree(MemoryPool* mp, void* p);
// 重新初始化内存池
MemoryPool* MemoryPoolClear(MemoryPool* mp);
// 内存池销毁
int MemoryPoolDestroy(MemoryPool* mp);

// 内存池信息API
// 总空间
mem_size_t GetTotalMemory(MemoryPool* mp);
// 实际分配空间
mem_size_t GetUsedMemory(MemoryPool* mp);
// 数据分配空间
mem_size_t GetProgMemory(MemoryPool* mp);
// 实际分配空间占比
float MemoryPoolGetUsage(MemoryPool* mp);
// 数据分配占比
float MemoryPoolGetProgUsage(MemoryPool* mp);

// ///////////////////////////////////////////////////////////////////////

// 通用非固定内存分配器
class CmnMemoryPool : NonCopyable
{
public:
	struct info_t 
	{
		// 是否在多线程下使用
		bool bThread = true;
		// 内存池中内存对象一次性最多可以管理的内存大小
		mem_size_t memPoolSize = 128 * MB;
		// 内存池字节数上限
		mem_size_t maxMemPoolSize = 512 * MB;
	};

public:
	CmnMemoryPool();
	~CmnMemoryPool();

	// 初始化
	int init(info_t* info);
	// 结束
	void fini();
	// 分配内存
	void* allocate(mem_size_t wantsize, bool align = true);
	// 释放内存
	int deallocate(void* p);
	// 没事别瞎用，测试的呢
	MemoryPool* getPtrMP();

public:
	// 总空间
	mem_size_t infoGetTotalMemory();
	// 实际分配空间
	mem_size_t infoGetUsedMemory();
	// 数据分配空间
	mem_size_t infoGetProgMemory();
	// 实际分配空间占比
	float infoMemoryPoolGetUsage();
	// 数据分配占比
	float infoMemoryPoolGetProgUsage();
	// 显示内存池信息
	void infoShow(const char* x);

private:
	// 配置
	info_t m_info;
	// 内存池
	MemoryPool* m_memeryPool;
	// 打印时候用的互斥量
	sz_mutex_t m_mutex;
};

} // end namespace sznet
#endif // _SZNET_MEMORY_MEMORYPOOL_H_