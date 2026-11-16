#include <format>
#include <functional>

#include "Book.h"

#include <iostream>
#include <string>

void Check(const bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void BookTest()
{
    using Helix::Book;
    using Helix::Order;



    const auto check_best{
        [&](const auto& result, const auto& side, const auto& expected_price, const auto& expected_quantity) -> void {
            using std::string_literals::operator ""s;

            const auto side_str{
                static_cast<Order::Side>(side) == Order::Side::Buy
                    ? "bid"s
                    : "ask"s
            };

            Check(
                result.has_value(),
                "Best should exist after replacement"
            );

            Check(
                result->price == expected_price,
                std::format("Best {} price should be {} after replacement, the actual price is {}", side_str, expected_price, result->price)
            );

            Check(
                result->total_quantity == expected_quantity,
                std::format("Best {} total quantity should be {} after replacement, the actual quantity is {}", side_str, expected_quantity, result->total_quantity)
            );
        }
    };

    // Empty book
    {
        auto book{ Book{} };

        Check(!book.BestBid().has_value(), "Best bid should not exist");
        Check(!book.BestAsk().has_value(), "Best ask should not exist");
    }

    // One bid, One ask
    {
        auto book{ Book{} };

        const auto status_bid{ book.Add(Order{ 1, 100, 10, Order::Side::Buy }) };
        Check(status_bid == Book::Status::Success, "Failed to add bid order");

        const auto status_ask{ book.Add(Order{ 2, 110, 20, Order::Side::Sell }) };
        Check(status_ask == Book::Status::Success, "Failed to add ask order");

        check_best(book.BestBid(), Order::Side::Buy, 100, 10);
        check_best(book.BestAsk(), Order::Side::Sell, 110, 20);
    }

    // Two orders at one price level
    {
        auto book{ Book{} };

        const auto status{ book.Add(Order{ 1, 100, 10, Order::Side::Buy }) };
        Check(status == Book::Status::Success, "Failed to add order");

        const auto status_add_another{ book.Add(Order{ 2, 100, 20, Order::Side::Buy }) };
        Check(status_add_another == Book::Status::Success, "Failed to add another order at same price level");
        check_best(book.BestBid(), Order::Side::Buy, 100, 30);
    }

    // Partial reduction
    {
        auto book{ Book{} };

        const auto status{ book.Add(Order{ 1, 100, 10, Order::Side::Buy }) };
        Check(status == Book::Status::Success, "Failed to add order");

        const auto status_reduce{ book.Reduce(1, 5) };
        Check(status_reduce == Book::Status::Success, "Failed to reduce order");
        check_best(book.BestBid(), Order::Side::Buy, 100, 5);
    }

    // Full reduction
    {
        auto book{ Book{} };

        const auto status{ book.Add(Order{ 1, 100, 10, Order::Side::Buy }) };
        Check(status == Book::Status::Success, "Failed to add order");

        const auto status_reduce_full{ book.Reduce(1, 10) };
        Check(status_reduce_full == Book::Status::Success, "Failed to fully reduce order");
        Check(!book.BestBid().has_value(), "Best bid should not exist after full reduction");
    }

    // Delete current best
    {
        auto book{ Book{} };

        const auto status{ book.Add(Order{ 1, 100, 10, Order::Side::Buy }) };
        Check(status == Book::Status::Success, "Failed to add order");

        const auto status_add_another{ book.Add(Order{ 2, 90, 20, Order::Side::Buy }) };
        Check(status_add_another == Book::Status::Success, "Failed to add another order");
        check_best(book.BestBid(), Order::Side::Buy, 100, 10);

        const auto status_delete{ book.Delete(1) };
        Check(status_delete == Book::Status::Success, "Failed to delete order");
        check_best(book.BestBid(), Order::Side::Buy, 90, 20);
    }

    // Empty side
    {
        auto book{ Book{} };

        const auto status{ book.Add(Order{ 1, 100, 10, Order::Side::Buy }) };
        Check(status == Book::Status::Success, "Failed to add order");
        Check(book.BestBid().has_value(), "Best bid should not be empty");
        Check(!book.BestAsk().has_value(), "Best ask should be empty");
    }

    // Replace order
    {
        auto book{ Book{} };

        const auto status{ book.Add(Order{ 1, 100, 10, Order::Side::Buy }) };
        Check(status == Book::Status::Success, "Failed to add order");

        const auto status_replace{ book.Replace(1, 2, 200, 20) };
        Check(status_replace == Book::Status::Success, "Failed to replace order");
    }

    // Full-capacity replacement
    {
        auto book{ Book{ 1 } };

        const auto status{ book.Add(Order{ 1, 100, 10, Order::Side::Buy }) };
        Check(status == Book::Status::Success, "Failed to add order");

        const auto status_replace{ book.Replace(1, 2, 200, 20) };
        Check(status_replace == Book::Status::Success, "Failed to replace order");

        const auto status_add_after_replace{ book.Add(Order{ 3, 300, 30, Order::Side::Buy }) };
        Check(status_add_after_replace == Book::Status::CapacityReached, "Failed to reach capacity after replacement");
        Check(book.OrderLimit() == 1, "Order limit should be 1");

        check_best(book.BestBid(), Order::Side::Buy, 200, 20);
        Check(book.OrderCount() == 1, "Order count should be 1 after replacement");

        Check(book.Delete(1) == Book::Status::OrderNotFound, "Failed to detect non-existent order after replacement");
        Check(book.Delete(2) == Book::Status::Success, "Failed to delete replaced) order");
    }

    // Reduce by negative
    {
        auto book{ Book{} };

        const auto status{ book.Add(Order{ 1, 100, 10, Order::Side::Buy }) };
        Check(status == Book::Status::Success, "Failed to add order");

        const auto status_reduce_negative{ book.Reduce(1, -5) };
        Check(status_reduce_negative == Book::Status::InvalidOperation, "Failed to detect negative reduction");
        check_best(book.BestBid(), Order::Side::Buy, 100, 10);
    }

    // Duplicate/Unknown ID
    {
        auto book{ Book{} };

        const auto status{ book.Add(Order{ 1, 100, 10, Order::Side::Buy }) };
        Check(status == Book::Status::Success, "Failed to add order");

        const auto status_duplicate{ book.Add(Order{ 1, 100, 10, Order::Side::Buy }) };
        Check(status_duplicate == Book::Status::OrderAlreadyExists, "Failed to detect duplicate order");
    }

    // Zero/excessive quantity or bad side/price
    {
        auto book{ Book{} };

        const auto status_invalid_id{ book.Add(Order{ 0, 100, 10, Order::Side::Buy }) };
        Check(status_invalid_id == Book::Status::InvalidOperation, "Failed to detect invalid ID");

        const auto status_invalid_price{ book.Add(Order{ 1, 0, 10, Order::Side::Buy }) };
        Check(status_invalid_price == Book::Status::InvalidOperation, "Failed to detect invalid price");

        const auto status_invalid_quantity{ book.Add(Order{ 1, 100, 0, Order::Side::Buy }) };
        Check(status_invalid_quantity == Book::Status::InvalidOperation, "Failed to detect zero quantity");
    }

    // Capacity reached
    {
        auto book{ Book{ 3 } };

        for (auto i{ 0uz }; i < book.OrderLimit(); ++i) {
            const auto status{ book.Add(Order{ i + 1, 100, 10, Order::Side::Buy }) };
            Check(status == Book::Status::Success, "Failed to add order at capacity");
        }

        const auto status{ book.Add(Order{ book.OrderLimit() + 1, 100, 10, Order::Side::Buy }) };
        Check(status == Book::Status::CapacityReached, "Failed to reach capacity");
    }
}

int main()
{
    try {
        BookTest();
    }
    catch (const std::exception& error) {
        std::cerr << "Exception: " << error.what() << '\n';
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown exception occurred\n";
        return 1;
    }

    return 0;
}
