#pragma once
#include <atomic>
#include <memory>
#include <thread>
#include <type_traits>
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

/* 无锁并行队列，实测比boost的无锁队列快大约3倍
*/
template<class _Value, class _Alloc = std::allocator<_Value>>
class lockfree_queue
{
    SHARELIB_DISABLE_COPY_CLASS(lockfree_queue);

    static_assert(sizeof(size_t) >= 4, "unspport type");

    using value_type = _Value;
    using storage_type =
        std::aligned_storage_t<sizeof(value_type), std::alignment_of<value_type>::value>;

    enum CONSTANT_VALUE : size_t
    {
        //子队列个数
        SUBLIST_COUNT = 8, //2的整倍数

        //每页的数据个数
        ITEMS_PER_PAGE = (sizeof(value_type) <= 4
                              ? 64
                              : sizeof(value_type) <= 8
                                    ? 32
                                    : sizeof(value_type) <= 16
                                          ? 16
                                          : sizeof(value_type) <= 32
                                                ? 8
                                                : sizeof(value_type) <= 64
                                                      ? 4
                                                      : sizeof(value_type) <= 128 ? 2 : 1),

        //子队列内部索引 掩码
        SUBLIST_INDEX_MASK = (size_t)(~(SUBLIST_COUNT - 1)),

        //页内偏移 掩码
        PAGE_OFFSET_MASK = (ITEMS_PER_PAGE - 1) << 3,

        //页索引 掩码
        PAGE_INDEX_MASK = ~(PAGE_OFFSET_MASK | (SUBLIST_COUNT - 1))
    };

    //获取页内偏移地址
#define GET_PAGE_OFFSET(x) ((x & PAGE_OFFSET_MASK) >> 3)

    //页
    struct page
    {
        page()
        {
            for (auto &item : m_flags)
            {
                item = false;
            }
        }
        std::atomic<bool> m_flags[ITEMS_PER_PAGE];
        page *m_pNext{nullptr};
        storage_type m_pData[ITEMS_PER_PAGE];
    };

    //子队列
    struct sublist
    {
        page *m_pHead{nullptr};
        std::atomic<size_t> m_popCount{0};
        page *m_pTail{nullptr};
        std::atomic<size_t> m_pushCount{0};
    };

public:
    explicit lockfree_queue(const _Alloc &alloc = _Alloc{})
        : m_alloc(alloc)
    {
        for (size_t i = 0; i < SUBLIST_COUNT; ++i)
        {
            m_sublists[i].m_pTail = m_sublists[i].m_pHead = alloc_page();
        }
    }

    ~lockfree_queue()
    {
        if (count() > 0)
        {
            value_type tempData{};
            while (try_pop(tempData))
            {
                ;
            }
        }
        for (size_t i = 0; i < SUBLIST_COUNT; ++i)
        {
            page *pCur = m_sublists[i].m_pHead;
            page *pNext = nullptr;
            while (pCur->m_pNext)
            {
                pNext = pCur->m_pNext;
                free_page(pCur);
                pCur = pNext;
            }
            free_page(pCur);
        }
    }

    size_t count() { return m_pushIndex - m_popIndex; }

    template<class... T>
    void push(T &&... data)
    {
        size_t index = m_pushIndex++;
        //选择子队列
        sublist &curList = m_sublists[index & (SUBLIST_COUNT - 1)];
        size_t nPageIndex = index & PAGE_INDEX_MASK;
        size_t nPageOffset = GET_PAGE_OFFSET(index);

        page *pCur = nullptr;
        if (!nPageOffset)
        {
            //页内第一个，预分配下一页
            pCur = alloc_page();
        }

        //如果需要换页，等待前一页处理完
        size_t nSpinCount = 0;
        while ((curList.m_pushCount.load(std::memory_order::memory_order_acquire) &
                PAGE_INDEX_MASK) != nPageIndex)
        {
            yield(nSpinCount++);
        }

        if (!nPageOffset)
        {
            //预分配的页挂到链表上，但尾指针不变
            curList.m_pTail->m_pNext = pCur;
        }
        pCur = curList.m_pTail;

        if (nPageOffset == ITEMS_PER_PAGE - 1)
        {
            //只可能有一个线程执行到该分支，并且该线程是该页上最一个处理者，修改尾指针，换页
            nSpinCount = 0;
            while (curList.m_pushCount.load(std::memory_order::memory_order_acquire) !=
                   (index & SUBLIST_INDEX_MASK))
            {
                yield(nSpinCount++);
            }
            curList.m_pTail = curList.m_pTail->m_pNext;
        }
        curList.m_pushCount.fetch_add(SUBLIST_COUNT, std::memory_order::memory_order_release);

        //存储数据，写入标记
        ::new (pCur->m_pData + nPageOffset) value_type(std::forward<T>(data)...);
        pCur->m_flags[nPageOffset].store(true, std::memory_order::memory_order_release);
    }

    bool try_pop(value_type &data)
    {
        size_t index = m_popIndex.load(std::memory_order::memory_order_acquire);
        do
        {
            if (index == m_pushIndex.load(std::memory_order::memory_order_acquire))
            {
                return false;
            }
        } while (!m_popIndex.compare_exchange_weak(index, index + 1));
        //选择子队列
        sublist &curList = m_sublists[index & (SUBLIST_COUNT - 1)];
        size_t nPageIndex = index & PAGE_INDEX_MASK;
        size_t nPageOffset = GET_PAGE_OFFSET(index);

        //如果需要换页，等待前一页处理完
        size_t nSpinCount = 0;
        while ((curList.m_popCount.load(std::memory_order::memory_order_acquire) &
                PAGE_INDEX_MASK) != nPageIndex)
        {
            yield(nSpinCount++);
        }

        //头指针一定有数据
        page *pCur = curList.m_pHead;
        //等待对应位置上的数据写入完毕
        nSpinCount = 0;
        while (!pCur->m_flags[nPageOffset].load(std::memory_order::memory_order_acquire))
        {
            yield(nSpinCount++);
        }

        data = std::move(*(value_type *)(pCur->m_pData + nPageOffset));
        ((value_type *)(pCur->m_pData + nPageOffset))->~value_type();

        if (nPageOffset == ITEMS_PER_PAGE - 1)
        {
            //最后一个处理者，换页，释放空闲页
            nSpinCount = 0;
            while (curList.m_popCount.load(std::memory_order::memory_order_acquire) !=
                   (index & SUBLIST_INDEX_MASK))
            {
                yield(nSpinCount++);
            }
            curList.m_pHead = curList.m_pHead->m_pNext;
            curList.m_popCount.fetch_add(SUBLIST_COUNT, std::memory_order::memory_order_release);
            free_page(pCur);
        }
        else
        {
            curList.m_popCount.fetch_add(SUBLIST_COUNT, std::memory_order::memory_order_release);
        }
        return true;
    }

#undef GET_PAGE_OFFSET

private:
    page *alloc_page()
    {
        page *pNew = m_alloc.allocate(1);
        m_alloc.construct(pNew);
        return pNew;
    }

    void free_page(page *pNode)
    {
        m_alloc.destroy(pNode);
        m_alloc.deallocate(pNode, 1);
    }

    static void yield(size_t k)
    {
        if (k > 4)
        {
            std::this_thread::yield();
        }
    }

private:
    sublist m_sublists[SUBLIST_COUNT];
    std::atomic<size_t> m_pushIndex{0};
    std::atomic<size_t> m_popIndex{0};

    typename _Alloc::template rebind<page>::other m_alloc;
};

SHARELIB_END_NAMESPACE
