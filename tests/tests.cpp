#include "Book.h"

#include <iostream>

void Check(const bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << message << '\n';
    }
}

void BookTest()
{
    using Helix::Book;
    using Helix::Order;

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

        Check(book.BestBid().has_value(), "Best bid should exist");
        Check(book.BestBid()->price == 100, "Best bid price should be 100");
        Check(book.BestBid()->total_quantity == 10, "Best bid total quantity should be 10");

        Check(book.BestAsk().has_value(), "Best ask should exist");
        Check(book.BestAsk()->price == 110, "Best ask price should be 110");
        Check(book.BestAsk()->total_quantity == 20, "Best ask total quantity should be 20");
    }

    // Two orders at one price level
    {
        auto book{ Book{} };

        const auto status{ book.Add(Order{ 1, 100, 10, Order::Side::Buy }) };
        Check(status == Book::Status::Success, "Failed to add order");

        const auto status_add_another{ book.Add(Order{ 2, 100, 20, Order::Side::Buy }) };
        Check(status_add_another == Book::Status::Success, "Failed to add another order at same price level");
        Check(book.BestBid().has_value(), "Best bid should exist");
        Check(book.BestBid()->price == 100, "Best bid price should be 100");
        Check(book.BestBid()->total_quantity == 30, "Best bid total quantity should be 30");
    }

    // Partial reduction
    {
        auto book{ Book{} };

        const auto status{ book.Add(Order{ 1, 100, 10, Order::Side::Buy }) };
        Check(status == Book::Status::Success, "Failed to add order");

        const auto status_reduce{ book.Reduce(1, 5) };
        Check(status_reduce == Book::Status::Success, "Failed to reduce order");
        Check(book.BestBid().has_value(), "Best bid should exist after reduction");
        Check(book.BestBid()->price == 100, "Best bid price should be 100 after reduction");
        Check(book.BestBid()->total_quantity == 5, "Best bid total quantity should be 5 after reduction");
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
        Check(book.BestBid().has_value(), "Best bid should exist");
        Check(book.BestBid()->price == 100, "Best bid price should be 100");
        Check(book.BestBid()->total_quantity == 10, "Best bid total quantity should be 30");

        const auto status_delete{ book.Delete(1) };
        Check(status_delete == Book::Status::Success, "Failed to delete order");
        Check(book.BestBid().has_value(), "Best bid should exist after deletion");
        Check(book.BestBid()->price == 90, "Best bid price should be 90 after deletion");
        Check(book.BestBid()->total_quantity == 20, "Best bid total quantity should be 20 after deletion");
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
        auto book{ Book{} };

        const auto status{ book.Add(Order{ 1, 100, 10, Order::Side::Buy }) };
        Check(status == Book::Status::Success, "Failed to add order");

        const auto status_replace{ book.Replace(1, 2, 200, 20) };
        Check(status_replace == Book::Status::Success, "Failed to replace order");
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
        auto book{ Book{} };

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
