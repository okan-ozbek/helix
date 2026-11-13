//
// Created by Okan Can Özbek on 13/11/2026.
//

#ifndef HELIX_ORDER_H
#define HELIX_ORDER_H

#include <cstdint>
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

    Order(id_t, price_t, quantity_t, Side);

    [[nodiscard]] id_t Id() const;
    [[nodiscard]] price_t Price() const;
    [[nodiscard]] quantity_t RemainingQuantity() const;
    [[nodiscard]] quantity_t InitialQuantity() const;

    void Fill(quantity_t);

    [[nodiscard]] bool IsFilled() const;
    [[nodiscard]] bool IsSell() const;
    [[nodiscard]] bool IsBuy() const;

private:
    id_t m_id;
    price_t m_price;
    quantity_t m_remaining_quantity;
    quantity_t m_initial_quantity;
    Side m_side;
};

using Orders = std::vector<Order>;

#endif //HELIX_ORDER_H
