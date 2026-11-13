//
// Created by Okan Can Özbek on 12/11/2026.
//

#ifndef HELIX_REPLAY_H
#define HELIX_REPLAY_H

#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <sstream>
#include <optional>
#include <charconv>

#include "Event.h"
#include "Order.h"
#include "Orderbook.h"

struct Event;

class Replay
{
public:
    Replay(Orderbook&, std::string_view);

    void Run();

    void Info() const;



    std::size_t GetEventCount() const;

private:
    std::vector<Event> m_events;
    std::unordered_map<Orderbook::Status, std::uint32_t> m_status_counts;
    Orderbook& m_orderbook;

    static std::string GetStatusString(Orderbook::Status);

    void ParseFile(std::string_view);

    static std::optional<Event> ParseEvent(std::string_view, std::istream&);
    static std::optional<Event> ParseAdd(std::string_view, std::istream&);
    static std::optional<Event> ParseReduce(std::string_view, std::istream&);
    static std::optional<Event> ParseDelete(std::string_view, std::istream&);
    static std::optional<Event> ParseReplace(std::string_view, std::istream&);

    [[nodiscard]] static bool ValidSide(const std::string& side_str, Order::Side& side);

    template<typename T>
    [[nodiscard]] static bool ValidInt(const std::string& t_str, T& id);
};

#endif //HELIX_REPLAY_H
