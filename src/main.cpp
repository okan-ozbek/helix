//
// Created by Okan Can Özbek on 12/11/2026.
//

#include <chrono>
#include <iostream>

#include "../include/Replay.h"
#include "../include/Simplebook.h"

class Benchmark
{
public:
    explicit Benchmark(Replay& replay)
        : m_replay{ std::make_unique<Replay>(replay) }
        , m_start{ std::chrono::steady_clock::now() }
    {}

    ~Benchmark()
    {
        m_end = { std::chrono::steady_clock::now() };

        const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(m_end - m_start).count();
        const double seconds = std::chrono::duration<double>(m_end - m_start).count();
        const double messages_per_second = static_cast<double>(m_replay->GetEventCount()) / seconds;

        std::cout << "Execution time: " << duration << " us" << std::endl;
        std::cout << "Synthetic messages: " << messages_per_second << " messages/s" << std::endl;
    }

private:
    std::unique_ptr<Replay> m_replay;

    std::chrono::time_point<std::chrono::steady_clock> m_start;
    std::chrono::time_point<std::chrono::steady_clock> m_end;
};

int main()
{
    auto orderbook = Orderbook{};
    auto replay = Replay{ orderbook, "../data/test.txt" };
    
    {
        const auto benchmark{ Benchmark{ replay } };

        replay.Run();
    }

    replay.Info();
    orderbook.Info();

    std::cin.get();

    return 0;
}
