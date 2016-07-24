#include "splice-pool.hpp"
#include "gtest/gtest.h"

namespace
{
    const std::size_t blockSize(20);
}

TEST(UniqueSemantics, AutoReleaseNode)
{
    splicer::ObjectPool<int> pool(blockSize);
    EXPECT_EQ(pool.allocated(), pool.available());

    {
        splicer::ObjectPool<int>::UniqueNodeType node(pool.acquireOne(42));
        ASSERT_TRUE(node.get());
        EXPECT_EQ(node.get()->val(), 42);
        EXPECT_EQ(*node, 42);  // Equivalent to above.

        *node = 271828;

        EXPECT_EQ(node.get()->val(), 271828);
        EXPECT_EQ(*node, 271828);

        EXPECT_GE(pool.allocated(), blockSize);
        EXPECT_EQ(pool.available(), pool.allocated() - 1);

        node.reset();
        EXPECT_EQ(pool.available(), pool.allocated());
        ASSERT_FALSE(node.get());

        node = pool.acquireOne(314);

        ASSERT_TRUE(node.get());
        EXPECT_EQ(*node, 314);

        EXPECT_GE(pool.allocated(), blockSize);
        EXPECT_EQ(pool.available(), pool.allocated() - 1);
    }

    EXPECT_EQ(pool.available(), pool.allocated());
}

TEST(UniqueSemantics, AutoReleaseStack)
{
    splicer::ObjectPool<int> pool(blockSize);

    std::size_t count(blockSize * 2 + 1);

    {
        splicer::ObjectPool<int>::UniqueStackType stack(pool.acquire(count));

        ASSERT_EQ(stack.size(), count);
        EXPECT_EQ(pool.available(), pool.allocated() - count);

        // Pop a Node from the UniqueStack and release it.
        splicer::ObjectPool<int>::UniqueNodeType node(stack.popOne());
        EXPECT_EQ(stack.size(), --count);
        EXPECT_TRUE(node.get());

        node.reset();

        EXPECT_FALSE(node.get());
        EXPECT_EQ(pool.available(), pool.allocated() - count);

        // Pop a UniqueStack from the first UniqueStack and release it.
        ASSERT_GT(count, 4);
        splicer::ObjectPool<int>::UniqueStackType other(stack.pop(4));

        count -= 4;

        EXPECT_EQ(stack.size(), count);
        EXPECT_EQ(other.size(), 4);

        other.reset();

        EXPECT_EQ(pool.available(), pool.allocated() - count);
    }

    EXPECT_EQ(pool.available(), pool.allocated());
}

TEST(UniqueSemantics, Combinations)
{
    splicer::ObjectPool<int> pool(blockSize);

    // Merge separately acquired UniqueStack instances and release.
    {
        splicer::ObjectPool<int>::UniqueStackType one(pool.acquire(blockSize));
        splicer::ObjectPool<int>::UniqueStackType two(pool.acquire(blockSize));

        EXPECT_EQ(pool.available(), pool.allocated() - blockSize * 2);
        EXPECT_EQ(one.size(), blockSize);
        EXPECT_EQ(two.size(), blockSize);

        one.push(std::move(two));

        EXPECT_EQ(pool.available(), pool.allocated() - blockSize * 2);
        EXPECT_EQ(one.size(), blockSize * 2);
        EXPECT_EQ(two.size(), 0);
    }

    EXPECT_EQ(pool.available(), pool.allocated());

    // Merge separately acquired UniqueNode instances into a UniqueStack and
    // release.
    {
        splicer::ObjectPool<int>::UniqueNodeType one(pool.acquireOne(1));
        splicer::ObjectPool<int>::UniqueNodeType two(pool.acquireOne(2));

        EXPECT_EQ(pool.available(), pool.allocated() - 2);
        ASSERT_TRUE(one.get());
        ASSERT_TRUE(two.get());
        EXPECT_EQ(*one, 1);
        EXPECT_EQ(*two, 2);

        splicer::ObjectPool<int>::UniqueStackType stack(pool);
        stack.push(std::move(one));
        stack.push(std::move(two));

        EXPECT_FALSE(one.get());
        EXPECT_FALSE(two.get());
        EXPECT_EQ(stack.size(), 2);
    }

    EXPECT_EQ(pool.available(), pool.allocated());
}

TEST(UniqueSemantics, ManualRelease)
{
    splicer::ObjectPool<int> pool(blockSize);

    // Manual Stack release.
    {
        splicer::ObjectPool<int>::UniqueStackType one(pool.acquire(blockSize));
        splicer::ObjectPool<int>::UniqueStackType two(pool.acquire(blockSize));

        EXPECT_EQ(pool.available(), pool.allocated() - blockSize * 2);
        EXPECT_EQ(one.size(), blockSize);
        EXPECT_EQ(two.size(), blockSize);

        pool.release(std::move(one));

        EXPECT_EQ(pool.available(), pool.allocated() - blockSize);
        EXPECT_TRUE(one.empty());

        splicer::Stack<int> manual(two.release());

        EXPECT_TRUE(two.empty());
        EXPECT_EQ(manual.size(), blockSize);
        EXPECT_EQ(pool.available(), pool.allocated() - blockSize);

        pool.release(std::move(manual));
        EXPECT_EQ(pool.available(), pool.allocated());
    }

    // Manual Node release.
    {
        splicer::ObjectPool<int>::UniqueNodeType one(pool.acquireOne(1));
        splicer::ObjectPool<int>::UniqueNodeType two(pool.acquireOne(2));

        EXPECT_EQ(pool.available(), pool.allocated() - 2);
        ASSERT_TRUE(one.get());
        ASSERT_TRUE(two.get());
        EXPECT_EQ(*one, 1);
        EXPECT_EQ(*two, 2);

        pool.release(std::move(one));

        EXPECT_EQ(pool.available(), pool.allocated() - 1);
        EXPECT_FALSE(one.get());

        splicer::Node<int>* manual(two.release());

        EXPECT_FALSE(two.get());
        ASSERT_TRUE(manual);
        EXPECT_EQ(manual->val(), 2);

        pool.release(manual);

        EXPECT_EQ(pool.available(), pool.allocated());
    }

    // Push a released Node onto a UniqueStack.
    {
        splicer::ObjectPool<int>::UniqueStackType stack(
                pool.acquire(blockSize));

        splicer::ObjectPool<int>::NodeType* node(pool.acquireOne(8).release());

        EXPECT_EQ(pool.available(), pool.allocated() - blockSize - 1);
        ASSERT_TRUE(node);
        EXPECT_EQ(node->val(), 8);

        EXPECT_EQ(stack.size(), blockSize);
        stack.push(node);
        EXPECT_EQ(stack.size(), blockSize + 1);

        stack.reset();

        EXPECT_TRUE(stack.empty());
        EXPECT_EQ(pool.available(), pool.allocated());
    }
}

TEST(UniqueSemantics, DefaultConstruct)
{
    splicer::ObjectPool<int> pool(blockSize);

    // This node doesn't have a real deleter since it's not tied to a pool
    // instance.
    splicer::ObjectPool<int>::UniqueNodeType node(pool);

    EXPECT_FALSE(node.get());
    ASSERT_NO_THROW(node.reset());

    node = pool.acquireOne(42);

    EXPECT_EQ(pool.available(), pool.allocated() - 1);
    ASSERT_TRUE(node.get());
    EXPECT_EQ(*node, 42);

    node.reset();

    EXPECT_EQ(pool.available(), pool.allocated());
}

TEST(UniqueSemantics, NodeConstruct)
{
    splicer::ObjectPool<int> pool(blockSize);
    splicer::ObjectPool<int>::UniqueNodeType node(pool.acquireOne(42));

    EXPECT_TRUE(node.get());
    EXPECT_EQ(*node, 42);

    splicer::ObjectPool<int>::UniqueStackType stack(std::move(node));

    EXPECT_FALSE(node.get());
    EXPECT_EQ(stack.size(), 1);

    stack.reset();

    EXPECT_EQ(stack.size(), 0);
    EXPECT_EQ(pool.available(), pool.allocated());
}

TEST(UniqueSemantics, Iterate)
{
    splicer::ObjectPool<int> pool(blockSize);
    splicer::ObjectPool<int>::UniqueStackType stack(pool);

    const std::size_t max(10);

    for (std::size_t i(0); i < max; ++i)
    {
        stack.push(pool.acquireOne(i));
    }

    std::size_t i(0);

    splicer::ObjectPool<int>::UniqueStackType::Iterator it(stack.begin());
    splicer::ObjectPool<int>::UniqueStackType::ConstIterator end(stack.cend());

    while (it != end)
    {
        EXPECT_EQ(*it, max - ++i);
        ++it;
    }

    ASSERT_EQ(i, max);

    it = stack.begin();
    while (it != end)
    {
        *it = 1;
        ++it;
    }

    it = stack.begin();
    while (it != end)
    {
        EXPECT_EQ(*it, 1);
        ++it;
    }
}

TEST(UniqueSemantics, Range)
{
    splicer::ObjectPool<int> pool(blockSize);
    splicer::ObjectPool<int>::UniqueStackType stack(pool);

    const std::size_t max(10);

    for (std::size_t i(0); i < max; ++i)
    {
        stack.push(pool.acquireOne(i));
    }

    std::size_t i(0);
    for (const auto& n : stack)
    {
        EXPECT_EQ(n, max - i - 1);
        ++i;
    }

    for (auto& n : stack) n = 1;
    for (const auto& n : stack) { EXPECT_EQ(n, 1); }
}

TEST(UniqueSemantics, RangeCopy)
{
    splicer::ObjectPool<int> pool(blockSize);
    splicer::ObjectPool<int>::UniqueStackType stack(pool);

    const std::size_t max(10);

    for (std::size_t i(0); i < max; ++i)
    {
        stack.push(pool.acquireOne(i));
    }

    std::size_t i(0);
    for (const auto n : stack)
    {
        EXPECT_EQ(n, max - i - 1);
        ++i;
    }

    // Not actually changing the Stack values.
    for (auto n : stack) n = 1;

    i = 0;
    for (const auto n : stack)
    {
        EXPECT_EQ(n, max - i - 1);
        ++i;
    }
}

TEST(UniqueSemantics, RangeEmpty)
{
    splicer::ObjectPool<int> pool(blockSize);
    splicer::ObjectPool<int>::UniqueStackType stack(pool);

    std::size_t i(0);
    for (const auto& n : stack)
    {
        EXPECT_NE(n, 12345);  // Get rid of unused variable warning.
        ++i;
    }

    EXPECT_EQ(i, 0);
}

TEST(UniqueSemantics, SortedNodes)
{
    splicer::ObjectPool<int> pool(blockSize);
    splicer::ObjectPool<int>::UniqueStackType stack(pool);

    for (std::size_t i(19); i <= 19; i -= 2)
    {
        stack.push(pool.acquireOne(i), std::less<int>());
    }

    for (std::size_t i(0); i < 20; i += 2)
    {
        stack.push(pool.acquireOne(i), std::less<int>());
    }

    for (std::size_t i(0); i < 10; ++i)
    {
        stack.push(pool.acquireOne(10), std::less<int>());
    }

    EXPECT_TRUE(stack.sortedBy(std::less<int>()));
    EXPECT_EQ(stack.size(), 30);

    std::size_t i(0);
    for (const auto n : stack)
    {
        if (i < 10) ASSERT_EQ(n, i);
        else if (i < 20) ASSERT_EQ(n, 10);
        else ASSERT_EQ(n, i - 10);

        ++i;
    }
}

