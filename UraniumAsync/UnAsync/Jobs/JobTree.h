#pragma once
#include <UnAsync/Internal/BoolPointer.h>
#include <UnTL/RTTI/RTTI.h>

namespace UN::Async
{
    class Job;

    class JobTree
    {
        JobTree* m_pSibling;
        // BoolPointer is used here to make sizeof(JobTree) = 16 bytes, not 17 for better alignment
        BoolPointer<JobTree> m_ChildCancelledPair;

    public:
        UN_RTTI_Struct(JobTree, "0E61A069-4E63-44D4-9084-DE314AFE8DA0");

        inline JobTree()
            : m_pSibling(nullptr)
            , m_ChildCancelledPair(nullptr, false)
        {
        }

        [[nodiscard]] inline bool IsCancelled() const
        {
            return m_ChildCancelledPair.GetBool();
        }

        inline void Cancel()
        {
            m_ChildCancelledPair.SetBool(true);
        }

        inline void AddChild(JobTree* child)
        {
            if (m_ChildCancelledPair.GetPointer())
            {
                child->m_pSibling = m_ChildCancelledPair.GetPointer();
            }
            m_ChildCancelledPair.SetPointer(child);
        }

        [[nodiscard]] inline JobTree* GetSibling() const
        {
            return m_pSibling;
        }

        [[nodiscard]] inline JobTree* GetFirstChild() const
        {
            return m_ChildCancelledPair.GetPointer();
        }
    };
} // namespace UN::Async
