#pragma once
#include <UnTL/Containers/ArraySlice.h>

namespace UN::Async
{
    template<class T>
    class [[nodiscard]] IReadOnlySequenceSegment
    {
    public:
        [[nodiscard]] virtual ArraySlice<const T> GetMemory() const     = 0;
        [[nodiscard]] virtual IReadOnlySequenceSegment<T>* Next() const = 0;
        [[nodiscard]] virtual USize RunningIndex() const                = 0;
        [[nodiscard]] virtual USize Length() const                      = 0;
    };
} // namespace UN::Async
