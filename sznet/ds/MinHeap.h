﻿// Comemnt: 最小堆实现

#ifndef _SZNET_DS_MINHEAP_H_
#define _SZNET_DS_MINHEAP_H_

#include "../NetCmn.h"
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

namespace sznet
{

// 最小堆元素必须要继承
struct MinHeaBaseElem
{
	int32_t min_heap_idx = -1;
};

// 最小堆实现
template<typename T>
class MinHeap
{
public:
	// 比较函数
	using CompareFunc = int(*)(T*, T*);
	// 打印函数
	using GetStrFunc = void(*)(T*, char* buffer, size_t size);

private:
	// 最小堆元素数组 
	T** m_ptrArr;
	// 当前堆大小
	uint32_t m_size;
	// 容量
	uint32_t m_capacity;
	// 元素比较函数
	// 1 - 左边大于右边， 0 - 左边等于右边，-1 - 左边小于右边
	CompareFunc m_pCmpFunc;

	// 从指定下标节点向上调整给e找个位置，从而满足最小堆性质
	// e是待放入元素
	void shiftUp_(uint32_t hole_index, T* e)
	{
		uint32_t parent_index = parent(hole_index);
		// 循环向上给e找个适合的位置
		while ((hole_index > 0) && m_pCmpFunc(m_ptrArr[parent_index], e) == 1)
		{
			m_ptrArr[hole_index] = m_ptrArr[parent_index];
			m_ptrArr[hole_index]->min_heap_idx = hole_index;
			hole_index = parent_index;
			parent_index = parent(hole_index);
		}
		m_ptrArr[hole_index] = e;
		m_ptrArr[hole_index]->min_heap_idx = hole_index;
	}
	// 从指定下标节点向下调整给e找个位置，从而满足最小堆性质
	// e是待放入元素
	void shiftDown_(uint32_t hole_index, T* e)
	{
		// 用于记录值较小的子结点的下标
		uint32_t min_child_index = rightChild(hole_index);
		// min_child_index小于m_size的情况说明有左孩子也有右孩子
		// min_child_index等于m_size的情况说明只有左孩子没有右孩子
		// min_child_index大于m_size的情况说明没有左孩子也没有右孩子
		while (min_child_index <= m_size)
		{
			// 没有右孩子 或者 左孩子小于右孩子
			if (min_child_index == m_size || m_pCmpFunc(m_ptrArr[min_child_index], m_ptrArr[min_child_index - 1]) == 1)
			{
				--min_child_index;
			}
			// 如果待放入的节点e比删除节点的左右孩子中较小的还小，符合性质，结束
			if (m_pCmpFunc(e, m_ptrArr[min_child_index]) == -1)
			{
				break;
			}
			// 继续向下找合适的位置
			m_ptrArr[hole_index] = m_ptrArr[min_child_index];
			m_ptrArr[hole_index]->min_heap_idx = hole_index;
			hole_index = min_child_index;
			min_child_index = rightChild(hole_index);
		}
		// 合适位置
		m_ptrArr[hole_index] = e;
		m_ptrArr[hole_index]->min_heap_idx = hole_index;
	}

public:
	MinHeap(CompareFunc pCmpFunc) :
		m_ptrArr(nullptr),
		m_size(0),
		m_capacity(0),
		m_pCmpFunc(pCmpFunc)
	{
		if (!pCmpFunc)
		{
			assert(0);
		}
	}
	~MinHeap()
	{
		if (m_ptrArr)
		{
			free(m_ptrArr);
			m_ptrArr = nullptr;
		}
	}

	// 返回指定下标的父亲下标
	uint32_t parent(uint32_t n)
	{
		if (n == 0)
		{
			return 0;
		}

		return ((n - 1) / 2);
	}
	// 返回指定下标的左孩子下标
	uint32_t leftChild(uint32_t n)
	{
		return (n * 2 + 1);
	}
	// 返回指定下标的右孩子下标
	uint32_t rightChild(uint32_t n)
	{
		return ((n + 1) * 2);
	}
	// 扩容
	void reserve(uint32_t n)
	{
		if (m_capacity < n)
		{
			T** p = nullptr;
			uint32_t capacity = m_capacity ? m_capacity * 2 : 8;
			if (capacity < n)
			{
				capacity = n;
			}
			p = (T**)realloc(m_ptrArr, capacity * (sizeof(T)));
			if (!p)
			{
				assert(0);
			}
			m_ptrArr = p;
			m_capacity = capacity;
		}
	}
	// 放入元素
	int push(T* e)
	{
		reserve(m_size + 1);
		shiftUp_(m_size++, e);

		return 0;
	}
	// 是否为空
	bool isEmpty()
	{
		return (m_size == 0);
	}
	uint32_t size()
	{
		return m_size;
	}
	// 删除指定的元素
	int erase(T* e)
	{
		if (-1 != e->min_heap_idx)
		{
			// 用最后的元素值替代被删除的结点
			T* last = m_ptrArr[--m_size];
			// 要删除元素的父亲元素下标
			uint32_t parent_index = parent(e->min_heap_idx);
			// 不是根节点 并且 要删除元素小于父结点，需要向上调整
			if (e->min_heap_idx > 0 && m_pCmpFunc(m_ptrArr[parent_index], last) == 1)
			{
				// 从删除元素的下标处向上给last找个合适的位置
				shiftUp_(e->min_heap_idx, last);
			}
			// 最后元素大于父结点，需要向下调整
			else
			{
				// 从删除元素的下标处向下给last找个合适的位置
				shiftDown_(e->min_heap_idx, last);
			}
			e->min_heap_idx = -1;

			return 0;
		}

		return -1;
	}
	// 康康
	T* top()
	{
		return m_size ? m_ptrArr[0] : nullptr;
	}
	// 弹出
	T* pop()
	{
		if (m_size)
		{
			T* e = m_ptrArr[0];
			shiftDown_(0, m_ptrArr[--m_size]);
			e->min_heap_idx = -1;

			return e;
		}

		return nullptr;
	}

public:
	// tool
	// 检测最小堆是否满足性质
	void checkHeap()
	{
		for (uint32_t index = (m_size - 1); index > 0; --index)
		{
			uint32_t curIndex = index;
			while (true)
			{
				// 当前节点下标的爸爸下标
				uint32_t parent_index = parent(curIndex);
				// 判断父亲节点是否比当前节点小
				auto r = m_pCmpFunc(m_ptrArr[parent_index], m_ptrArr[curIndex]);
				if (r == 1)
				{
					// 不要那就错了
					assert(0);
				}
				if (parent_index == 0)
				{
					break;
				}
				--curIndex;
			}
		}
	}
	// 层次遍历
	void levelPrint(GetStrFunc pFunc)
	{
		if (isEmpty() || !pFunc)
		{
			puts("");
			return;
		}

		// 通用BUFF
		char buff[256] = { 0 };
		// 每一行打印多少个，1*2^(n-1)
		uint32_t line = 1;
		// 遍历元素下标
		uint32_t index = 0;
		while (true)
		{
			auto max_line = pow(2, (line - 1));
			while (max_line > 0.0)
			{
				pFunc(m_ptrArr[index], buff, 256);
				printf("<%s> ", buff);
				--max_line;
				++index;
				if (index >= m_size)
				{
					break;
				}
			}
			printf("\n\n");
			if (index >= m_size)
			{
				break;
			}
			++line;
		}
	}
};

} // end namespace sznet

#endif // _SZNET_DS_MINHEAP_H_
