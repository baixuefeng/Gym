#pragma once

#include <memory>
#include <new>
#include <type_traits>
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

//-------------------------------------

/*!
 * \class IRealAlloc
 * \brief 实现的分配器传给std_allocator_adaptor的第二个模板参数,分配器必须提供如下几个接口
          ATL中的分配器 IAtlMemMgr 就可以满足条件
*/
class IRealAlloc
{
public:
    void *Allocate(size_t nBytes) throw();
    void Free(void *p) throw();
};

//-----------------------------------------
/*!
 * \class std_allocator_adaptor
 * \brief std_allocator_adaptor是符合C++标准的内存分配器的适配接口,
          _ValueType:要分配的实际数据类型,
          _RealAllocType:需要提供实际的内存分配器类型
 */

struct init_alloc_t
{};

template<class _ValueType, class _RealAllocType>
class std_allocator_adaptor
{
public:
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using value_type = std::remove_const_t<_ValueType>;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using reference = value_type &;
    using const_reference = const value_type &;

    using AllocType = _RealAllocType;

    template<class _Other>
    struct rebind
    { // convert this type to allocator<_Other>
        typedef std_allocator_adaptor<_Other, AllocType> other;
    };

    std_allocator_adaptor()
        : m_spAlloc(std::make_shared<AllocType>())
    {}

    template<class... _TParam>
    std_allocator_adaptor(init_alloc_t, _TParam &&... initParam)
        : m_spAlloc(std::make_shared<AllocType>(std::forward<_TParam>(initParam)...))
    {}

    template<typename _Tp1>
    std_allocator_adaptor(const std_allocator_adaptor<_Tp1, AllocType> &alloc2)
    {
        m_spAlloc = std::atomic_load(&alloc2.m_spAlloc);
    }

    template<typename _Tp1>
    std_allocator_adaptor(std_allocator_adaptor<_Tp1, AllocType> &&alloc2)
    {
        std::atomic_exchange(&m_spAlloc, alloc2.m_spAlloc);
    }

    template<typename _Tp1>
    std_allocator_adaptor &operator=(const std_allocator_adaptor<_Tp1, AllocType> &alloc2)
    {
        if (this != &alloc2)
        {
            m_spAlloc = std::atomic_load(&alloc2.m_spAlloc);
        }
        return *this;
    }

    template<typename _Tp1>
    std_allocator_adaptor &operator=(std_allocator_adaptor<_Tp1, AllocType> &&alloc2)
    {
        if (this != &alloc2)
        {
            std::atomic_exchange(&m_spAlloc, alloc2.m_spAlloc);
        }
        return *this;
    }

    //-------------------------------------------------------------

    size_type max_size() const { return ((size_type)(-1) / sizeof(value_type)); }

    const_pointer address(const_reference tValue) const { return &tValue; }

    pointer allocate(size_type nCount)
    {
        auto p = static_cast<pointer>(m_spAlloc->Allocate(nCount * sizeof(value_type)));
        if (!p)
        {
            throw std::bad_alloc();
        }
        return p;
    }

    pointer allocate(size_type nCount, const void *) { return allocate(nCount); }

    template<class... _Types>
    void construct(pointer pAddress, _Types &&... args)
    {
        ::new (pAddress) value_type(std::forward<_Types>(args)...);
    }

    void destroy(pointer pAddress)
    {
        (void)pAddress;
        pAddress->~_ValueType();
    }

    void deallocate(pointer pAddress, size_type /*uCount*/) { m_spAlloc->Free(pAddress); }

private:
    template<class _ValueType, class AllocType>
    friend class std_allocator_adaptor;

    std::shared_ptr<AllocType> m_spAlloc;
};

//不支持void
template<class T>
class std_allocator_adaptor<void, T>;
template<class T>
class std_allocator_adaptor<const void, T>;
template<class T>
class std_allocator_adaptor<volatile void, T>;
template<class T>
class std_allocator_adaptor<const volatile void, T>;

template<class _Ty, class _Other, class _RealAlloc>
inline bool operator==(const std_allocator_adaptor<_Ty, _RealAlloc> &,
                       const std_allocator_adaptor<_Other, _RealAlloc> &)
{
    return (true);
}

template<class _Ty, class _Other, class _RealAlloc>
inline bool operator!=(const std_allocator_adaptor<_Ty, _RealAlloc> &_Left,
                       const std_allocator_adaptor<_Other, _RealAlloc> &_Right)
{
    return (!(_Left == _Right));
}

/* C++11中的内存分配器只需要满足以下几个条件即可：
1. 定义value_type类型别名；
2. allocate 接口： pointer allocate (std::size_t)
3. deallocate 接口： void deallocate (T* p, std::size_t n)
4. 不同value_type之间的拷贝构造；template <class U> custom_allocator (const custom_allocator<U>&)
5. 重载 ==，!= 两个运算符；
剩下就可以 allocator_traits 补充起来，之后再经过stl的内部类 _Wrap_alloc 封装，就成为了
一个完整的内存分配器。
相比C++98已经很简单了，但还是有些问题，
1. 条件还是比较多；
2. _Wrap_alloc 是个内部实现，通常的容器都是直接使用标准的 allocator的接口，如果它没有经过 _Wrap_alloc 和
allocator_traits 的双重包装就无法兼容；
*/

//stl中 _Wrap_alloc 的复制, 当自己要实现容器时, 这个内部实现是非常有用的.
template<class _Alloc>
struct alloc_wrapper : public _Alloc
{ // defines traits for allocators
    typedef alloc_wrapper<_Alloc> other;

    typedef _Alloc _Mybase;
    typedef std::allocator_traits<_Alloc> _Mytraits;

    typedef typename _Mytraits::value_type value_type;

    typedef typename _Mytraits::pointer pointer;
    typedef typename _Mytraits::const_pointer const_pointer;
    typedef typename _Mytraits::void_pointer void_pointer;
    typedef typename _Mytraits::const_void_pointer const_void_pointer;

    typedef typename std::conditional<std::is_void<value_type>::value, int, value_type>::type
        &reference;
    typedef typename std::conditional<std::is_void<const value_type>::value,
                                      const int,
                                      const value_type>::type &const_reference;

    typedef typename _Mytraits::size_type size_type;
    typedef typename _Mytraits::difference_type difference_type;

    typedef typename _Mytraits::propagate_on_container_copy_assignment
        propagate_on_container_copy_assignment;
    typedef typename _Mytraits::propagate_on_container_move_assignment
        propagate_on_container_move_assignment;
    typedef typename _Mytraits::propagate_on_container_swap propagate_on_container_swap;

    alloc_wrapper select_on_container_copy_construction() const
    { // get allocator to use
        return (_Mytraits::select_on_container_copy_construction(*this));
    }

    template<class _Other>
    struct rebind
    { // convert this type to allocator<_Other>
        typedef typename _Mytraits::template rebind_alloc<_Other> _Other_alloc;
        typedef alloc_wrapper<_Other_alloc> other;
    };

    pointer address(reference _Val) const
    { // return address of mutable _Val
        return (_STD addressof(_Val));
    }

    const_pointer address(const_reference _Val) const
    { // return address of nonmutable _Val
        return (_STD addressof(_Val));
    }

    alloc_wrapper() throw()
        : _Mybase()
    { // construct default allocator (do nothing)
    }

    alloc_wrapper(const _Mybase &_Right) throw()
        : _Mybase(_Right)
    { // construct by copying base
    }

    alloc_wrapper(const alloc_wrapper &_Right) throw()
        : _Mybase(_Right)
    { // construct by copying
    }

    template<class _Other>
    alloc_wrapper(const _Other &_Right) throw()
        : _Mybase(_Right)
    { // construct from a related allocator
    }

    template<class _Other>
    alloc_wrapper(_Other &_Right) throw()
        : _Mybase(_Right)
    { // construct from a related allocator
    }

    alloc_wrapper &operator=(const _Mybase &_Right)
    { // construct by copying base
        _Mybase::operator=(_Right);
        return (*this);
    }

    alloc_wrapper &operator=(const alloc_wrapper &_Right)
    { // construct by copying
        _Mybase::operator=(_Right);
        return (*this);
    }

    template<class _Other>
    alloc_wrapper &operator=(const _Other &_Right)
    { // assign from a related allocator
        _Mybase::operator=(_Right);
        return (*this);
    }

    pointer allocate(size_type _Count)
    { // allocate array of _Count elements
        return (_Mybase::allocate(_Count));
    }

    pointer allocate(size_type _Count, const_void_pointer _Hint)
    { // allocate array of _Count elements, with hint
        return (_Mytraits::allocate(*this, _Count, _Hint));
    }

    void deallocate(pointer _Ptr, size_type _Count)
    { // deallocate object at _Ptr, ignore size
        _Mybase::deallocate(_Ptr, _Count);
    }

    void construct(value_type *_Ptr)
    { // default construct object at _Ptr
        _Mytraits::construct(*this, _Ptr);
    }

    template<class _Ty, class... _Types>
    void construct(_Ty *_Ptr, _Types &&... _Args)
    { // construct _Ty(_Types...) at _Ptr
        _Mytraits::construct(*this, _Ptr, _STD forward<_Types>(_Args)...);
    }

    template<class _Ty>
    void destroy(_Ty *_Ptr)
    { // destroy object at _Ptr
        _Mytraits::destroy(*this, _Ptr);
    }

    size_type max_size() const throw()
    { // get maximum size
        return (_Mytraits::max_size(*this));
    }
};

template<class _Ty, class _Other>
inline bool operator==(const alloc_wrapper<_Ty> &_Left, const alloc_wrapper<_Other> &_Right) throw()
{ // test for allocator equality
    return (static_cast<_Ty>(_Left) == static_cast<_Other>(_Right));
}

template<class _Ty, class _Other>
inline bool operator!=(const alloc_wrapper<_Ty> &_Left, const alloc_wrapper<_Other> &_Right) throw()
{ // test for allocator inequality
    return (!(_Left == _Right));
}

SHARELIB_END_NAMESPACE
