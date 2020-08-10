#include "MemoryPool.h"
#include <thread>
#include <iostream>

namespace sznet
{

// 初始化内存 
static void MP_INIT_MEMORY_STRUCT(_MP_Memory*& mm, mem_size_t mem_sz) 
{
	mm->mem_pool_size = mem_sz;
	mm->alloc_mem = 0;
	mm->alloc_prog_mem = 0;
	mm->free_list = (_MP_Chunk*)mm->start;
	mm->free_list->is_free = 1;
	mm->free_list->alloc_mem = mem_sz;
	mm->free_list->prev = nullptr;
	mm->free_list->next = nullptr;
	mm->alloc_list = nullptr;
}

// 内存块双向链表头插入
static void MP_DLINKLIST_INS_FRT(_MP_Chunk*& head, _MP_Chunk*& x) 
{
	x->prev = nullptr;
	x->next = head;
	if (head)
	{
		head->prev = x;
	}
	head = x;
}

// 内存块双向链删除
static void MP_DLINKLIST_DEL(_MP_Chunk*& head, _MP_Chunk*& x) 
{
	if (!x->prev) 
	{
		head = x->next;
		if (x->next)
		{
			x->next->prev = nullptr;
		}
	}
	else 
	{
		x->prev->next = x->next;
		if (x->next)
		{
			x->next->prev = x->prev;
		}
	}
}

void get_memory_list_count(MemoryPool* mp, mem_size_t* mlist_len) 
{
	if (mp->bThread)
	{
		sz_mutex_lock(&mp->lock);
	}

	mem_size_t mlist_l = 0;
	_MP_Memory* mm = mp->mlist;
	while (mm) 
	{
		mlist_l++;
		mm = mm->next;
	}
	*mlist_len = mlist_l;

	if (mp->bThread)
	{
		sz_mutex_unlock(&mp->lock);
	}
}

void get_memory_info(MemoryPool* mp, _MP_Memory* mm, mem_size_t* free_list_len, mem_size_t* alloc_list_len) 
{
	if (mp->bThread)
	{
		sz_mutex_lock(&mp->lock);
	}

	mem_size_t free_l = 0;
	mem_size_t alloc_l = 0;
	_MP_Chunk* p = mm->free_list;
	while (p) 
	{
		free_l++;
		p = p->next;
	}

	p = mm->alloc_list;
	while (p) 
	{
		alloc_l++;
		p = p->next;
	}
	*free_list_len = free_l;
	*alloc_list_len = alloc_l;

	if (mp->bThread)
	{
		sz_mutex_unlock(&mp->lock);
	}
}

int get_memory_id(_MP_Memory* mm)
{
	return mm->id;
}

// 扩展内存，单链表头插入
static _MP_Memory* extend_memory_list(MemoryPool* mp, mem_size_t new_mem_sz) 
{
	char* s = (char*)malloc(sizeof(_MP_Memory) + new_mem_sz * sizeof(char));
	if (!s)
	{
		return nullptr;
	}

	_MP_Memory* mm = (_MP_Memory*)s;
	mm->start = s + sizeof(_MP_Memory);

	MP_INIT_MEMORY_STRUCT(mm, new_mem_sz);
	mm->id = mp->last_id++;
	mm->next = mp->mlist;
	mp->mlist = mm;

	return mm;
}

// 查找p所在的内存对象
static _MP_Memory* find_memory_list(MemoryPool* mp, void* p) 
{
	_MP_Memory* tmp = mp->mlist;
	while (tmp) 
	{
		if (tmp->start <= (char*)p && tmp->start + mp->mem_pool_size > (char*)p)
		{
			break;
		}
		tmp = tmp->next;
	}

	return tmp;
}

// 合并c前面的空闲内存区块
static int merge_free_chunk(MemoryPool* mp, _MP_Memory* mm, _MP_Chunk* c) 
{
	_MP_Chunk* p0 = c;
	_MP_Chunk* p1 = c;
	// 寻找c最前面没有被使用的空闲内存
	while (p0->is_free) 
	{
		p1 = p0;
		if ((char*)p0 - MP_CHUNKEND - MP_CHUNKHEADER <= mm->start)
		{
			break;
		}
		p0 = *(_MP_Chunk**)((char*)p0 - MP_CHUNKEND);
	}

	// 从p1开始向后合并空闲的内存块，内存地址增加的方向
	p0 = (_MP_Chunk*)((char*)p1 + p1->alloc_mem);
	while ((char*)p0 < mm->start + mp->mem_pool_size && p0->is_free) 
	{
		MP_DLINKLIST_DEL(mm->free_list, p0);
		p1->alloc_mem += p0->alloc_mem;
		p0 = (_MP_Chunk*)((char*)p0 + p0->alloc_mem);
	}

	*(_MP_Chunk**)((char*)p1 + p1->alloc_mem - MP_CHUNKEND) = p1;

	if (mp->bThread)
	{
		sz_mutex_unlock(&mp->lock);
	}

	return 0;
}

MemoryPool* MemoryPoolInit(mem_size_t maxmempoolsize, mem_size_t mempoolsize, bool bThread)
{
	if (mempoolsize > maxmempoolsize) 
	{
		return nullptr;
	}

	MemoryPool* mp = (MemoryPool*)malloc(sizeof(MemoryPool));
	if (!mp)
	{
		return nullptr;
	}

	mp->last_id = 0;
	if (mempoolsize < maxmempoolsize)
	{
		mp->auto_extend = 1;
	}
	mp->max_mem_pool_size = maxmempoolsize;
	mp->total_mem_pool_size = mp->mem_pool_size = mempoolsize;

	mp->bThread = bThread;
	if (mp->bThread)
	{
		sz_mutex_init(&mp->lock);
	}

	char* s = (char*)malloc(sizeof(_MP_Memory) + sizeof(char) * mp->mem_pool_size);
	if (!s)
	{
		return nullptr;
	}

	mp->mlist = (_MP_Memory*)s;
	mp->mlist->start = s + sizeof(_MP_Memory);
	MP_INIT_MEMORY_STRUCT(mp->mlist, mp->mem_pool_size);
	mp->mlist->next = nullptr;
	mp->mlist->id = mp->last_id++;

	return mp;
}

void* MemoryPoolAlloc(MemoryPool* mp, mem_size_t wantsize, bool align) 
{
	if (wantsize <= 0)
	{
		return nullptr;
	}

	mem_size_t total_needed_size = MP_ALIGN_SIZE(wantsize + MP_CHUNKHEADER + MP_CHUNKEND);
	if (!align) 
	{
		// 主要用来测试
		total_needed_size = wantsize + MP_CHUNKHEADER + MP_CHUNKEND;
	}
	if (total_needed_size > mp->mem_pool_size)
	{
		return nullptr;
	}

	_MP_Memory* mm = nullptr;
	_MP_Memory* mm1 = nullptr;
	_MP_Chunk* _free = nullptr;
	_MP_Chunk* _not_free = nullptr;

	if (mp->bThread)
	{
		sz_mutex_lock(&mp->lock);
	}

FIND_FREE_CHUNK:
	mm = mp->mlist;
	while (mm) 
	{
		if (mp->mem_pool_size - mm->alloc_mem < total_needed_size) 
		{
			mm = mm->next;
			continue;
		}

		_free = mm->free_list;
		_not_free = nullptr;

		while (_free) 
		{
			if (_free->alloc_mem >= total_needed_size) 
			{
				// 如果free块分割后剩余内存足够大 则进行分割
				if (_free->alloc_mem - total_needed_size > MP_CHUNKHEADER + MP_CHUNKEND) 
				{
					// 从free块头开始分割出alloc块
					_not_free = _free;

					_free = (_MP_Chunk*)((char*)_not_free + total_needed_size);
					*_free = *_not_free;
					_free->alloc_mem -= total_needed_size;
					*(_MP_Chunk**)((char*)_free + _free->alloc_mem - MP_CHUNKEND) = _free;

					// update free_list
					// 这时候free的信息还是_not_free所以需要更新一哈，把_not_free脱离出来
					if (!_free->prev) 
					{
						mm->free_list = _free;
					}
					else 
					{
						_free->prev->next = _free;
					}
					if (_free->next)
					{
						_free->next->prev = _free;
					}

					_not_free->is_free = 0;
					_not_free->alloc_mem = total_needed_size;

					*(_MP_Chunk**)((char*)_not_free + total_needed_size - MP_CHUNKEND) = _not_free;
				}
				// 不够，则整块分配为alloc
				else 
				{
					_not_free = _free;
					MP_DLINKLIST_DEL(mm->free_list, _not_free);
					_not_free->is_free = 0;
				}

				MP_DLINKLIST_INS_FRT(mm->alloc_list, _not_free);

				mm->alloc_mem += _not_free->alloc_mem;
				mm->alloc_prog_mem += (_not_free->alloc_mem - MP_CHUNKHEADER - MP_CHUNKEND);

				if (mp->bThread)
				{
					sz_mutex_unlock(&mp->lock);
				}

				return (void*)((char*)_not_free + MP_CHUNKHEADER);
			}
			_free = _free->next;
		}
		// 当前内存对象空闲的块，都没有合适大小的，下一个选手吧
		mm = mm->next;
	}

	if (mp->auto_extend) 
	{
		// 超过总内存限制
		if (mp->total_mem_pool_size >= mp->max_mem_pool_size) 
		{
			goto err_out;
		}
		mem_size_t add_mem_sz = mp->max_mem_pool_size - mp->mem_pool_size;
		add_mem_sz = add_mem_sz >= mp->mem_pool_size ? mp->mem_pool_size : add_mem_sz;
		mm1 = extend_memory_list(mp, add_mem_sz);
		if (!mm1) 
		{
			goto err_out;
		}
		mp->total_mem_pool_size += add_mem_sz;

		goto FIND_FREE_CHUNK;
	}

err_out:
	if (mp->bThread)
	{
		sz_mutex_unlock(&mp->lock);
	}

	return nullptr;
}

int MemoryPoolFree(MemoryPool* mp, void* p) 
{
	if (p == nullptr || mp == nullptr)
	{
		return 1;
	}

	if (mp->bThread)
	{
		sz_mutex_lock(&mp->lock);
	}

	_MP_Memory* mm = mp->mlist;
	if (mp->auto_extend)
	{
		mm = find_memory_list(mp, p);
	}

	_MP_Chunk* ck = (_MP_Chunk*)((char*)p - MP_CHUNKHEADER);

	MP_DLINKLIST_DEL(mm->alloc_list, ck);
	MP_DLINKLIST_INS_FRT(mm->free_list, ck);
	ck->is_free = 1;

	mm->alloc_mem -= ck->alloc_mem;
	mm->alloc_prog_mem -= (ck->alloc_mem - MP_CHUNKHEADER - MP_CHUNKEND);

	return merge_free_chunk(mp, mm, ck);
}

MemoryPool* MemoryPoolClear(MemoryPool* mp) 
{
	if (!mp)
	{
		return nullptr;
	}
	
	if (mp->bThread)
	{
		sz_mutex_lock(&mp->lock);
	}

	_MP_Memory* mm = mp->mlist;
	while (mm) 
	{
		MP_INIT_MEMORY_STRUCT(mm, mm->mem_pool_size);
		mm = mm->next;
	}

	if (mp->bThread)
	{
		sz_mutex_unlock(&mp->lock);
	}

	return mp;
}

int MemoryPoolDestroy(MemoryPool* mp) 
{
	if (mp == nullptr)
	{
		return 1;
	}

	if (mp->bThread)
	{
		sz_mutex_lock(&mp->lock);
	}

	_MP_Memory* mm = mp->mlist;
	_MP_Memory* mm1 = nullptr;
	while (mm) 
	{
		mm1 = mm;
		mm = mm->next;
		free(mm1);
	}

	if (mp->bThread)
	{
		sz_mutex_unlock(&mp->lock);
		sz_mutex_fini(&mp->lock);
	}

	free(mp);
	return 0;
}

mem_size_t GetTotalMemory(MemoryPool* mp) 
{
	return mp->total_mem_pool_size;
}

mem_size_t GetUsedMemory(MemoryPool* mp) 
{
	if (mp->bThread)
	{
		sz_mutex_lock(&mp->lock);
	}

	mem_size_t total_alloc = 0;
	_MP_Memory* mm = mp->mlist;
	while (mm) 
	{
		total_alloc += mm->alloc_mem;
		mm = mm->next;
	}

	if (mp->bThread)
	{
		sz_mutex_unlock(&mp->lock);
	}

	return total_alloc;
}

mem_size_t GetProgMemory(MemoryPool* mp) 
{
	if (mp->bThread)
	{
		sz_mutex_lock(&mp->lock);
	}

	mem_size_t total_alloc_prog = 0;
	_MP_Memory* mm = mp->mlist;
	while (mm) 
	{
		total_alloc_prog += mm->alloc_prog_mem;
		mm = mm->next;
	}

	if (mp->bThread)
	{
		sz_mutex_unlock(&mp->lock);
	}

	return total_alloc_prog;
}

float MemoryPoolGetUsage(MemoryPool* mp) 
{
	return (float)GetUsedMemory(mp) / GetTotalMemory(mp);
}

float MemoryPoolGetProgUsage(MemoryPool* mp) 
{
	return (float)GetProgMemory(mp) / GetTotalMemory(mp);
}

// ///////////////////////////////////////////////////////////////////////

CmnMemoryPool::CmnMemoryPool():
	m_memeryPool(nullptr)
{
}

CmnMemoryPool::~CmnMemoryPool()
{
	if (!m_memeryPool)
	{
		return;
	}

	Fini();
}

int CmnMemoryPool::Init(info_t* info)
{
	memcpy(&m_info, info, sizeof(m_info));

	m_memeryPool = MemoryPoolInit(m_info.maxMemPoolSize, m_info.memPoolSize, m_info.bThread);
	if (!m_memeryPool)
	{
		return -1;
	}

	sz_mutex_init(&m_mutex);

	return 0;
}

void CmnMemoryPool::Fini()
{
	MemoryPoolDestroy(m_memeryPool);
	m_memeryPool = nullptr;
	sz_mutex_fini(&m_mutex);
}

void* CmnMemoryPool::Allocate(mem_size_t wantsize, bool align)
{
	if (!m_memeryPool)
	{
		return nullptr;
	}

	return MemoryPoolAlloc(m_memeryPool, wantsize, align);
}

int CmnMemoryPool::Deallocate(void* p)
{
	if (!m_memeryPool)
	{
		return -1;
	}

	return MemoryPoolFree(m_memeryPool, p);
}

MemoryPool* CmnMemoryPool::GetPtrMP()
{
	return m_memeryPool;
}

mem_size_t CmnMemoryPool::InfoGetTotalMemory()
{
	if (!m_memeryPool)
	{
		return 0;
	}

	return GetTotalMemory(m_memeryPool);
}

mem_size_t CmnMemoryPool::InfoGetUsedMemory()
{
	if (!m_memeryPool)
	{
		return 0;
	}

	return GetUsedMemory(m_memeryPool);
}

mem_size_t CmnMemoryPool::InfoGetProgMemory()
{
	if (!m_memeryPool)
	{
		return 0;
	}

	return GetProgMemory(m_memeryPool);
}

float CmnMemoryPool::InfoMemoryPoolGetUsage()
{
	if (!m_memeryPool)
	{
		return 0.0f;
	}

	return MemoryPoolGetUsage(m_memeryPool);
}

float CmnMemoryPool::InfoMemoryPoolGetProgUsage()
{
	if (!m_memeryPool)
	{
		return 0.0f;
	}

	return MemoryPoolGetProgUsage(m_memeryPool);
}

void CmnMemoryPool::InfoShow(const char* x)
{
	if (!m_memeryPool)
	{
		printf("m_memeryPool is null.\n");
		return;
	}

	if (m_info.bThread)
	{
		sz_mutex_lock(&m_mutex);
	}

	std::cout << "============ " << std::this_thread::get_id() << " ============\n";
	mem_size_t mlist_cnt = 0;
	mem_size_t free_cnt = 0;
	mem_size_t alloc_cnt = 0;
	printf("-> %s\n->> Memory Usage: %.4lf\n->> Memory Usage(prog): %.4lf\n", x, MemoryPoolGetUsage(m_memeryPool), MemoryPoolGetProgUsage(m_memeryPool));
	get_memory_list_count(m_memeryPool, &mlist_cnt);
	printf("->> [memorypool_list_count] mlist(%llu)\n", mlist_cnt);
	_MP_Memory* mlist = m_memeryPool->mlist;
	while (mlist)
	{
		get_memory_info(m_memeryPool, mlist, &free_cnt, &alloc_cnt);
		printf("->>> id: %d [list_count] free_list(%llu) alloc_list(%llu)\n", get_memory_id(mlist), free_cnt, alloc_cnt);
		mlist = mlist->next;
	}
	std::cout << "============ " << std::this_thread::get_id() << " ============\n\n" << std::endl;

	if (m_info.bThread)
	{
		sz_mutex_unlock(&m_mutex);
	}
}

#undef MP_LOCK
#undef MP_ALIGN_SIZE

}
