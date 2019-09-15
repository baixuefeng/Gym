
SHARELIB_BEGIN_NAMESPACE

template<class Derived, class... Tags>
tree_node<Derived, Tags...>::tree_node()
{
    m_uChildCount = 0;
    m_pParent = m_pPrevSibling = m_pNextSibling = m_pFirstChild = m_pLastChild = nullptr;
}

template<class Derived, class... Tags>
tree_node<Derived, Tags...>::~tree_node()
{
    destroy_children((Derived *)this);
    //最后才从树中移除
    remove_tree_node((Derived *)this);
}

template<class Derived, class... Tags>
Derived *tree_node<Derived, Tags...>::root() const
{
    if (!m_pParent)
    {
        return (Derived *)this;
    }
    Derived *pRoot = m_pParent;
    while (pRoot->m_pParent)
    {
        pRoot = pRoot->m_pParent;
    }
    return pRoot;
}

template<class Derived, class... Tags>
bool tree_node<Derived, Tags...>::is_root() const
{
    return (m_pParent == nullptr);
}

template<class Derived, class... Tags>
Derived *tree_node<Derived, Tags...>::nth_child(size_t nth) const
{
    Derived *pChild = m_pFirstChild;
    while (pChild && nth-- > 0)
    {
        pChild = pChild->m_pNextSibling;
    }
    return pChild;
}

template<class Derived, class... Tags>
void tree_node<Derived, Tags...>::insert_tree_node(
    Derived *pNewNode,
    TInsertPos posType /* = TInsertPos::AsLastChild*/)
{
    assert(pNewNode);
    assert(pNewNode != this);
    switch (posType)
    {
    case Derived::AsFirstChild:
        prepend_child(pNewNode);
        break;
    case Derived::AsLastChild:
        append_child(pNewNode);
        break;
    case Derived::AsPrevSibling:
        prepend_sibling(pNewNode);
        break;
    case Derived::AsNextSibling:
        append_sibling(pNewNode);
        break;
    default:
        assert(!"参数错误");
        return;
    }
}

template<class Derived, class... Tags>
Derived *tree_node<Derived, Tags...>::remove_tree_node(Derived *pNode,
                                                       bool bNextSibling /* = true*/)
{
    if (!pNode)
    {
        return nullptr;
    }
    if (pNode->m_pParent)
    {
        if (pNode->m_pParent->m_pFirstChild == pNode)
        {
            pNode->m_pParent->m_pFirstChild = pNode->m_pNextSibling;
        }
        if (pNode->m_pParent->m_pLastChild == pNode)
        {
            pNode->m_pParent->m_pLastChild = pNode->m_pPrevSibling;
        }
        assert(pNode->m_pParent->m_uChildCount);
        --pNode->m_pParent->m_uChildCount;
    }
    pNode->m_pParent = nullptr;

    //记录要返回的结点
    Derived *pReturn = nullptr;
    if (bNextSibling)
    {
        pReturn = pNode->m_pNextSibling;
    }
    else
    {
        pReturn = pNode->m_pPrevSibling;
    }

    if (pNode->m_pPrevSibling)
    {
        pNode->m_pPrevSibling->m_pNextSibling = pNode->m_pNextSibling;
    }
    if (pNode->m_pNextSibling)
    {
        pNode->m_pNextSibling->m_pPrevSibling = pNode->m_pPrevSibling;
    }
    pNode->m_pPrevSibling = nullptr;
    pNode->m_pNextSibling = nullptr;

    return pReturn;
}

template<class Derived, class... Tags>
Derived *tree_node<Derived, Tags...>::destroy_tree_node(Derived *pNode,
                                                        bool bNextSibling /* = true*/)
{
    return destroy_tree_node(pNode, std::default_delete<Derived>(), bNextSibling);
}

template<class Derived, class... Tags>
template<class Deletor>
Derived *tree_node<Derived, Tags...>::destroy_tree_node(Derived *pNode,
                                                        Deletor &&d,
                                                        bool bNextSibling /* = true*/)
{
    if (!pNode)
    {
        return nullptr;
    }
    destroy_children(pNode, std::forward<Deletor>(d));

    //记录要返回的结点，注意确保结点最后才从树中移除
    Derived *pReturn = nullptr;
    if (bNextSibling)
    {
        pReturn = pNode->m_pNextSibling;
    }
    else
    {
        pReturn = pNode->m_pPrevSibling;
    }
    d(pNode);
    return pReturn;
}

template<class Derived, class... Tags>
void tree_node<Derived, Tags...>::destroy_children(Derived *pNode)
{
    if (!pNode || pNode->child_count() == 0)
    {
        return;
    }
    destroy_children(pNode, std::default_delete<Derived>());
}

template<class Derived, class... Tags>
template<class Deletor>
void tree_node<Derived, Tags...>::destroy_children(Derived *pNode, Deletor &&d)
{
    if (!pNode || pNode->child_count() == 0)
    {
        return;
    }
    for (Derived *pChild = pNode->first_child(); pChild;)
    {
        pChild = destroy_tree_node_helper(pChild, std::forward<Deletor>(d));
    }
}

template<class Derived, class... Tags>
void tree_node<Derived, Tags...>::prepend_child(Derived *pChild)
{
    assert(pChild);
    if (!pChild)
    {
        return;
    }

    //兄
    pChild->m_pNextSibling = m_pFirstChild;
    if (m_pFirstChild)
    {
        m_pFirstChild->m_pPrevSibling = pChild;
        m_pFirstChild = pChild;
    }
    else
    {
        m_pFirstChild = m_pLastChild = pChild;
    }

    //父
    pChild->m_pParent = (Derived *)this;
    ++m_uChildCount;
}

template<class Derived, class... Tags>
void tree_node<Derived, Tags...>::append_child(Derived *pChild)
{
    assert(pChild);
    if (!pChild)
    {
        return;
    }

    //兄
    pChild->m_pPrevSibling = m_pLastChild;
    if (m_pLastChild)
    {
        m_pLastChild->m_pNextSibling = pChild;
        m_pLastChild = pChild;
    }
    else
    {
        m_pFirstChild = m_pLastChild = pChild;
    }

    //父
    pChild->m_pParent = (Derived *)this;
    ++m_uChildCount;
}

template<class Derived, class... Tags>
void tree_node<Derived, Tags...>::prepend_sibling(Derived *pSibling)
{
    assert(pSibling);
    if (!pSibling)
    {
        return;
    }
    //父
    pSibling->m_pParent = m_pParent;
    if (m_pParent)
    {
        ++m_pParent->m_uChildCount;
        if (!m_pPrevSibling)
        {
            m_pParent->m_pFirstChild = pSibling;
        }
    }

    //兄
    pSibling->m_pPrevSibling = m_pPrevSibling;
    pSibling->m_pNextSibling = (Derived *)this;
    if (m_pPrevSibling)
    {
        m_pPrevSibling->m_pNextSibling = pSibling;
    }
    m_pPrevSibling = pSibling;
}

template<class Derived, class... Tags>
void tree_node<Derived, Tags...>::append_sibling(Derived *pSibling)
{
    assert(pSibling);
    if (!pSibling)
    {
        return;
    }
    //父
    pSibling->m_pParent = m_pParent;
    if (m_pParent)
    {
        ++m_pParent->m_uChildCount;
        if (!m_pNextSibling)
        {
            m_pParent->m_pLastChild = pSibling;
        }
    }

    //兄
    pSibling->m_pPrevSibling = (Derived *)this;
    pSibling->m_pNextSibling = m_pNextSibling;
    if (m_pNextSibling)
    {
        m_pNextSibling->m_pPrevSibling = pSibling;
    }
    m_pNextSibling = pSibling;
}

template<class Derived, class... Tags>
template<class Deletor>
Derived *tree_node<Derived, Tags...>::destroy_tree_node_helper(Derived *pNode, Deletor &&d)
{
    if (!pNode)
    {
        return nullptr;
    }
    for (Derived *pChild = pNode->first_child(); pChild;)
    {
        pChild = destroy_tree_node_helper(pChild, std::forward<Deletor>(d));
    }

    //记录要返回的结点，注意确保结点最后才从树中移除
    Derived *pReturn = pNode->m_pNextSibling;
    d(pNode);
    return pReturn;
}

SHARELIB_END_NAMESPACE
