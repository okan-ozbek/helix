//
// Created by Okan Can Özbek on 12/11/2026.
//

#include <chrono>
#include <iostream>

#include "../include/Replay.h"
#include "../include/Simplebook.h"

int main()
{
    auto orderbook = Orderbook{};
    auto replay = Replay{ orderbook, "../data/Test.txt" };

    const auto start = std::chrono::steady_clock::now();

    replay.Run();

    const auto end = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    const double seconds = std::chrono::duration<double>(end - start).count();
    const double messages_per_second = static_cast<double>(replay.GetEventCount()) / seconds;

    std::cout << "Execution time: " << duration << " us" << std::endl;
    std::cout << "Synthetic messages: " << messages_per_second << " messages/s" << std::endl;

    replay.Info();
    orderbook.Info();

    std::cin.get();

    return 0;
}
