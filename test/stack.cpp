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

    // If we're checking stack contents we don't care about the order.  This
    // utility tracks that all values are present.
    template<typename T>
    class Counter
    {
    public:
        void add(T val) { ++m_counts[val]; }

        bool sub(T val)
        {
            if (!m_counts.count(val)) return false;
            if (!--m_counts[val]) m_counts.erase(val);

            return true;
        }

        bool empty() const { return m_counts.empty(); }

    private:
        std::map<T, int> m_counts;
    };
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

TEST(Stack, PushNode)
{
    splicer::Stack<int> stack;

    const int value(4);

    splicer::Node<int> node;
    node.val() = value;

    EXPECT_TRUE(stack.empty());

    stack.push(&node);
    EXPECT_FALSE(stack.empty());
    EXPECT_EQ(stack.size(), 1);

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

    Counter<int> stackCounter;
    Counter<int> otherCounter;

    for (std::size_t i(0); i < values.size(); ++i)
    {
        if (i < total - 2) stackCounter.add(values[i]);
        else otherCounter.add(values[i]);
    }

    while (!other.empty())
    {
        ASSERT_TRUE(otherCounter.sub(other.pop()->val()));
    }

    while (!stack.empty())
    {
        ASSERT_TRUE(stackCounter.sub(stack.pop()->val()));
    }

    ASSERT_TRUE(stack.empty());
    ASSERT_TRUE(other.empty());

    ASSERT_TRUE(stackCounter.empty());
    ASSERT_TRUE(otherCounter.empty());
}

TEST(Stack, PopStackFull)
{
    // TODO
}

TEST(Stack, PopStackTooMany)
{
    // TODO
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

