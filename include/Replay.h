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

            m_events.push_back(*event);
        }
    }

    std::optional<Event> ParseEvent(const std::string_view command, std::istream& fields)
    {
        if (command == "ADD") {
            id_t id;
            price_t price;
            quantity_t quantity;
            std::string side_token;

            if (!(fields >> id >> side_token >> price >> quantity)) {
                throw std::runtime_error(
                    std::format(
                        "Invalid Add event format: {}, {}, {}, {}",
                        std::to_string(id),
                        std::to_string(price),
                        std::to_string(quantity),
                        side_token
                    )
                );
            }

            const auto side_opt = TryGetSide(side_token);
            if (!side_opt) {
                throw std::runtime_error(std::format("Invalid side token: {}", side_token));
            }

            return Event{
                .command = Command::Add,
                .id = id,
                .price = price,
                .quantity = quantity,
                .side = *side_opt
            };
        }

        if (command == "REDUCE") {
            id_t id;
            quantity_t quantity;

            if (!(fields >> id >> quantity)) {
                throw std::runtime_error(
                    std::format(
                        "Invalid Reduce event format: {}, {}",
                        std::to_string(id),
                        std::to_string(quantity)
                    )
                );
            }

            return Event{
                .command = Command::Reduce,
                .id = id,
                .quantity = quantity
            };
        }

        if (command == "DELETE") {
            id_t id;

            if (!(fields >> id)) {
                throw std::runtime_error(
                    std::format("Invalid Delete event format: {}", std::to_string(id))
                );
            }

            return Event{
                .command = Command::Delete,
                .id = id
            };
        }

        if (command == "REPLACE") {
            id_t id;
            id_t new_id;
            price_t price;
            quantity_t quantity;

            if (!(fields >> id >> new_id >> price >> quantity)) {
                throw std::runtime_error(
                    std::format(
                        "Invalid Replace event format: {}, {}, {}, {}",
                        std::to_string(id),
                        std::to_string(new_id),
                        std::to_string(price),
                        std::to_string(quantity)
                    )
                );
            }

            return Event{
                .command = Command::Replace,
                .id = id,
                .new_id = new_id,
                .price = price,
                .quantity = quantity,
            };
        }

        return std::nullopt;
    }

    [[nodiscard]] std::optional<Order::Side> TryGetSide(const std::string& side_token) const
    {
        if (side_token == "BUY") {
            return Order::Side::Buy;
        }

        if (side_token == "SELL") {
            return Order::Side::Sell;
        }

        return std::nullopt;
    }
};

#endif //HELIX_REPLAY_H
