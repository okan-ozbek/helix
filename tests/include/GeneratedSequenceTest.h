//
// Created by Okan Can Özbek on 21/11/2026.
//

#ifndef HELIX_GENERATEDSEQUENCETEST_H
#define HELIX_GENERATEDSEQUENCETEST_H

#include <format>
#include <random>

#include "Book.h"

using Book = Helix::Book;
using Order = Helix::Order;

struct Metadata
{
    std::size_t seed{};
    std::size_t iteration{};
    Helix::id_t id{};
};

struct MockOrder
{
    Helix::id_t id{};
    Helix::price_t price{};
    mutable Helix::quantity_t remaining_quantity{};
    Order::Side side{};
};

struct HitFrequency
{
    int reduce_frequency{ 5 };
    int delete_frequency{ 10 };
    int replace_frequency{ 15 };
    int invalidation_frequency{ 20};
};

class GeneratedSequenceTest
{
public:
    GeneratedSequenceTest() : seed{ 42 }
    {
        metadata = { seed, 0, 0 };
        Run();
    }

    explicit GeneratedSequenceTest(const std::size_t& seed) : seed{ seed }
    {
        metadata = { seed, 0, 0 };
        Run();
    }

    explicit GeneratedSequenceTest(const std::size_t& seed, const HitFrequency& hit_frequency)
        : seed{ seed }
        , hit_frequency{ hit_frequency }
    {
        metadata = { seed, 0, 0 };
        Run();
    }

private:
    std::size_t seed{};
    std::mt19937 random{ static_cast<unsigned int>(seed) };

    Book book{ Book{} };

    Metadata metadata{};
    HitFrequency hit_frequency{ 10, 20, 30 };
    Helix::id_t next_id{ 1 };

    std::vector<MockOrder> orders{};

    Helix::large_quantity_t expected_total_bid_quantity{ 0 };
    Helix::large_quantity_t expected_total_ask_quantity{ 0 };

    void Check(const bool condition, const std::string& message) const
    {
        if (!condition) {
            throw std::runtime_error(
                std::format(
                    "Check failed at seed {}, iteration {}, id {}: {}",
                    metadata.seed,
                    metadata.iteration,
                    metadata.id,
                    message
                )
            );
        }
    }

    void Run()
    {
        for (metadata.iteration = {}; metadata.iteration < book.OrderLimit(); ++metadata.iteration) {
            AddOrder();

            if (metadata.iteration % hit_frequency.reduce_frequency == 0) {
                ReduceOrder();
            }

            if (metadata.iteration % hit_frequency.delete_frequency == 0) {
                DeleteOrder();
            }

            if (metadata.iteration % hit_frequency.replace_frequency == 0) {
                ReplaceOrder();
            }

            if (metadata.iteration % hit_frequency.invalidation_frequency == 0) {
                InvalidOrder();
            }
        }
    }

    void AddOrder()
    {
        orders.emplace_back(
            ClaimNextId(),
            500,
            static_cast<Helix::quantity_t>(random() % 100 + 1),
            random() % 2 == 0
                ? Order::Side::Buy
                : Order::Side::Sell
        );

        metadata.id = { orders.back().id };

        const auto&[id, price, remaining_quantity, side]{ orders.back() };

        if (side == Order::Side::Buy) expected_total_bid_quantity += remaining_quantity;
        if (side == Order::Side::Sell) expected_total_ask_quantity += remaining_quantity;

        Check(
            book.Add({ id, price, remaining_quantity, side }) == Book::Status::Success,
            "Failed to add order"
        );
    }

    void ReduceOrder()
    {
        if (orders.empty()) {
            return;
        }

        const auto& [order_index, order]{ SelectRandomOrder() };
        const auto& [id, price, remaining_quantity, side]{ order };

        const auto selected_side { side };
        metadata.id = { id };

        std::uniform_int_distribution quantity_distribution{ 1, remaining_quantity };
        const auto& quantity_to_reduce{ quantity_distribution(random) };

        if (quantity_to_reduce == remaining_quantity) {
            Check(
                book.Reduce(id, quantity_to_reduce) == Book::Status::Success,
                "Failed to fully reduce order"
            );
            Check(
                !book.Find(id).has_value(),
                "Order should not exist after full reduction"
            );

            orders.erase(orders.begin() + order_index);
        }
        else {
            remaining_quantity -= quantity_to_reduce;

            Check(
                book.Reduce(id, quantity_to_reduce) == Book::Status::Success,
                "Failed to reduce order"
            );
            Check(
                book.Find(id)->RemainingQuantity() == remaining_quantity,
                "Remaining quantity should be updated after reduction"
            );
        }

        if (selected_side == Order::Side::Buy) expected_total_bid_quantity -= quantity_to_reduce;
        if (selected_side == Order::Side::Sell) expected_total_ask_quantity -= quantity_to_reduce;

        ValidateState();
    }

    void DeleteOrder()
    {
        if (orders.empty()) {
            return;
        }

        const auto& [order_index, order]{ SelectRandomOrder() };
        const auto [id, price, remaining_quantity, side]{ order };

        metadata.id = { id };

        Check(
            book.Delete(id) == Book::Status::Success,
            "Failed to delete order"
        );
        Check(
            !book.Find(id).has_value(),
            "Order should not exist after deletion"
        );

        if (side == Order::Side::Buy) expected_total_bid_quantity -= remaining_quantity;
        if (side == Order::Side::Sell) expected_total_ask_quantity -= remaining_quantity;

        orders.erase(orders.begin() + order_index);

        ValidateState();
    }

    void ReplaceOrder()
    {
        if (orders.empty()) {
            return;
        }

        const auto& [order_index, order]{ SelectRandomOrder() };
        const auto [id, price, remaining_quantity, side]{ order };

        metadata.id = { id };

        const Helix::id_t new_id{ ClaimNextId() };
        constexpr Helix::price_t new_price{ 500 };
        const Helix::quantity_t new_quantity{ static_cast<Helix::quantity_t>(random() % 100 + 1) };

        Check(
            book.Replace(id, new_id, new_price, new_quantity) == Book::Status::Success,
            "Failed to replace order"
        );
        Check(
            !book.Find(id).has_value(),
            "Old order should not exist after replacement"
        );
        Check(
            book.Find(new_id).has_value(),
            "New order should exist after replacement"
        );

        orders[order_index] = { new_id, new_price, new_quantity, side };

        if (side == Order::Side::Buy) {
            expected_total_bid_quantity -= remaining_quantity;
            expected_total_bid_quantity += new_quantity;
        }

        if (side == Order::Side::Sell) {
            expected_total_ask_quantity -= remaining_quantity;
            expected_total_ask_quantity += new_quantity;
        }

        ValidateState();
    }

    void InvalidOrder()
    {
        if (orders.empty()) {
            return;
        }

        const auto& [order_index, order]{ SelectRandomOrder() };
        const auto [id, price, remaining_quantity, side]{ order };

        metadata.id = { id };

        Check(
            book.Add({ id, price * 2, remaining_quantity * 2, (side == Order::Side::Buy ? Order::Side::Sell : Order::Side::Buy) }) == Book::Status::OrderAlreadyExists,
            "Adding an existing order should fail"
        );
        Check(
            book.Replace(id, id, price * 2, remaining_quantity * 2) == Book::Status::InvalidOperation,
            "Replacing an order with the same ID should fail"
        );
        Check(
            book.Reduce(id, remaining_quantity * 2) == Book::Status::InvalidOperation,
            "Reducing an order with excessive quantity should fail"
        );
        Check(
            book.Delete(id + book.OrderLimit() * 10) == Book::Status::OrderNotFound,
            "Deleting a non-existent order should fail"
        );

        ValidateExistingOrder(order, book.Find(id));
        ValidateState();
    }

    void ValidateState() const
    {
        Check(
            book.OrderCount() == orders.size(),
            "Order count mismatch"
        );

        const auto& best_bid{ book.BestBid() };
        const auto& best_ask{ book.BestAsk() };

        if (expected_total_bid_quantity == 0) {
            Check(
                !best_bid.has_value(),
                "Bid side should be empty"
            );
        }
        else {
            Check(
                best_bid.has_value(),
                "Best bid should exist"
            );
            Check(
                best_bid->price == 500,
                "Best bid price mismatch"
            );
            Check(
                best_bid->total_quantity == expected_total_bid_quantity,
                "Best bid total quantity mismatch"
            );
        }

        if (expected_total_ask_quantity == 0) {
            Check(
                !best_ask.has_value(),
                "Ask side should be empty"
            );
        }
        else {
            Check(
                best_ask.has_value(),
                "Best ask should exist"
            );
            Check(
                best_ask->price == 500,
                "Best ask price mismatch"
            );
            Check(
                best_ask->total_quantity == expected_total_ask_quantity,
                "Best ask total quantity mismatch"
            );
        }
    }

    void ValidateExistingOrder(const MockOrder& order, const std::optional<Order>& existing_order) const
    {
        Check(
            existing_order.has_value(),
            "Order should exist"
        );
        Check(
            existing_order->Id() == order.id,
            "Order ID mismatch"
        );
        Check(
            existing_order->Price() == order.price,
            "Order price mismatch"
        );
        Check(
            existing_order->RemainingQuantity() == order.remaining_quantity,
            "Order remaining quantity mismatch"
        );
        Check(
            existing_order->IsBuy() == (order.side == Order::Side::Buy),
            "Order side mismatch"
        );
    }

    void ValidateSetup() const
    {
        Check(
            book.OrderLimit() > 0,
            "Order limit should be greater than zero"
        );
        Check(
            book.OrderCount() == 0,
            "Order count should be zero at setup"
        );
        Check(
            !book.BestBid().has_value(),
            "Best bid should not exist at setup"
        );
        Check(
            !book.BestAsk().has_value(),
            "Best ask should not exist at setup"
        );
        Check(
            hit_frequency.reduce_frequency > 0,
            "Reduce frequency should be greater than zero"
        );
        Check(
            hit_frequency.delete_frequency > 0,
            "Delete frequency should be greater than zero"
        );
        Check(
            hit_frequency.replace_frequency > 0,
            "Replace frequency should be greater than zero"
        );
        Check(
            hit_frequency.invalidation_frequency > 0,
            "Invalidation frequency should be greater than zero"
        );
    }

    [[nodiscard]]
    std::pair<const int, const MockOrder&> SelectRandomOrder()
    {
        std::uniform_int_distribution order_distribution{ 0, static_cast<int>(orders.size() - 1) };
        const auto& order_index{ order_distribution(random) };
        const auto& order{ orders[order_index] };

        return { order_index, order };
    }

    [[nodiscard]]
    Helix::id_t ClaimNextId()
    {
        const auto current_id{ next_id };
        ++next_id;

        return current_id;
    }
};

#endif //HELIX_GENERATEDSEQUENCETEST_H
