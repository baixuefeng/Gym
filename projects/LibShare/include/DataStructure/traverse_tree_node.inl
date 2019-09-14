#include "TemplateMeta/MetaUtility.h"
#include <utility>

SHARELIB_BEGIN_NAMESPACE

template<class _TreeNode, class _Callable>
void traverse_tree_node_t2b(_TreeNode && rootNode, _Callable && func)
{
    int nDepth = 0;
    if (func(rootNode, nDepth) <= 0)
    {
        return;
    }

    auto cur = to_reference(rootNode).first_child();

    if (cur)
    {
        ++nDepth;

        int res = 0;
        do
        {
            res = func(cur, nDepth);
            if (res < 0)
            {
                return;
            }
            else if (res > 0
                && to_reference(cur).first_child())
            {
                ++nDepth;
                cur = to_reference(cur).first_child();
            }
            else if (to_reference(cur).next_sibling())
            {
                cur = to_reference(cur).next_sibling();
            }
            else
            {
                while (!to_reference(cur).next_sibling()
                    && cur != rootNode
                    && to_reference(cur).parent())
                {
                    --nDepth;
                    cur = to_reference(cur).parent();
                }

                if (cur != rootNode)
                {
                    cur = to_reference(cur).next_sibling();
                }
            }
        } while (cur && cur != rootNode);
    }
}

template<class _TreeNode, class _Callable>
void traverse_tree_node_b2t(_TreeNode && rootNode, _Callable && func)
{
    int nDepth = 0;

    auto cur = to_reference(rootNode).first_child();

    if (cur)
    {
        ++nDepth;

        do
        {
            if (to_reference(cur).first_child())
            {
                ++nDepth;
                cur = to_reference(cur).first_child();
            }
            else
            {
                if (func(cur, nDepth) < 0)
                {
                    return;
                }

                if (to_reference(cur).next_sibling())
                {
                    cur = to_reference(cur).next_sibling();
                }
                else
                {
                    while (!to_reference(cur).next_sibling()
                        && cur != rootNode
                        && to_reference(cur).parent())
                    {
                        --nDepth;
                        cur = to_reference(cur).parent();

                        if (cur != rootNode)
                        {
                            if (func(cur, nDepth) < 0)
                            {
                                return;
                            }
                        }
                    }

                    if (cur != rootNode)
                    {
                        cur = to_reference(cur).next_sibling();
                    }
                }
            }
        } while (cur && cur != rootNode);
    }

    func(rootNode, 0);
}

template<class _TreeNode, class _Callable>
void traverse_tree_node_reverse_t2b(_TreeNode && rootNode, _Callable && func)
{
    int nDepth = 0;
    if (func(rootNode, nDepth) <= 0)
    {
        return;
    }

    auto cur = to_reference(rootNode).last_child();

    if (cur)
    {
        ++nDepth;

        int res = 0;
        do
        {
            res = func(cur, nDepth);
            if (res < 0)
            {
                return;
            }
            else if (res > 0
                && to_reference(cur).last_child())
            {
                ++nDepth;
                cur = to_reference(cur).last_child();
            }
            else if (to_reference(cur).previous_sibling())
            {
                cur = to_reference(cur).previous_sibling();
            }
            else
            {
                while (!to_reference(cur).previous_sibling()
                    && cur != rootNode
                    && to_reference(cur).parent())
                {
                    --nDepth;
                    cur = to_reference(cur).parent();
                }

                if (cur != rootNode)
                {
                    cur = to_reference(cur).previous_sibling();
                }
            }
        } while (cur && cur != rootNode);
    }
}

template<class _TreeNode, class _Callable>
void traverse_tree_node_reverse_b2t(_TreeNode && rootNode, _Callable && func)
{
    int nDepth = 0;

    auto cur = to_reference(rootNode).last_child();

    if (cur)
    {
        ++nDepth;

        do
        {
            if (to_reference(cur).last_child())
            {
                ++nDepth;
                cur = to_reference(cur).last_child();
            }
            else
            {
                if (func(cur, nDepth) < 0)
                {
                    return;
                }

                if (to_reference(cur).previous_sibling())
                {
                    cur = to_reference(cur).previous_sibling();
                }
                else
                {
                    while (!to_reference(cur).previous_sibling()
                        && cur != rootNode
                        && to_reference(cur).parent())
                    {
                        --nDepth;
                        cur = to_reference(cur).parent();

                        if (cur != rootNode)
                        {
                            if (func(cur, nDepth) < 0)
                            {
                                return;
                            }
                        }
                    }

                    if (cur != rootNode)
                    {
                        cur = to_reference(cur).previous_sibling();
                    }
                }
            }
        } while (cur && cur != rootNode);
    }

    func(rootNode, 0);
}

SHARELIB_END_NAMESPACE
