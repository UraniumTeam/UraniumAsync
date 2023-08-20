#include <Tests/Common/Common.h>
#include <UnAsync/Buffers/ReadOnlySequence.h>
#include <UnAsync/Buffers/SequenceReader.h>
#include <UnAsync/Pipes/Internal/BufferSegment.h>
#include <UnTL/Containers/HeapArray.h>
#include <UnTL/Strings/StringSlice.h>

using namespace UN;
using namespace UN::Async;

using UN::Async::Internal::BufferSegment;

inline StringSlice BufferToString(const ArraySlice<const Byte>& buffer)
{
    return { reinterpret_cast<const char*>(buffer.Data()), buffer.Length() };
}

inline ArraySlice<Byte> AllocateMemory(USize size)
{
    Byte* pData = static_cast<Byte*>(SystemAllocator::Get()->Allocate(size, MaximumAlignment));
    return { pData, size };
}

inline void DeallocateMemory(const ArraySlice<Byte>& memory)
{
    SystemAllocator::Get()->Deallocate(memory.Data());
}

inline ReadOnlySequence<Byte> Create(USize segmentSize, USize segmentEnd, USize segmentCount, USize startIndex = 0,
                                     USize endIndex = std::numeric_limits<USize>::max())
{
    List<BufferSegment*> segments;
    for (USize i = 0; i < segmentCount; ++i)
    {
        auto* pSegment = segments.Emplace(new BufferSegment);
        pSegment->SetMemory(AllocateMemory(segmentSize));
        pSegment->AdvanceEnd(segmentEnd);
    }

    for (USize i = 0; i < segmentCount - 1; ++i)
    {
        segments[i]->SetNext(segments[i + 1]);
    }

    auto begin = SequencePosition<Byte>(segments.Front(), startIndex);
    auto end   = SequencePosition<Byte>(segments.Back(), std::min(endIndex, segments.Back()->Length()));

    return { begin, end };
}

inline ReadOnlySequence<Byte> Create(const List<StringSlice>& strings, USize startIndex = 0,
                                     USize endIndex = std::numeric_limits<USize>::max())
{
    List<BufferSegment*> segments;
    for (USize i = 0; i < strings.Size(); ++i)
    {
        auto* pSegment = segments.Emplace(new BufferSegment);
        auto memory    = AllocateMemory(strings[i].Size());
        memcpy(memory.Data(), strings[i].Data(), strings[i].Size());
        pSegment->SetMemory(memory);
        pSegment->AdvanceEnd(memory.Length());
    }

    for (USize i = 0; i < strings.Size() - 1; ++i)
    {
        segments[i]->SetNext(segments[i + 1]);
    }

    auto begin = SequencePosition<Byte>(segments.Front(), startIndex);
    auto end   = SequencePosition<Byte>(segments.Back(), std::min(endIndex, segments.Back()->Length()));

    return { begin, end };
}

inline void Destroy(ReadOnlySequence<Byte>& sequence, bool deallocateBuffers = true)
{
    List<ArraySlice<Byte>> buffers;
    List<IReadOnlySequenceSegment<Byte>*> segments;
    for (auto iter = sequence.begin(); iter != sequence.end(); ++iter)
    {
        auto memory = static_cast<BufferSegment*>(iter.Segment())->GetAvailableMemory();
        buffers.Push(memory);
        segments.Push(iter.Segment());
    }

    sequence = {};
    for (auto* pSegment : segments)
    {
        delete pSegment;
    }

    if (!deallocateBuffers)
    {
        return;
    }

    for (auto& buffer : buffers)
    {
        DeallocateMemory(buffer);
    }
}

TEST(ReadOnlySequence, Empty)
{
    ReadOnlySequence<char> s;
    EXPECT_EQ(s.GetLength(), 0);
}

TEST(ReadOnlySequence, Length)
{
    {
        auto s = Create(100, 100, 10);
        EXPECT_EQ(s.GetLength(), 1000);
        Destroy(s);
    }
    {
        auto s = Create(100, 100, 1);
        EXPECT_EQ(s.GetLength(), 100);
        Destroy(s);
    }
    {
        auto s = Create(100, 50, 10);
        EXPECT_EQ(s.GetLength(), 500);
        Destroy(s);
    }
}

TEST(ReadOnlySequence, Iterate)
{
    auto s           = Create(100, 50, 10, 10, 90);
    USize totalBytes = 0;
    for (auto buffer : s)
    {
        totalBytes += buffer.Length();
    }

    EXPECT_EQ(totalBytes, 50 * 10 - 20);
    Destroy(s);
}

TEST(ReadOnlySequence, CopyDataTo)
{
    List<StringSlice> strings = { "##c123", "def456", "ghi7##" };
    auto s                    = Create(strings, 2, 4);

    auto dest = HeapArray<Byte>::CreateUninitialized(6 * 3 + 1);
    dest[14]  = static_cast<Byte>(0); // set '\0' before the copy to ensure that we don't touch the data past the end
    s.CopyDataTo(dest);

    EXPECT_EQ(StringSlice(reinterpret_cast<const char*>(dest.Data())), "c123def456ghi7");
    Destroy(s, false);
}

TEST(ReadOnlySequence, Slice)
{
    List<StringSlice> strings = { "#bc123", "def456", "ghi78#" };

    auto s     = Create(strings, 1, 5);
    auto slice = s(s.Seek(s.BeginPosition(), 2), s.Seek(s.BeginPosition(), 12));

    auto dest = HeapArray<Byte>::CreateUninitialized(6 * 3 + 1);
    dest[10]  = static_cast<Byte>(0);
    slice.CopyDataTo(dest);

    EXPECT_EQ(StringSlice(reinterpret_cast<const char*>(dest.Data())), "123def456g");
    Destroy(s, false);
}

TEST(ReadOnlySequence, SliceToEnd)
{
    auto s     = Create(10, 10, 10);
    auto slice = s(s.EndPosition());
    EXPECT_EQ(slice.GetLength(), 0);
    Destroy(s);
}

TEST(ReadOnlySequence, SliceIndexed)
{
    List<StringSlice> strings = { "#bc123", "def456", "ghi78#" };

    auto s     = Create(strings, 1, 5);
    auto slice = s(2, 12);

    auto dest = HeapArray<Byte>::CreateUninitialized(6 * 3 + 1);
    dest[10]  = static_cast<Byte>(0);
    slice.CopyDataTo(dest);

    EXPECT_EQ(StringSlice(reinterpret_cast<const char*>(dest.Data())), "123def456g");
    Destroy(s, false);
}

TEST(SequenceReader, UnreadSequence)
{
    List<StringSlice> strings = { "#abbbb", "bbcccc", "ccccd###" };
    auto s                    = Create(strings, 1, 5);
    auto reader               = SequenceReader(s);
    auto unread               = reader.UnreadSequence();
    auto unreadReader         = SequenceReader(unread);

    EXPECT_EQ(BufferToString(unreadReader.UnreadBuffer()), "abbbb");
}

TEST(SequenceReader, Construct)
{
    List<StringSlice> strings = { "#bc123", "def456", "ghi78#" };
    auto s                    = Create(strings, 1, 5);
    auto reader               = SequenceReader(s);

    EXPECT_EQ(BufferToString(reader.UnreadBuffer()), "bc123");
    EXPECT_EQ(BufferToString(reader.CurrentBuffer()), "bc123");
    EXPECT_EQ(reader.CurrentBufferIndex(), 0);
    EXPECT_EQ(reader.Consumed(), 0);
    EXPECT_EQ(reader.Length(), s.GetLength());

    Destroy(s, false);
}

TEST(SequenceReader, Advance)
{
    List<StringSlice> strings = { "#bc123", "def456", "ghi78#" };
    auto s                    = Create(strings, 1, 5);
    auto reader               = SequenceReader(s);

    reader.Advance(1);
    EXPECT_EQ(BufferToString(reader.UnreadBuffer()), "c123");

    reader.Advance(5);
    EXPECT_EQ(BufferToString(reader.UnreadBuffer()), "ef456");

    reader.Advance(10);
    EXPECT_EQ(BufferToString(reader.UnreadBuffer()), "");

    Destroy(s, false);
}

TEST(SequenceReader, Peek)
{
    List<StringSlice> strings = { "#bc123", "def456", "ghi78#" };
    auto s                    = Create(strings, 1, 5);
    auto reader               = SequenceReader(s);

    EXPECT_EQ(un_enum_cast(reader.Peek().value()), 'b');
    EXPECT_EQ(un_enum_cast(reader.Peek().value()), 'b');
    reader.Advance(2);
    EXPECT_EQ(un_enum_cast(reader.Peek().value()), '1');
    reader.Advance(4);
    EXPECT_EQ(un_enum_cast(reader.Peek().value()), 'e');
    reader.Advance(10);
    EXPECT_EQ(reader.Peek(), std::nullopt);

    Destroy(s, false);
}

TEST(SequenceReader, AdvancePast)
{
    List<StringSlice> strings = { "#abbbb", "bbcccc", "ccccd###" };
    auto s                    = Create(strings, 1, 5);
    auto reader               = SequenceReader(s);

    reader.AdvancePast(static_cast<Byte>('a'));
    EXPECT_EQ(un_enum_cast(reader.Peek().value()), 'b');
    reader.AdvancePast(static_cast<Byte>('a'));
    EXPECT_EQ(un_enum_cast(reader.Peek().value()), 'b');
    reader.AdvancePast(static_cast<Byte>('q'));
    EXPECT_EQ(un_enum_cast(reader.Peek().value()), 'b');
    reader.AdvancePast(static_cast<Byte>('b'));
    EXPECT_EQ(un_enum_cast(reader.Peek().value()), 'c');
    reader.AdvancePast(static_cast<Byte>('c'));
    EXPECT_EQ(un_enum_cast(reader.Peek().value()), 'd');
    reader.AdvancePast(static_cast<Byte>('d'));
    EXPECT_EQ(reader.Peek(), std::nullopt);

    Destroy(s, false);
}

TEST(SequenceReader, AdvancePastAny)
{
    List<StringSlice> strings = { "#abbbb", "bbcccc", "ccccd###" };
    auto s                    = Create(strings, 1, 5);
    auto reader               = SequenceReader(s);

    reader.AdvancePastAny(static_cast<Byte>('a'), static_cast<Byte>('b'), static_cast<Byte>('c'));
    EXPECT_EQ(un_enum_cast(reader.Peek().value()), 'd');
    reader.AdvancePast(static_cast<Byte>('d'));
    EXPECT_EQ(reader.Peek(), std::nullopt);

    Destroy(s, false);
}

TEST(SequenceReader, AdvancePastAnyArray)
{
    List<StringSlice> strings = { "#abbbb", "bbcccc", "ccccd###" };
    auto s                    = Create(strings, 1, 5);
    auto reader               = SequenceReader(s);

    reader.AdvancePastAny({});
    EXPECT_EQ(un_enum_cast(reader.Peek().value()), 'a');
    reader.AdvancePastAny({ static_cast<Byte>('a'), static_cast<Byte>('b'), static_cast<Byte>('c') });
    EXPECT_EQ(un_enum_cast(reader.Peek().value()), 'd');
    reader.AdvancePast(static_cast<Byte>('d'));
    EXPECT_EQ(reader.Peek(), std::nullopt);

    Destroy(s, false);
}

TEST(SequenceReader, TryReadTo)
{
    List<StringSlice> strings = { "#bc123", "def456", "ghi78#" };
    auto s                    = Create(strings, 1, 5);
    auto reader               = SequenceReader(s);

    ArraySlice<const Byte> buffer;
    HeapArray<Byte> owner;
    EXPECT_TRUE(reader.TryReadTo(buffer, owner, static_cast<Byte>('1')));
    EXPECT_EQ(BufferToString(reader.UnreadBuffer()), "23");
    EXPECT_EQ(BufferToString(buffer), "bc");
    EXPECT_EQ(owner.Length(), 0);

    Destroy(s, false);
}

TEST(SequenceReader, TryReadToWithOwner)
{
    List<StringSlice> strings = { "#bc123", "def456", "ghi78#" };
    auto s                    = Create(strings, 1, 5);
    auto reader               = SequenceReader(s);

    ArraySlice<const Byte> buffer;
    HeapArray<Byte> owner;
    EXPECT_TRUE(reader.TryReadTo(buffer, owner, static_cast<Byte>('i')));
    EXPECT_EQ(BufferToString(reader.UnreadBuffer()), "78");
    EXPECT_EQ(BufferToString(buffer), "bc123def456gh");
    EXPECT_EQ(BufferToString(owner), "bc123def456gh");

    Destroy(s, false);
}
