#pragma once
#include <cassert>
#include <memory>
#include <type_traits>
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

/** 注意，该类中的接口是有用意的，故意和pugixml的接口相同，这样traverse_tree_node的算法就两个都可以用了
*/

/** 浸入式树形结构
@param[in] Derived 派生类传给这个模板参数
@param[in] Tags 自定义参数，用于多重继承
*/
template<class Derived, class... Tags>
class tree_node
{
    SHARELIB_DISABLE_COPY_CLASS(tree_node);

public:
    tree_node();
    // 会自动删除它所有的子结点, 并把自己从树中移除
    virtual ~tree_node();

    Derived *root() const;
    bool is_root() const;
    inline size_t child_count() const { return m_uChildCount; }
    inline Derived *parent() const { return m_pParent; }
    inline Derived *previous_sibling() const { return m_pPrevSibling; }
    inline Derived *next_sibling() const { return m_pNextSibling; }
    inline Derived *first_child() const { return m_pFirstChild; }
    inline Derived *last_child() const { return m_pLastChild; }

    /** 获取第N个子结点，从0开始计数，时间复杂度O(n)
    @param[in] nth 子结点索引
    @return 成功返回子结点的指针，下标超出返回nullptr
    */
    Derived *nth_child(size_t nth) const;

    // 插入位置类型
    enum TInsertPos
    {
        AsFirstChild,
        AsLastChild,
        AsPrevSibling,
        AsNextSibling
    };

    /** 插入新结点
    @param[in] pNewNode 新结点
    @param[in] posType 插入位置
    */
    void insert_tree_node(Derived *pNewNode, TInsertPos posType = TInsertPos::AsLastChild);

public:
    //--------------------------------------
    /** \static 下面几个是静态成员，用于删除（移除）结点。
    */

    /** 只从树结构中移出,不删除
    @param [in] bNextSibling 如果为true, 返回移除结点的下一个兄弟结点, 否则返回它的上一个兄弟结点
    */
    static Derived *remove_tree_node(Derived *pNode, bool bNextSibling = true);

    /** 销毁结点及其子树, 使用delete删除结点
    @param[in] pNode 要删除的结点及其子树
    @param[in] bNextSibling 如果为true, 返回移除结点的下一个兄弟结点, 否则返回它的上一个兄弟结点
    */
    static Derived *destroy_tree_node(Derived *pNode, bool bNextSibling = true);

    /** 销毁结点及其子树, 使用指定的删除器删除结点
    @param[in] pNode 要删除的结点及其子树
    @param[in] d 删除器，d(Derived*)
    @param[in] bNextSibling 如果为true, 返回移除结点的下一个兄弟结点, 否则返回它的上一个兄弟结点
    */
    template<class Deletor>
    static Derived *destroy_tree_node(Derived *pNode, Deletor &&d, bool bNextSibling = true);

    /** 销毁所有子结点，自身不会被销毁, 使用delete删除结点
    @param[in] pNode 结点
    */
    static void destroy_children(Derived *pNode);

    /** 销毁所有子结点，自身不会被销毁, 使用指定的删除器删除结点
    @param[in] pNode 结点
    @param[in] d 删除器，d(Derived*)
    */
    template<class Deletor>
    static void destroy_children(Derived *pNode, Deletor &&d);

private:
    //插入结点
    void prepend_child(Derived *pChild);     //对应 AsFirstChild
    void append_child(Derived *pChild);      //对应 AsLastChild
    void prepend_sibling(Derived *pSibling); //对应 AsPrevSibling
    void append_sibling(Derived *pSibling);  //对应 AsNextSibling

    //销毁结点的辅助函数
    template<class Deletor>
    static Derived *destroy_tree_node_helper(Derived *pNode, Deletor &&d);

private:
    size_t m_uChildCount;
    Derived *m_pParent;
    Derived *m_pPrevSibling;
    Derived *m_pNextSibling;
    Derived *m_pFirstChild;
    Derived *m_pLastChild;
};

SHARELIB_END_NAMESPACE

#include "tree_node.inl"
