#include <algorithm>
#include <format>

#include "Book.h"

#include <iostream>
#include <random>
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

        Check(
            !book.BestBid().has_value(),
            "Best bid should not exist"
        );
        Check(
            !book.BestAsk().has_value(),
            "Best ask should not exist"
        );
    }

    // One bid, One ask
    {
        auto book{ Book{} };

        Check(
            book.Add(Order{ 1, 100, 10, Order::Side::Buy }) == Book::Status::Success,
            "Failed to add bid order"
        );
        Check(
            book.Add(Order{ 2, 110, 20, Order::Side::Sell }) == Book::Status::Success,
            "Failed to add ask order"
        );

        check_best(book.BestBid(), Order::Side::Buy, 100, 10);
        check_best(book.BestAsk(), Order::Side::Sell, 110, 20);
    }

    // Two orders at one price level
    {
        auto book{ Book{} };

        Check(
            book.Add(Order{ 1, 100, 10, Order::Side::Buy }) == Book::Status::Success,
            "Failed to add order"
        );
        Check(
            book.Add(Order{ 2, 100, 20, Order::Side::Buy }) == Book::Status::Success,
            "Failed to add another order at same price level"
        );
        check_best(book.BestBid(), Order::Side::Buy, 100, 30);
    }

    // Partial reduction
    {
        auto book{ Book{} };

        Check(
            book.Add(Order{ 1, 100, 10, Order::Side::Buy }) == Book::Status::Success,
            "Failed to add order"
        );
        Check(
            book.Reduce(1, 5) == Book::Status::Success,
            "Failed to reduce order"
        );
        check_best(book.BestBid(), Order::Side::Buy, 100, 5);
    }

    // Full reduction
    {
        auto book{ Book{} };

        Check(
            book.Add(Order{ 1, 100, 10, Order::Side::Buy }) == Book::Status::Success,
            "Failed to add order"
        );
        Check(
            book.Add(Order{ 2, 100, 20, Order::Side::Buy }) == Book::Status::Success,
            "Failed to add another order at same price level"
        );
        check_best(book.BestBid(), Order::Side::Buy, 100, 30);

        Check(
            book.Reduce(1, 10) == Book::Status::Success,
            "Failed to fully reduce order"
        );
        check_best(book.BestBid(), Order::Side::Buy, 100, 20);

        Check(
            book.Delete(1) == Book::Status::OrderNotFound,
            "Failed to detect non-existent order after full reduction"
        );
    }

    // Delete current best
    {
        auto book{ Book{} };

        Check(
            book.Add(Order{ 1, 100, 10, Order::Side::Buy }) == Book::Status::Success,
            "Failed to add order"
        );
        Check(
            book.Add(Order{ 2, 90, 20, Order::Side::Buy }) == Book::Status::Success,
            "Failed to add another order"
        );
        check_best(book.BestBid(), Order::Side::Buy, 100, 10);

        Check(
            book.Delete(1) == Book::Status::Success,
            "Failed to delete order"
        );
        check_best(book.BestBid(), Order::Side::Buy, 90, 20);
    }

    // Empty side
    {
        auto book{ Book{} };

        Check(
            book.Add(Order{ 1, 100, 10, Order::Side::Buy }) == Book::Status::Success,
            "Failed to add order"
        );
        Check(
            book.BestBid().has_value(),
            "Best bid should not be empty"
        );
        Check(
            !book.BestAsk().has_value(),
            "Best ask should be empty"
        );
    }

    // Replace order
    {
        auto book{ Book{} };

        Check(
            book.Add(Order{ 1, 100, 10, Order::Side::Buy }) == Book::Status::Success,
            "Failed to add order"
        );
        Check(
            book.Replace(1, 2, 200, 20) == Book::Status::Success,
            "Failed to replace order"
        );
    }

    // Full-capacity replacement
    {
        auto book{ Book{ 1 } };

        Check(
            book.Add(Order{ 1, 100, 10, Order::Side::Buy }) == Book::Status::Success,
            "Failed to add order"
        );
        Check(
            book.Replace(1, 2, 200, 20) == Book::Status::Success,
            "Failed to replace order"
        );

        Check(
            book.Add(Order{ 3, 300, 30, Order::Side::Buy }) == Book::Status::CapacityReached,
            "Failed to reach capacity after replacement"
        );
        Check(
            book.OrderLimit() == 1,
            "Order limit should be 1"
        );

        check_best(book.BestBid(), Order::Side::Buy, 200, 20);
        Check(
            book.OrderCount() == 1,
            "Order count should be 1 after replacement"
        );

        Check(
            book.Delete(1) == Book::Status::OrderNotFound,
            "Failed to detect non-existent order after replacement"
        );
        Check(
            book.Delete(2) == Book::Status::Success,
            "Failed to delete replaced) order"
        );
    }

    // Reduce by negative
    {
        auto book{ Book{} };

        Check(
            book.Add(Order{ 1, 100, 10, Order::Side::Buy }) == Book::Status::Success,
            "Failed to add order"
        );
        Check(
            book.Reduce(1, -5) == Book::Status::InvalidOperation,
            "Failed to detect negative reduction"
        );
        check_best(book.BestBid(), Order::Side::Buy, 100, 10);
    }

    // Duplicate/Unknown ID
    {
        auto book{ Book{} };

        Check(
            book.Add(Order{ 1, 100, 10, Order::Side::Buy }) == Book::Status::Success,
            "Failed to add order"
        );
        Check(
            book.Add(Order{ 1, 100, 10, Order::Side::Buy }) == Book::Status::OrderAlreadyExists,
            "Failed to detect duplicate order"
        );
    }

    // Replace duplicate/unknown ID
    {
        auto book{ Book{} };

        Check(
            book.Add(Order{ 1, 100, 10, Order::Side::Buy }) == Book::Status::Success,
            "Failed to add order"
        );
        Check(
            book.Add(Order{ 2, 200, 20, Order::Side::Buy }) == Book::Status::Success,
            "Failed to add order"
        );

        Check(
            book.Replace(1, 2, 200, 20) == Book::Status::OrderAlreadyExists,
            "Failed to detect duplicate replacement ID"
        );
        Check(
            book.Replace(3, 4, 200, 20) == Book::Status::OrderNotFound,
            "Failed to detect unknown replacement ID"
        );

        check_best(book.BestBid(), Order::Side::Buy, 200, 20);
        Check(
            book.Delete(2) == Book::Status::Success,
            "Failed to delete order 2"
        );
        check_best(book.BestBid(), Order::Side::Buy, 100, 10);
    }

    // Zero/excessive quantity or bad side/price
    {
        auto book{ Book{} };

        Check(
            book.Add(Order{ 0, 100, 10, Order::Side::Buy }) == Book::Status::InvalidOperation,
            "Failed to detect invalid ID"
        );
        Check(
            book.Add(Order{ 1, 0, 10, Order::Side::Buy }) == Book::Status::InvalidOperation,
            "Failed to detect invalid price"
        );
        Check(
            book.Add(Order{ 1, 100, 0, Order::Side::Buy }) == Book::Status::InvalidOperation,
            "Failed to detect zero quantity"
        );
    }

    // Capacity reached
    {
        auto book{ Book{ 3 } };

        for (auto i{ 0uz }; i < book.OrderLimit(); ++i) {
            Check(
                book.Add(Order{ i + 1, 100, 10, Order::Side::Buy }) == Book::Status::Success,
                "Failed to add order at capacity"
            );
        }

        Check(
            book.Add(Order{ book.OrderLimit() + 1, 100, 10, Order::Side::Buy }) == Book::Status::CapacityReached,
            "Failed to reach capacity"
        );
    }
}

constexpr void GeneratedSequenceTest(const std::size_t& seed)
{
    using Helix::Book;
    using Helix::Order;
    using std::string_literals::operator ""s;

    auto book{ Book{} };

    std::mt19937_64 random{ seed };

    Helix::large_quantity_t expected_total_bid_quantity{ 0 };
    Helix::large_quantity_t expected_total_ask_quantity{ 0 };

    struct MockOrder
    {
        Helix::id_t id{};
        Helix::price_t price{};
        mutable Helix::quantity_t remaining_quantity{};
        Order::Side side{};
    };

    std::vector<MockOrder> orders{};

    // Add orders to the book
    for (auto i{ 0uz }; i < book.OrderLimit(); ++i) {
        orders.emplace_back(
            i + 1,
            500,
            static_cast<Helix::quantity_t>(random() % 100 + 1),
            random() % 2 == 0
                ? Order::Side::Buy
                : Order::Side::Sell
        );

        const auto&[id, price, remaining_quantity, side]{ orders.back() };

        // Update expected total quantities
        if (side == Order::Side::Buy) {
            expected_total_bid_quantity += remaining_quantity;
        }

        if (side == Order::Side::Sell) {
            expected_total_ask_quantity += remaining_quantity;
        }

        Check(
            book.Add({ id, price, remaining_quantity, side }) == Book::Status::Success,
            "Failed to add order"
        );

        if (i % 10 == 0) {
            // Randomly select an order to reduce
            std::uniform_int_distribution order_distribution{ 0, static_cast<int>(orders.size() - 1) };
            const auto& order_index{ order_distribution(random) };
            const auto& target_order{ orders[order_index] };

            // Randomly reduce the order by a quantity between 1 and the remaining quantity
            std::uniform_int_distribution quantity_distribution{ 1, target_order.remaining_quantity };
            const auto& quantity_to_reduce{ quantity_distribution(random) };

            // Copy of selected side, in-case we delete the target order in a full reduction scenario, we still have the side to validate best bid/ask
            const auto selected_side{ target_order.side };

            // Full reduction
            if (quantity_to_reduce == target_order.remaining_quantity) {
                const auto count{ book.OrderCount() };

                Check(
                    book.Reduce(target_order.id, quantity_to_reduce) == Book::Status::Success,
                    "Failed to fully reduce order"
                );
                Check(
                    book.OrderCount() == count - 1,
                    "Order count should decrease after full reduction"
                );

                Check(
                    !book.Find(target_order.id).has_value(),
                    "Order should not exist after full reduction"
                );

                orders.erase(orders.begin() + order_index);
            }
            // Partial reduction
            else {
                Check(
                    book.Reduce(target_order.id, quantity_to_reduce) == Book::Status::Success,
                    "Failed to reduce order"
                );

                // Update of the remaining quantity in the mock order
                target_order.remaining_quantity -= quantity_to_reduce;

                Check(
                    book.Find(target_order.id)->RemainingQuantity() == target_order.remaining_quantity,
                    "Remaining quantity should be updated after reduction"
                );
            }

            // Validate best bid/ask after reduction
            const auto validate_expected_quantity{
                [&](auto& expected_quantity, const auto& best) -> void {
                    const auto string{
                        selected_side == Order::Side::Buy
                            ? "bid"s
                            : "ask"s
                    };

                    expected_quantity -= quantity_to_reduce;

                    if (expected_quantity == 0) {
                        Check(
                            !best.has_value(),
                            std::format("Best {} should not exist after full reduction", string)
                        );
                    }
                    else {
                        Check(
                            best.has_value(),
                            std::format("Best {} should exist", string)
                        );
                        Check(
                            best->total_quantity == expected_quantity,
                            std::format("Best {} total quantity should be equal to the expected total {} quantity", string, string)
                        );
                    }
                }
            };

            selected_side == Order::Side::Buy
                ? validate_expected_quantity(expected_total_bid_quantity, book.BestBid())
                : validate_expected_quantity(expected_total_ask_quantity, book.BestAsk());
        }
    }
}

int main()
{
    try {
        BookTest();
        GeneratedSequenceTest(64);
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
