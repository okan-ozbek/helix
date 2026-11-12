//
// Created by Okan Can Özbek on 12/11/2026.
//

#ifndef HELIX_REPLAY_H
#define HELIX_REPLAY_H

#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <sstream>
#include <optional>

#include "Event.h"
#include "Simplebook.h"

struct Event;

class Replay
{
public:
    Replay(Orderbook& orderbook, const std::string_view file_path)
        : m_orderbook(orderbook)
    {
        ParseFile(file_path);
    }

    void Run()
    {
        for (const auto& [command, id, new_id, price, quantity, side] : m_events) {
            switch (command) {
                case Command::Add:
                    m_status_counts[m_orderbook.Add(Order(id, price, quantity, side))]++;
                    break;
                case Command::Reduce:
                    m_status_counts[m_orderbook.Reduce(id, quantity)]++;
                    break;
                case Command::Delete:
                    m_status_counts[m_orderbook.Delete(id)]++;
                    break;
                case Command::Replace:
                    m_status_counts[m_orderbook.Replace(id, new_id, price, quantity)]++;
                    break;
                default:
                    throw std::runtime_error("Unknown command");
            }
        }
    }

    void Info() const
    {
        std::cout << "Replay Info:" << std::endl;
        std::cout << "Total Events: " << m_events.size() << std::endl;

        std::cout << "Status Counts:" << std::endl;
        for (const auto& [status, count] : m_status_counts) {
            std::cout << "  * " << GetStatusString(status) << ": " << count << std::endl;
        }
    }

    static std::string GetStatusString(const Orderbook::Status status)
    {
        switch (status) {
            case Orderbook::Status::Success:
                return "Success";
            case Orderbook::Status::OrderNotFound:
                return "Order Not Found";
            case Orderbook::Status::OrderAlreadyExists:
                return "Order Already Exists";
            case Orderbook::Status::InvalidOperation:
                return "Invalid Operation";
            default:
                return "Unknown Status";
        }
    }

    std::size_t GetEventCount() const
    {
        return m_events.size();
    }

private:
    std::vector<Event> m_events;
    std::unordered_map<Orderbook::Status, std::uint32_t> m_status_counts;
    Orderbook& m_orderbook;

    void ParseFile(const std::string_view file_path)
    {
        std::ifstream file(file_path.data());
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + std::string(file_path));
        }

        std::string line{};

        while (std::getline(file, line)) {
            std::istringstream fields{line};
            std::string command;

            if (!(fields >> command)) {
                continue;
            }

            auto event = ParseEvent(command, fields);

            if (!event) {
                throw std::runtime_error("Invalid event format: " + line);
            }

            std::string junk{};

            fields >> junk;
            if (!junk.empty()) {
                throw std::runtime_error("Extra fields in event: " + line);
            }

            m_events.push_back(*event);
        }
    }

    static std::optional<Event> ParseEvent(const std::string_view command, std::istream& fields)
    {
        if (command == "ADD") {
            return ParseAdd(command, fields);
        }

        if (command == "REDUCE") {
            return ParseReduce(command, fields);
        }

        if (command == "DELETE") {
            return ParseDelete(command, fields);
        }

        if (command == "REPLACE") {
            return ParseReplace(command, fields);
        }

        return std::nullopt;
    }

    static std::optional<Event> ParseAdd(const std::string_view command, std::istream& fields)
    {
        if (command != "ADD") {
            return std::nullopt;
        }

        id_t id;
        price_t price;
        quantity_t quantity;
        Order::Side side;

        std::string id_str;
        std::string price_str;
        std::string quantity_str;
        std::string side_str;

        if (!(fields >> id_str >> side_str >> price_str >> quantity_str)) {
            return std::nullopt;
        }

        if (
            !ValidInt(id_str, id) ||
            !ValidInt(price_str, price) ||
            !ValidInt(quantity_str, quantity) ||
            !ValidSide(side_str, side)
        ) {
            return std::nullopt;
        }

        return Event{
            .command = Command::Add,
            .id = id,
            .price = price,
            .quantity = quantity,
            .side = side,
        };
    }

    static std::optional<Event> ParseReduce(const std::string_view command, std::istream& fields)
    {
        if (command != "REDUCE") {
            return std::nullopt;
        }

        id_t id{};
        quantity_t quantity{};

        std::string id_str{};
        std::string quantity_str{};

        if (!(fields >> id_str >> quantity_str)) {
            return std::nullopt;
        }

        if (
            !ValidInt(id_str, id) ||
            !ValidInt(quantity_str, quantity)
        ) {
            return std::nullopt;
        }

        return Event{
            .command = Command::Reduce,
            .id = id,
            .quantity = quantity
        };
    }

    static std::optional<Event> ParseDelete(const std::string_view command, std::istream& fields)
    {
        if (command != "DELETE") {
            return std::nullopt;
        }

        id_t id{};
        std::string id_str{};

        if (!(fields >> id_str)) {
            return std::nullopt;
        }

        if (!ValidInt(id_str, id)) {
            return std::nullopt;
        }

        return Event{
            .command = Command::Delete,
            .id = id
        };
    }

    static std::optional<Event> ParseReplace(const std::string_view command, std::istream& fields)
    {
        if (command != "REPLACE") {
            return std::nullopt;
        }

        id_t id{};
        id_t new_id{};
        price_t price{};
        quantity_t quantity{};

        std::string id_str{};
        std::string new_id_str{};
        std::string price_str{};
        std::string quantity_str{};

        if (!(fields >> id_str >> new_id_str >> price_str >> quantity_str)) {
            return std::nullopt;
        }

        if (
            !ValidInt(id_str, id) ||
            !ValidInt(new_id_str, new_id) ||
            !ValidInt(price_str, price) ||
            !ValidInt(quantity_str, quantity)
        ) {
            return std::nullopt;
        }

        return Event{
            .command = Command::Replace,
            .id = id,
            .new_id = new_id,
            .price = price,
            .quantity = quantity
        };
    }

    [[nodiscard]]
    static bool ValidSide(const std::string& side_str, Order::Side& side)
    {
        if (side_str == "BUY") {
            side = Order::Side::Buy;
            return true;
        }

        if (side_str == "SELL") {
            side = Order::Side::Sell;
            return true;
        }

        return false;
    }


    template<typename T>
    [[nodiscard]]
    static bool ValidInt(const std::string& t_str, T& id)
    {
        const auto [ptr, ec]{ std::from_chars(t_str.data(), t_str.data() + t_str.size(), id) };

        return ptr == t_str.data() + t_str.size() && ec == std::errc{};
    }
};

#endif //HELIX_REPLAY_H
