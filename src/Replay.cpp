//
// Created by Okan Can Özbek on 13/11/2026.
//

#include "../include/Replay.h"

Replay::Replay(Orderbook& orderbook, const std::string_view file_path)
    : m_orderbook(orderbook)
{
    ParseFile(file_path);
}

void Replay::Run()
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

        // const auto& bestBid { m_orderbook.BestBid() };
        // const auto& bestAsk { m_orderbook.BestAsk() };
    }
}

void Replay::Info() const
{
    std::cout << "Replay Info:" << std::endl;
    std::cout << "Total Events: " << m_events.size() << std::endl;

    std::cout << "Status Counts:" << std::endl;
    for (const auto& [status, count] : m_status_counts) {
        std::cout << "  * " << GetStatusString(status) << ": " << count << std::endl;
    }
}

std::size_t Replay::GetEventCount() const
{
    return m_events.size();
}

std::string Replay::GetStatusString(const Orderbook::Status status)
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
        case Orderbook::Status::CapacityReached:
            return "Capacity Reached";
        default:
            return "Unknown Status";
    }
}

void Replay::ParseFile(const std::string_view file_path)
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

std::optional<Event> Replay::ParseEvent(const std::string_view command, std::istream& fields)
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

std::optional<Event> Replay::ParseAdd(const std::string_view command, std::istream& fields)
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

std::optional<Event> Replay::ParseReduce(const std::string_view command, std::istream& fields)
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

    if (quantity < 0) {
        return std::nullopt;
    }

    return Event{
        .command = Command::Reduce,
        .id = id,
        .quantity = quantity
    };
}

std::optional<Event> Replay::ParseDelete(const std::string_view command, std::istream& fields)
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

std::optional<Event> Replay::ParseReplace(const std::string_view command, std::istream& fields)
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
bool Replay::ValidSide(const std::string& side_str, Order::Side& side)
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
bool Replay::ValidInt(const std::string& t_str, T& id)
{
    const auto [ptr, ec]{ std::from_chars(t_str.data(), t_str.data() + t_str.size(), id) };

    return ptr == t_str.data() + t_str.size() && ec == std::errc{};
}

