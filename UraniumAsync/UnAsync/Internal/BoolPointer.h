#pragma once
#include <UnTL/Base/Base.h>
#include <atomic>

namespace UN
{
    //! \brief Stores a boolean-pointer pair in 8 bytes instead of 9.
    //!
    //! Every pointer is at least 2-bytes aligned, so the least significant bit
    //! of the pointer will always be 0. The BoolPointer class uses this fact to store a boolean
    //! value in the least significant bit.
    //!
    //! \tparam T Type of pointer to store.
    template<class T>
    class BoolPointer
    {
        std::atomic<UInt64> m_Data;

        inline static constexpr UInt64 BooleanMask = 1;
        inline static constexpr UInt64 PointerMask = ~BooleanMask;

    public:
        UN_FINLINE BoolPointer()
            : m_Data(0)
        {
        }

        UN_FINLINE BoolPointer(T* pointer, bool boolean)
            : m_Data(reinterpret_cast<Int64>(pointer) | (boolean ? 1 : 0))
        {
        }

        UN_FINLINE void SetPointer(T* pointer)
        {
            auto value = m_Data.load(std::memory_order_consume) & BooleanMask;
            m_Data.store(value | reinterpret_cast<Int64>(pointer), std::memory_order_acq_rel);
        }

        UN_FINLINE void SetBool(bool boolean)
        {
            // The class is currently used only by JobTree, so the pointer can be changed only on one thread.
            // The boolean value can be only set to true (cancelled state) and it is never going back.
            // That's why this should be thread-safe
            auto value = m_Data.load(std::memory_order_consume) & PointerMask;
            m_Data.store(value | (boolean ? 1 : 0), std::memory_order_acq_rel);
        }

        [[nodiscard]] UN_FINLINE T* GetPointer(std::memory_order memoryOrder = std::memory_order_seq_cst) const
        {
            return reinterpret_cast<T*>(m_Data.load(memoryOrder));
        }

        [[nodiscard]] UN_FINLINE bool GetBool(std::memory_order memoryOrder = std::memory_order_seq_cst) const
        {
            return m_Data.load(memoryOrder) & BooleanMask;
        }
    };
} // namespace UN
