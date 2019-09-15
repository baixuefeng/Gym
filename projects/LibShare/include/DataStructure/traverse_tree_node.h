#pragma once
#include <type_traits>
#include "MacroDefBase.h"

/*!
 * \file traverse_tree_node.h
 * \brief 这些遍历算法都是非递归算法，好处一，速度快，避免了函数调用的开销；好处二，不会因为遍历深度过大导致线程栈溢出
*/

SHARELIB_BEGIN_NAMESPACE

/*
        A
      /    \
     B      C
    / \    / \
   D   E  F   G
*/

/** 从上到下(top to bottom)顺序(兄弟间)深度优先遍历_TreeNode
    遍历顺序： A-B-D-E-C-F-G
@param [in] rootNode 要遍历的根结点，本身的depth为0,第一级子树为1...
@param [in] func 调用原型：int Func(_TreeNode & node, int nDepth);
                 返回值 > 0,继续遍历; == 0,跳过子树,继续遍历; < 0, 终止遍历;
*/
template<class _TreeNode, class _Callable>
void traverse_tree_node_t2b(_TreeNode &&rootNode, _Callable &&func);

/** 从下到上(bottom to top)顺序(兄弟间)深度优先遍历_TreeNode
    遍历顺序： D-E-B-F-G-C-A
@param [in] rootNode 要遍历的根结点，本身的depth为0,第一级子树为1...
@param [in] func 调用原型：int Func(_TreeNode & node, int nDepth);
                 返回值 < 0, 终止遍历;其它继续遍历
*/
template<class _TreeNode, class _Callable>
void traverse_tree_node_b2t(_TreeNode &&rootNode, _Callable &&func);

/** 从上到下(top to bottom)逆序(兄弟间)深度优先遍历_TreeNode
    遍历顺序： A-C-G-F-B-E-D
@param [in] rootNode 要遍历的根结点，本身的depth为0,第一级子树为1...
@param [in] func 调用原型：int Func(_TreeNode & node, int nDepth);
                 返回值 > 0,继续遍历; == 0,跳过子树,继续遍历; < 0, 终止遍历;
*/
template<class _TreeNode, class _Callable>
void traverse_tree_node_reverse_t2b(_TreeNode &&rootNode, _Callable &&func);

/** 从下到上(bottom to top)逆序(兄弟间)深度优先遍历_TreeNode
    遍历顺序： G-F-C-E-D-B-A
@param [in] rootNode 要遍历的根结点，本身的depth为0,第一级子树为1...
@param [in] func 调用原型：int Func(_TreeNode & node, int nDepth);
                 返回值 < 0, 终止遍历;其它继续遍历
*/
template<class _TreeNode, class _Callable>
void traverse_tree_node_reverse_b2t(_TreeNode &&rootNode, _Callable &&func);

SHARELIB_END_NAMESPACE

#include "traverse_tree_node.inl"
