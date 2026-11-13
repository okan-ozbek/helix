//
// Created by Okan Can Özbek on 13/11/2026.
//

#include "../include/Order.h"

#include <format>
#include <stdexcept>

Order::Order(const id_t id, const price_t price, const quantity_t quantity, const Side side)
    : m_id(id)
    , m_price(price)
    , m_remaining_quantity(quantity)
    , m_initial_quantity(quantity)
    , m_side(side)
{}

[[nodiscard]]
id_t Order::Id() const
{
    return m_id;
}

[[nodiscard]]
price_t Order::Price() const
{
    return m_price;
}

[[nodiscard]]
quantity_t Order::RemainingQuantity() const
{
    return m_remaining_quantity;
}

[[nodiscard]]
quantity_t Order::InitialQuantity() const
{
    return m_initial_quantity;
}

void Order::Fill(const quantity_t quantity)
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
bool Order::IsFilled() const
{
    return m_remaining_quantity == 0;
}

[[nodiscard]]
bool Order::IsSell() const
{
    return m_side == Side::Sell;
}

[[nodiscard]]
bool Order::IsBuy() const
{
    return m_side == Side::Buy;
}