#include "splice-pool.hpp"
#include "gtest/gtest.h"

namespace
{
    const std::size_t blockSize(20);
}

TEST(ObjectPool, CopyConstruct)
{
    splicer::ObjectPool<int> pool(blockSize);
    splicer::Node<int>* node(0);

    EXPECT_EQ(pool.allocated(), pool.available());

    ASSERT_TRUE(node = pool.acquire());

    EXPECT_GE(pool.allocated(), blockSize);
    EXPECT_EQ(pool.available(), pool.allocated() - 1);

    node->val() = 4;
    EXPECT_EQ(node->val(), 4);

    EXPECT_NO_THROW(pool.release(node));
}

TEST(ObjectPool, ForwardConstruct)
{
    splicer::ObjectPool<int> pool(blockSize);
    splicer::Node<int>* node(0);

    EXPECT_EQ(pool.allocated(), pool.available());

    ASSERT_TRUE(node = pool.acquire(42));
    EXPECT_EQ(node->val(), 42);

    EXPECT_GE(pool.allocated(), blockSize);
    EXPECT_EQ(pool.available(), pool.allocated() - 1);

    EXPECT_NO_THROW(pool.release(node));
}

TEST(ObjectPool, MultipleAlloc)
{
    splicer::ObjectPool<int> pool(blockSize);
    std::vector<splicer::Node<int>*> nodes;

    for (std::size_t i(0); i < blockSize * 2; ++i)
    {
        nodes.push_back(pool.acquire(i));
    }

    ASSERT_GE(pool.allocated(), blockSize * 2);
    ASSERT_EQ(pool.available(), pool.allocated() - blockSize * 2);

    ASSERT_EQ(nodes.size(), blockSize * 2);

    for (std::size_t i(0); i < nodes.size(); ++i)
    {
        ASSERT_EQ(nodes[i]->val(), i);
    }

    while (nodes.size())
    {
        ASSERT_NO_THROW(pool.release(nodes.back()));
        nodes.pop_back();
    }

    ASSERT_GE(pool.allocated(), blockSize * 2);
    ASSERT_EQ(pool.available(), pool.allocated());
}

TEST(ObjectPool, ReleaseStack)
{
    splicer::ObjectPool<int> pool(blockSize);
    splicer::Stack<int> stack;
    splicer::Stack<int> other;

    splicer::Node<int>* node(nullptr);

    for (std::size_t i(0); i < blockSize * 2; ++i)
    {
        node = pool.acquire(i);

        ASSERT_TRUE(node);
        stack.push(node);

        node = nullptr;
    }

    EXPECT_GE(pool.allocated(), blockSize * 2);
    EXPECT_EQ(pool.available(), pool.allocated() - blockSize * 2);

    ASSERT_EQ(stack.size(), blockSize * 2);
    std::size_t i(blockSize * 2);

    while (!stack.empty())
    {
        node = stack.pop();

        ASSERT_TRUE(node);
        ASSERT_EQ(node->val(), --i);

        other.push(node);
        node = nullptr;
    }

    EXPECT_TRUE(stack.empty());
    EXPECT_EQ(stack.size(), 0);

    EXPECT_FALSE(other.empty());
    EXPECT_EQ(other.size(), blockSize * 2);

    EXPECT_NO_THROW(pool.release(other));

    EXPECT_GE(pool.allocated(), blockSize * 2);
    EXPECT_EQ(pool.available(), pool.allocated());
}

TEST(ObjectPool, AcquireStackFromEmpty)
{
    splicer::ObjectPool<int> pool(blockSize);
    splicer::Stack<int> stack(pool.acquireStack(blockSize * 2));

    EXPECT_EQ(stack.size(), blockSize * 2);
    EXPECT_GE(pool.allocated(), blockSize * 2);
    EXPECT_EQ(pool.available(), pool.allocated() - blockSize * 2);
}

TEST(ObjectPool, AcquireStackFromPopulated)
{
    splicer::ObjectPool<int> pool(blockSize);
    splicer::Stack<int> stack(pool.acquireStack(blockSize * 2));
    pool.release(stack);

    ASSERT_EQ(pool.allocated(), pool.available());
    ASSERT_GE(pool.available(), blockSize * 2);

    const std::size_t size(pool.available());
    stack = pool.acquireStack(size - 1);

    EXPECT_EQ(stack.size(), size - 1);
    EXPECT_FALSE(stack.empty());
    EXPECT_EQ(pool.available(), 1);
    EXPECT_EQ(pool.allocated(), size);

    pool.release(stack);

    EXPECT_EQ(stack.size(), 0);
    EXPECT_TRUE(stack.empty());
    EXPECT_EQ(pool.available(), size);
    EXPECT_EQ(pool.allocated(), size);
}

