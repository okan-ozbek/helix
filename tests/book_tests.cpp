#include "Book.h"

#include <iostream>

int main()
{
    using Helix::Book;
    using Helix::Order;

    auto book{ Book{} };

    const auto check = [](const bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
        }
        return condition;
    };

    if (!check(!book.BestBid() && !book.BestAsk(), "New book should be empty")) {
        return 1;
    }

    if (!check(book.Add(Order{1, 100, 10, Order::Side::Buy}) == Book::Status::Success,
               "Adding a bid should succeed")) {
        return 1;
    }

    if (!check(book.Reduce(1, 3) == Book::Status::Success, "Reducing a bid should succeed")) {
        return 1;
    }

    const auto bid = book.BestBid();

    if (!check(bid && bid->price == 100 && bid->total_quantity == 7,
               "Best bid should reflect the reduced quantity")) {
        return 1;
    }

    if (!check(book.Reduce(1, 8) == Book::Status::InvalidOperation,
               "Reducing more than the remaining quantity should fail")) {
        return 1;
    }

    if (!check(book.Reduce(1, 7) == Book::Status::Success && !book.BestBid(),
               "Reducing the full remaining quantity should remove the bid")) {
        return 1;
    }

    if (!check(book.Reduce(1, 1) == Book::Status::OrderNotFound,
               "Reducing a removed order should fail")) {
        return 1;
    }

    return 0;
}
