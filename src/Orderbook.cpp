//
// Created by Okan Can Özbek on 13/11/2026.
//

#include "../include/Orderbook.h"

#include <algorithm>
#include <iostream>

Orderbook::Orderbook(const limit_t order_limit)
    : m_order_limit{ order_limit }
{}

[[nodiscard]]
std::optional<Orderbook::PriceLevel> Orderbook::BestBid() const
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
std::optional<Orderbook::PriceLevel> Orderbook::BestAsk() const
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

Orderbook::Status Orderbook::Add(const Order& order)
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

Orderbook::Status Orderbook::Reduce(const id_t id, const quantity_t quantity_to_remove)
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

Orderbook::Status Orderbook::Delete(const id_t id)
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

Orderbook::Status Orderbook::Replace(const id_t id, const id_t new_id, const price_t new_price, const quantity_t new_quantity)
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

void Orderbook::Info() const
{
    std::cout << "Orderbook Info:" << std::endl;
    std::cout << "Bids: " << m_bids.size() << " price levels" << std::endl;
    std::cout << "Asks: " << m_asks.size() << " price levels" << std::endl;
    std::cout << "Total Orders: " << m_lookup_table.size() << std::endl;
}

[[nodiscard]]
bool Orderbook::IsFull() const
{
    return m_lookup_table.size() >= m_order_limit;
}

[[nodiscard]]
bool Orderbook::IsInvalidAdd(const Order& order)
{
    return
        order.Id() <= 0 ||
        order.Price() <= 0 ||
        order.RemainingQuantity() <= 0;
}

[[nodiscard]]
bool Orderbook::IsInvalidReplace(const id_t id, const id_t new_id, const price_t new_price, const quantity_t new_quantity)
{
    return
        id <= 0 ||
        new_id <= 0 ||
        new_price <= 0 ||
        new_quantity <= 0;
}

[[nodiscard]]
bool Orderbook::IsInvalidDelete(const id_t id)
{
    return
        id <= 0;
}

void Orderbook::RemoveItem(auto& price_map, const Order& order)
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

void Orderbook::UpdateMap(auto& price_map, const Order& order, const quantity_t temp_remaining_quantity)
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