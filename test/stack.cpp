#include <map>
#include <stack>

#include "splice-pool.hpp"
#include "gtest/gtest.h"

namespace
{
    const std::vector<int> values{3, 1, 4, 1, 5, 9};

    std::vector<splicer::Node<int>> makeNodes()
    {
        std::vector<splicer::Node<int>> nodes(values.size());

        for (std::size_t i(0); i < values.size(); ++i)
        {
            nodes[i].val() = values[i];
        }

        return nodes;
    }

    splicer::Stack<int> makeStack(std::vector<splicer::Node<int>>& nodes)
    {
        splicer::Stack<int> stack;

        for (std::size_t i(0); i < nodes.size(); ++i)
        {
            stack.push(&nodes[i]);
        }

        return stack;
    }
}

TEST(Stack, PopEmpty)
{
    splicer::Stack<int> stack;
    splicer::Node<int>* node(nullptr);

    EXPECT_TRUE(stack.empty());
    EXPECT_EQ(stack.size(), 0);
    EXPECT_NO_THROW(node = stack.pop());

    EXPECT_EQ(node, nullptr);
    EXPECT_TRUE(stack.empty());
    EXPECT_EQ(stack.size(), 0);
}

TEST(Stack, PushPopNode)
{
    splicer::Stack<int> stack;

    const int value(4);

    splicer::Node<int> node;
    node.val() = value;

    EXPECT_TRUE(stack.empty());

    stack.push(&node);
    EXPECT_FALSE(stack.empty());
    EXPECT_EQ(stack.size(), 1);

    splicer::Node<int>* popped(stack.pop());
    ASSERT_TRUE(popped);
    EXPECT_EQ(popped->val(), value);
    EXPECT_FALSE(popped->next());
    EXPECT_TRUE(stack.empty());
}

TEST(Stack, Swap)
{
    std::vector<splicer::Node<int>> nodes(makeNodes());
    splicer::Stack<int> stack(makeStack(nodes));

    splicer::Stack<int> other;

    EXPECT_EQ(stack.size(), values.size());
    EXPECT_FALSE(stack.empty());
    EXPECT_EQ(other.size(), 0);
    EXPECT_TRUE(other.empty());

    stack.swap(other);

    EXPECT_EQ(stack.size(), 0);
    EXPECT_TRUE(stack.empty());
    EXPECT_EQ(other.size(), values.size());
    EXPECT_FALSE(other.empty());

    for (std::size_t i(0); i < values.size(); ++i)
    {
        ASSERT_EQ(other.pop()->val(), values[values.size() - i - 1]);
    }

    ASSERT_EQ(other.size(), 0);
    ASSERT_TRUE(other.empty());
}

TEST(Stack, PushStack)
{
    splicer::Stack<int> stack;
    splicer::Stack<int> other;

    const int value(4);

    splicer::Node<int> node;
    node.val() = value;

    other.push(&node);
    EXPECT_FALSE(other.empty());
    EXPECT_EQ(other.size(), 1);

    EXPECT_TRUE(stack.empty());
    EXPECT_EQ(stack.size(), 0);

    stack.push(other);
    EXPECT_FALSE(stack.empty());
    EXPECT_EQ(stack.size(), 1);
    EXPECT_TRUE(other.empty());
    EXPECT_EQ(other.size(), 0);

    ASSERT_EQ(stack.pop()->val(), value);
    EXPECT_TRUE(stack.empty());
    EXPECT_TRUE(other.empty());
    EXPECT_EQ(stack.size(), 0);
    EXPECT_EQ(other.size(), 0);
}

TEST(Stack, PopStackEmpty)
{
    splicer::Stack<int> stack;
    splicer::Stack<int> other;

    EXPECT_NO_THROW(other = stack.popStack(1));
    EXPECT_TRUE(stack.empty());
    EXPECT_TRUE(other.empty());

    EXPECT_NO_THROW(other = stack.popStack(0));
    EXPECT_TRUE(stack.empty());
    EXPECT_TRUE(other.empty());
}

TEST(Stack, PopStackZero)
{
    std::vector<splicer::Node<int>> nodes(makeNodes());
    splicer::Stack<int> stack(makeStack(nodes));

    const std::size_t total(values.size());

    splicer::Stack<int> other(stack.popStack(0));

    ASSERT_EQ(stack.size(), total);
    ASSERT_FALSE(stack.empty());

    ASSERT_EQ(other.size(), 0);
    ASSERT_TRUE(other.empty());
}

TEST(Stack, PopStackPartial)
{
    std::vector<splicer::Node<int>> nodes(makeNodes());
    splicer::Stack<int> stack(makeStack(nodes));

    const std::size_t total(values.size());

    splicer::Stack<int> other(stack.popStack(2));

    EXPECT_EQ(stack.size(), total - 2);
    EXPECT_EQ(other.size(), 2);

    std::size_t i(total);

    while (--i < total)
    {
        const auto check(values[i]);

        if (i >= 4) ASSERT_EQ(other.pop()->val(), check);
        else ASSERT_EQ(stack.pop()->val(), check);
    }

    ASSERT_TRUE(stack.empty());
    ASSERT_TRUE(other.empty());
}

TEST(Stack, PopStackFull)
{
    std::vector<splicer::Node<int>> nodes(makeNodes());
    splicer::Stack<int> stack(makeStack(nodes));

    const std::size_t total(values.size());

    ASSERT_EQ(stack.size(), total);

    splicer::Stack<int> other(stack.popStack(total));

    EXPECT_EQ(stack.size(), 0);
    EXPECT_TRUE(stack.empty());
    EXPECT_EQ(other.size(), total);
    EXPECT_FALSE(other.empty());

    for (std::size_t i(total - 1); i < total; --i)
    {
        ASSERT_EQ(other.pop()->val(), values[i]);
    }

    EXPECT_TRUE(other.empty());
}

TEST(Stack, PopStackTooMany)
{
    std::vector<splicer::Node<int>> nodes(makeNodes());
    splicer::Stack<int> stack(makeStack(nodes));

    const std::size_t total(values.size());

    ASSERT_EQ(stack.size(), total);

    splicer::Stack<int> other(stack.popStack(total * 2));

    EXPECT_EQ(stack.size(), 0);
    EXPECT_TRUE(stack.empty());
    EXPECT_EQ(other.size(), total);
    EXPECT_FALSE(other.empty());

    for (std::size_t i(total - 1); i < total; --i)
    {
        ASSERT_EQ(other.pop()->val(), values[i]);
    }

    EXPECT_TRUE(other.empty());
}

TEST(Stack, PushPopSingle)
{
    std::stack<int> validator;
    splicer::Stack<int> stack;
    std::vector<splicer::Node<int>> nodes(makeNodes());

    auto doPush([&](std::size_t i)->void
    {
        const int value(values.at(i));
        splicer::Node<int>& node(nodes.at(i));

        ASSERT_EQ(value, node.val());

        validator.push(value);
        stack.push(&node);

        ASSERT_FALSE(validator.empty());
        ASSERT_FALSE(stack.empty());
        ASSERT_EQ(stack.size(), i + 1);
        ASSERT_EQ(stack.size(), validator.size());
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

TEST(Stack, PushPopOtherStack)
{
    std::stack<int> validator;
    splicer::Stack<int> stack;
    std::vector<splicer::Node<int>> nodes(makeNodes());

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

