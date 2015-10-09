#include <stack>

#include "splice-pool.hpp"
#include "gtest/gtest.h"

TEST(Stack, PushNode)
{
    splicer::Stack<int> stack;

    const int value(4);

    splicer::Node<int> node;
    node.val() = value;

    EXPECT_TRUE(stack.empty());

    stack.push(&node);
    EXPECT_FALSE(stack.empty());

    EXPECT_EQ(stack.pop()->val(), value);
    EXPECT_TRUE(stack.empty());
}

TEST(Stack, PushStack)
{
    splicer::Stack<int> stack;
    splicer::Stack<int> other;

    const int value(4);

    splicer::Node<int> node;
    node.val() = value;

    other.push(&node);

    EXPECT_TRUE(stack.empty());
    EXPECT_FALSE(other.empty());

    stack.push(other);
    EXPECT_FALSE(stack.empty());
    EXPECT_TRUE(other.empty());

    ASSERT_EQ(stack.pop()->val(), value);
    EXPECT_TRUE(stack.empty());
    EXPECT_TRUE(other.empty());
}

TEST(Stack, ValidatePushPopSingle)
{
    std::stack<int> validator;
    splicer::Stack<int> stack;

    const std::vector<int> values{3, 1, 4, 1, 5, 9};
    std::vector<splicer::Node<int>> nodes(values.size());
    for (std::size_t i(0); i < values.size(); ++i) nodes[i].val() = values[i];

    auto doPush([&](std::size_t i)->void
    {
        const int value(values.at(i));
        splicer::Node<int>& node(nodes.at(i));

        ASSERT_EQ(value, node.val());

        validator.push(value);
        stack.push(&node);

        ASSERT_FALSE(validator.empty());
        ASSERT_FALSE(stack.empty());
    });

    auto doPop([&](std::size_t i)->void
    {
        const int value(values.at(i));

        ASSERT_EQ(value, validator.top());
        validator.pop();

        ASSERT_EQ(value, stack.pop()->val());
        ASSERT_EQ(validator.empty(), stack.empty());
    });

    ASSERT_TRUE(validator.empty());
    ASSERT_TRUE(stack.empty());

    for (std::size_t i(0); i < values.size(); ++i) doPush(i);

    ASSERT_FALSE(validator.empty());
    ASSERT_FALSE(stack.empty());

    for (std::size_t i(values.size() - 1); i < values.size(); --i) doPop(i);

    ASSERT_TRUE(validator.empty());
    ASSERT_TRUE(stack.empty());
}

TEST(Stack, ValidatePushPopOtherStack)
{
    std::stack<int> validator;
    splicer::Stack<int> stack;

    const std::vector<int> values{3, 1, 4, 1, 5, 9};
    std::vector<splicer::Node<int>> nodes(values.size());
    for (std::size_t i(0); i < values.size(); ++i) nodes[i].val() = values[i];

    auto doPush([&]()->void
    {
        splicer::Stack<int> other;

        for (auto& node : nodes)
        {
            other.push(&node);
            validator.push(node.val());
        }

        ASSERT_FALSE(other.empty());
        ASSERT_TRUE(stack.empty());

        stack.push(other);

        ASSERT_TRUE(other.empty());
        ASSERT_FALSE(stack.empty());

        ASSERT_FALSE(validator.empty());
    });

    auto doPop([&](std::size_t i)->void
    {
        const int value(values.at(i));

        ASSERT_EQ(value, validator.top());
        validator.pop();

        ASSERT_EQ(value, stack.pop()->val());
        ASSERT_EQ(validator.empty(), stack.empty());
    });

    ASSERT_TRUE(validator.empty());
    ASSERT_TRUE(stack.empty());

    doPush();

    ASSERT_FALSE(validator.empty());
    ASSERT_FALSE(stack.empty());

    for (std::size_t i(values.size() - 1); i < values.size(); --i) doPop(i);

    ASSERT_TRUE(validator.empty());
    ASSERT_TRUE(stack.empty());
}

