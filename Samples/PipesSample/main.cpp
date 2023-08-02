#include <UnAsync/Cancellation/CancellationSource.h>
#include <UnAsync/Cancellation/CancellationToken.h>
#include <UnAsync/Jobs/JobScheduler.h>
#include <UnAsync/Pipes/Pipe.h>
#include <UnAsync/Pipes/PipeReader.h>
#include <UnAsync/Pipes/PipeWriter.h>
#include <UnAsync/SyncWait.h>
#include <UnAsync/Task.h>
#include <UnAsync/WhenAll.h>
#include <UnTL/Strings/Format.h>
#include <iostream>
#include <ranges>

using namespace UN;
using namespace UN::Async;

Ptr<IJobScheduler> pScheduler = AllocateObject<JobScheduler>(4);

USize bytesWritten = 0;
USize bytesRead    = 0;

Task<> ReceiveData(const ArraySlice<Byte>& memory)
{
    //    co_await Job::Run(pScheduler.Get());
    //    using namespace std::chrono_literals;
    //    std::this_thread::sleep_for(1us);

    for (auto i : std::views::iota(static_cast<USize>(0), memory.Length()))
    {
        memory[i] = static_cast<Byte>(i);
    }

    co_return;
}

Task<> FillPipeTask(const PipeWriter& writer, const CancellationToken& token)
{
    constexpr auto minimumBufferSize = 512;

    // for (auto i : std::views::iota(0, 100))
    while (true)
    {
        // std::cout << Fmt::Format("Writing {}\n", i) << std::flush;
        auto memory = writer.GetMemory(minimumBufferSize);
        co_await ReceiveData(memory);

        writer.Advance(memory.Length());
        bytesWritten += memory.Length();

        auto flush = co_await writer.FlushAsync(token);

        if (flush.IsCompleted())
        {
            break;
        }

        if (flush.IsCancelled())
        {
            std::cout << "CANCEL\n" << std::flush;
            break;
        }
    }

    writer.Complete();
}

Task<> ReadPipeTask(const PipeReader& reader, const CancellationToken& token)
{
    while (true)
    {
        auto read = co_await reader.ReadAsync(token);

        //        using namespace std::chrono_literals;
        //        std::this_thread::sleep_for(100ms);

        auto buffer = read.GetMemory();
        //        String s;
        for (auto& byte : buffer)
        {
            (void)byte;
            //            s += Fmt::Format("{}", static_cast<Int32>(byte));
            bytesRead++;
        }

        // std::cout << Fmt::Format("Read {}\n", s(0, 16)) << std::flush;
        reader.Advance(buffer.end());

        if (read.IsCompleted() || read.IsCancelled())
        {
            break;
        }
    }

    reader.Complete();
}

Task<> Process()
{
    Ptr pPipe = AllocateObject<Pipe>(PipeDesc{ .JobScheduler = pScheduler });

    AsyncEvent cancellationThreadFinished;

    CancellationSource source;
    auto token = source.GetToken();
    Job::RunOneTime(pScheduler.Get(), [&source, &cancellationThreadFinished]() {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(5s);
        source.Cancel();
        cancellationThreadFinished.Set();
    });
    co_await WhenAllReady(FillPipeTask(PipeWriter(pPipe.Get()), token), ReadPipeTask(PipeReader(pPipe.Get()), token));
    co_await cancellationThreadFinished;
}

int main()
{
    SyncWait(Process());

    std::cout << "Write: " << bytesWritten << "; Read: " << bytesRead << ";" << std::flush;
    return 0;
}
