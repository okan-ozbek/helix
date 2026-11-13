//
// Created by Okan Can Özbek on 12/11/2026.
//

#ifndef HELIX_EVENT_H
#define HELIX_EVENT_H

#include "Orderbook.h"

enum class Command : std::uint8_t
{
    Add,
    Reduce,
    Delete,
    Replace,
};

struct Event
{
    Command command{};
    id_t id{};
    id_t new_id{};
    price_t price{};
    quantity_t quantity{};
    Order::Side side{};
};


#endif //HELIX_EVENT_H
