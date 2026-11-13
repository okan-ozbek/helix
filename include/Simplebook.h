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

    enum class Status : std::uint8_t
    {
        Success,
        OrderNotFound,
        OrderAlreadyExists,
        InvalidOperation,
    };

    struct PriceLevel
    {
        price_t price{};
        quantity_t total_quantity{};
        //std::vector<Order> orders{};
    };

    [[nodiscard]]
    std::optional<PriceLevel> BestBid()
    {
        if (m_bids.empty()) {
            return std::nullopt;
        }

        const auto& [best_price, quantity_map] = *m_bids.begin();

        quantity_t total_quantity = 0;
        for (const auto & [quantity, orders]: quantity_map) {
            total_quantity += quantity * static_cast<quantity_t>(orders.size());
        }

        return PriceLevel{ best_price, total_quantity };
    }

    [[nodiscard]]
    std::optional<PriceLevel> BestAsk()
    {
        if (m_asks.empty()) {
            return std::nullopt;
        }

        const auto& [best_price, quantity_map] = *m_asks.begin();

        quantity_t total_quantity = 0;
        for (const auto & [quantity, orders]: quantity_map) {
            total_quantity += quantity * static_cast<quantity_t>(orders.size());
        }

        return PriceLevel{ best_price, total_quantity };
    }

    Status Add(const Order& order)
    {
        if (
            order.Id() <= 0 ||
            order.Price() <= 0 ||
            order.RemainingQuantity() <= 0
        ) {
            return Status::InvalidOperation;
        }

        if (m_lookup_table.contains(order.Id())) {
            return Status::OrderAlreadyExists;
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

        const auto& update_map{
            [&](auto& price_map) {
                auto& quantity_map{ price_map.at(order.Price()) };
                auto& orders{ quantity_map.at(temp_remaining_quantity) };

                orders.erase(
                    std::remove_if(orders.begin(), orders.end(),
                    [&](const Order& current_order) -> bool {
                        return current_order.Id() == id;
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

        if (order.IsBuy()) {
            update_map(m_bids);
        }
        else if (order.IsSell()) {
            update_map(m_asks);
        }

        return Status::Success;
    }

    Status Delete(const id_t id)
    {
        if (id <= 0) {
            return Status::InvalidOperation;
        }

        if (!m_lookup_table.contains(id)) {
            return Status::OrderNotFound;
        }

        const auto& order{ m_lookup_table.at(id) };
        const auto& remove_item{
            [&](auto& price_map) -> void {
                auto& quantity_map{ price_map.at(order.Price()) };
                auto& orders{ quantity_map.at(order.RemainingQuantity()) };

                orders.erase(
                    std::remove_if(orders.begin(), orders.end(),
                    [&](const Order& current_order) -> bool {
                        return current_order.Id() == id;
                    }),
                    orders.end()
                );

                if (orders.empty()) {
                    quantity_map.erase(order.RemainingQuantity());
                }

                if (quantity_map.empty()) {
                    price_map.erase(order.Price());
                }

                m_lookup_table.erase(id);
            }
        };

        if (order.IsBuy()) {
            remove_item(m_bids);
        }
        else if (order.IsSell()) {
            remove_item(m_asks);
        }

        return Status::Success;
    }

    Status Replace(const id_t id, const id_t new_id, const price_t new_price, const quantity_t new_quantity)
    {
        if (
            id <= 0 ||
            new_id <= 0 ||
            new_price <= 0 ||
            new_quantity <= 0
        ) {
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
};

#endif //HELIX_SIMPLEBOOK_H
