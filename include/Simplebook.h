//
// Created by Okan Can Özbek on 12/11/2026.
//

#ifndef HELIX_SIMPLEBOOK_H
#define HELIX_SIMPLEBOOK_H

#include <algorithm>
#include <cstdint>
#include <map>
#include <ranges>
#include <stdexcept>
#include <unordered_map>
#include <vector>

using id_t = std::uint64_t;
using price_t = std::int32_t;
using quantity_t = std::int32_t;
using large_price_t = std::int64_t;
using large_quantity_t = std::int64_t;

class Order
{
public:
    enum class Side : std::uint8_t
    {
        Buy,
        Sell,
    };

    Order(const id_t id, const price_t price, const quantity_t quantity, const Side side)
        : m_id(id)
        , m_price(price)
        , m_remaining_quantity(quantity)
        , m_initial_quantity(quantity)
        , m_side(side)
    {}

    [[nodiscard]]
    id_t Id() const
    {
        return m_id;
    }

    [[nodiscard]]
    price_t Price() const
    {
        return m_price;
    }

    [[nodiscard]]
    quantity_t RemainingQuantity() const
    {
        return m_remaining_quantity;
    }

    [[nodiscard]]
    quantity_t InitialQuantity() const
    {
        return m_initial_quantity;
    }

    void Fill(const quantity_t quantity)
    {
        if (quantity > m_remaining_quantity)
        {
            throw std::runtime_error(
                std::format("Fill quantity {} exceeds remaining quantity {}", std::to_string(quantity), std::to_string(m_remaining_quantity))
            );
        }

        m_remaining_quantity -= quantity;
    }

    [[nodiscard]]
    bool IsFilled() const
    {
        return m_remaining_quantity == 0;
    }

    [[nodiscard]]
    bool IsSell() const
    {
        return m_side == Side::Sell;
    }

    [[nodiscard]]
    bool IsBuy() const
    {
        return m_side == Side::Buy;
    }

private:
    id_t m_id;
    price_t m_price;
    quantity_t m_remaining_quantity;
    quantity_t m_initial_quantity;
    Side m_side;
};

using Orders = std::vector<Order>;

class Orderbook
{
public:
    using bids_t = std::map<price_t, std::map<quantity_t, Orders>, std::greater<>>;
    using asks_t = std::map<price_t, std::map<quantity_t, Orders>, std::less<>>;
    using lookup_table_t = std::unordered_map<id_t, Order>;

    Orderbook() = default;

    explicit Orderbook(const quantity_t order_limit)
        : m_order_limit{ order_limit }
    {}

    enum class Status : std::uint8_t
    {
        Success,
        OrderNotFound,
        OrderAlreadyExists,
        InvalidOperation,
        CapacityReached,
    };

    struct PriceLevel
    {
        large_price_t price{};
        large_quantity_t total_quantity{};
    };

    [[nodiscard]]
    std::optional<PriceLevel> BestBid() const
    {
        if (m_bids.empty()) {
            return std::nullopt;
        }

        const auto& [best_price, quantity_map] = *m_bids.begin();

        large_quantity_t total_quantity = 0;
        for (const auto & [quantity, orders]: quantity_map) {
            total_quantity += quantity * static_cast<large_quantity_t>(orders.size());
        }

        return PriceLevel{ best_price, total_quantity };
    }

    [[nodiscard]]
    std::optional<PriceLevel> BestAsk() const
    {
        if (m_asks.empty()) {
            return std::nullopt;
        }

        const auto& [best_price, quantity_map] = *m_asks.begin();

        large_quantity_t total_quantity = 0;
        for (const auto & [quantity, orders]: quantity_map) {
            total_quantity += quantity * static_cast<large_quantity_t>(orders.size());
        }

        return PriceLevel{ best_price, total_quantity };
    }

    Status Add(const Order& order)
    {
        if (m_lookup_table.contains(order.Id())) {
            return Status::OrderAlreadyExists;
        }

        if (IsFull()) {
            return Status::CapacityReached;
        }

        if (IsInvalidAdd(order)) {
            return Status::InvalidOperation;
        }

        if (order.IsBuy()) {
            m_bids[order.Price()][order.RemainingQuantity()].emplace_back(order);
        }
        else if (order.IsSell()) {
            m_asks[order.Price()][order.RemainingQuantity()].emplace_back(order);
        }

        m_lookup_table.emplace(order.Id(), order);

        return Status::Success;
    }

    Status Reduce(const id_t id, const quantity_t quantity_to_remove)
    {
        if (!m_lookup_table.contains(id)) {
            return Status::OrderNotFound;
        }

        auto& order{ m_lookup_table.at(id) };

        if (
            quantity_to_remove == 0 ||
            quantity_to_remove > order.RemainingQuantity()
        ) {
            return Status::InvalidOperation;
        }

        if (quantity_to_remove == order.RemainingQuantity()) {
            Delete(id);

            return Status::Success;
        }

        const auto temp_remaining_quantity{ order.RemainingQuantity() };

        try {
            order.Fill(quantity_to_remove);
        }
        catch (...) {
            return Status::InvalidOperation;
        }

        if (order.IsBuy()) {
            UpdateMap(m_bids, order, temp_remaining_quantity);
        }
        else if (order.IsSell()) {
            UpdateMap(m_asks, order, temp_remaining_quantity);
        }

        return Status::Success;
    }

    Status Delete(const id_t id)
    {
        if (!m_lookup_table.contains(id)) {
            return Status::OrderNotFound;
        }

        if (IsInvalidDelete(id)) {
            return Status::InvalidOperation;
        }

        const auto& order{ m_lookup_table.at(id) };

        if (order.IsBuy()) {
            RemoveItem(m_bids, order);
        }
        else if (order.IsSell()) {
            RemoveItem(m_asks, order);
        }

        return Status::Success;
    }

    Status Replace(const id_t id, const id_t new_id, const price_t new_price, const quantity_t new_quantity)
    {
        if (IsInvalidReplace(id, new_id, new_price, new_quantity)) {
            return Status::InvalidOperation;
        }

        if (!m_lookup_table.contains(id)) {
            return Status::OrderNotFound;
        }

        if (m_lookup_table.contains(new_id)) {
            return Status::OrderAlreadyExists;
        }

        const auto& side = m_lookup_table.at(id).IsBuy()
            ? Order::Side::Buy
            : Order::Side::Sell;

        Delete(id);
        Add({ new_id, new_price, new_quantity, side });

        return Status::Success;
    }

    void Info() const
    {
        std::cout << "Orderbook Info:" << std::endl;
        std::cout << "Bids: " << m_bids.size() << " price levels" << std::endl;
        std::cout << "Asks: " << m_asks.size() << " price levels" << std::endl;
        std::cout << "Total Orders: " << m_lookup_table.size() << std::endl;
    }

private:
    bids_t m_bids;
    asks_t m_asks;
    lookup_table_t m_lookup_table;
    quantity_t m_order_limit{ 1024 };

    [[nodiscard]]
    bool IsFull() const
    {
        return m_lookup_table.size() >= m_order_limit;
    }

    [[nodiscard]]
    static bool IsInvalidAdd(const Order& order)
    {
        return
            order.Id() <= 0 ||
            order.Price() <= 0 ||
            order.RemainingQuantity() <= 0;
    }

    [[nodiscard]]
    static bool IsInvalidReplace(const id_t id, const id_t new_id, const price_t new_price, const quantity_t new_quantity)
    {
        return
            id <= 0 ||
            new_id <= 0 ||
            new_price <= 0 ||
            new_quantity <= 0;
    }

    static bool IsInvalidDelete(const id_t id)
    {
        return
            id <= 0;
    }

    void RemoveItem(auto& price_map, const Order& order)
    {
        auto& quantity_map{ price_map.at(order.Price()) };
        auto& orders{ quantity_map.at(order.RemainingQuantity()) };

        orders.erase(
            std::remove_if(orders.begin(), orders.end(),
            [&](const Order& current_order) -> bool {
                return current_order.Id() == order.Id();
            }),
            orders.end()
        );

        if (orders.empty()) {
            quantity_map.erase(order.RemainingQuantity());
        }

        if (quantity_map.empty()) {
            price_map.erase(order.Price());
        }

        m_lookup_table.erase(order.Id());
    }

    void UpdateMap(auto& price_map, const Order& order, const quantity_t temp_remaining_quantity)
    {
        auto& quantity_map{ price_map.at(order.Price()) };
        auto& orders{ quantity_map.at(temp_remaining_quantity) };

        orders.erase(
            std::remove_if(orders.begin(), orders.end(),
            [&](const Order& current_order) -> bool {
                return current_order.Id() == order.Id();
            }),
            orders.end()
        );

        if (orders.empty()) {
            quantity_map.erase(temp_remaining_quantity);
        }

        if (quantity_map.empty()) {
            price_map.erase(order.Price());
        }

        price_map[order.Price()][order.RemainingQuantity()].emplace_back(order);
    }
};

#endif //HELIX_SIMPLEBOOK_H
