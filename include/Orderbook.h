//
// Created by Okan Can Özbek on 12/11/2026.
//

#ifndef HELIX_SIMPLEBOOK_H
#define HELIX_SIMPLEBOOK_H

#include <cstdint>
#include <map>
#include <ranges>
#include <unordered_map>
#include <vector>

#include "Order.h"

using id_t = std::uint64_t;
using price_t = std::int32_t;
using quantity_t = std::int32_t;
using large_price_t = std::int64_t;
using large_quantity_t = std::int64_t;

class Order;

class Orderbook
{
public:
    Orderbook() = default;
    explicit Orderbook(std::size_t);

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

    [[nodiscard]] std::optional<PriceLevel> BestBid() const;
    [[nodiscard]] std::optional<PriceLevel> BestAsk() const;

    Status Add(const Order&);
    Status Reduce(id_t, quantity_t);
    Status Delete(id_t id);
    Status Replace(id_t, id_t, price_t, quantity_t);

    void Info() const;

private:
    using bids_t = std::map<price_t, std::map<quantity_t, Orders>, std::greater<>>;
    using asks_t = std::map<price_t, std::map<quantity_t, Orders>, std::less<>>;
    using lookup_table_t = std::unordered_map<id_t, Order>;
    using limit_t = std::size_t;

    bids_t m_bids;
    asks_t m_asks;
    lookup_table_t m_lookup_table;
    limit_t m_order_limit{ 1024 };

    [[nodiscard]] bool IsFull() const;
    [[nodiscard]] static bool IsInvalidAdd(const Order&);
    [[nodiscard]] static bool IsInvalidReplace(id_t, id_t, price_t, quantity_t);
    [[nodiscard]] static bool IsInvalidDelete(id_t);

    void RemoveItem(auto&, const Order&);
    void UpdateMap(auto&, const Order&, quantity_t);
};

#endif //HELIX_SIMPLEBOOK_H
