#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <deque>
#include <iostream>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

namespace splicer
{

template<typename T> class Stack;
template<typename T> class SplicePool;

template<typename T>
class Node
{
    friend class SplicePool<T>;
    friend class Stack<T>;

public:
    Node(Node* next = nullptr) : m_val(), m_next(next) { }

    template<class... Args>
    void construct(Args&&... args)
    {
        new (&m_val) T(std::forward<Args>(args)...);
    }

    T& val() { return m_val; }
    const T& val() const { return m_val; }

private:
    Node* next() { return m_next; }
    void next(Node* node) { m_next = node; }

    T m_val;
    Node* m_next;
};

template<typename T>
class Stack
{
    friend class SplicePool<T>;

public:
    Stack() : m_tail(nullptr), m_head(nullptr), m_size(0) { }

    void push(Node<T>* node)
    {
        assert(!m_tail || m_size);

        node->next(m_head);
        m_head = node;

        if (!m_size) m_tail = node;
        ++m_size;
    }

    void push(Stack& other)
    {
        if (!other.empty())
        {
            push(other.m_tail);
            m_head = other.head();
            m_size += other.size() - 1; // Tail has already been accounted for.
            other.clear();
        }
    }

    Node<T>* pop()
    {
        Node<T>* node(m_head);
        if (m_head)
        {
            m_head = m_head->next();
            if (!--m_size) m_tail = nullptr;
        }
        return node;
    }

    Stack popStack(std::size_t count)
    {
        Stack other;

        if (count >= size())
        {
            swap(other);
        }
        else if (count)
        {
            assert(!empty());

            Node<T>* tail(m_head);
            for (std::size_t i(0); i < count - 1; ++i) tail = tail->next();

            other.m_head = m_head;
            m_head = tail->next();

            tail->next(nullptr);
            other.m_tail = tail;

            other.m_size = count;
            m_size -= count;
        }

        return other;
    }

    bool empty() const { return !m_head; }
    std::size_t size() const { return m_size; }

    void print(std::size_t maxElements) const
    {
        if (Node<T>* current = m_head)
        {
            std::size_t i(0);

            while (current && i++ < maxElements)
            {
                std::cout << current->val() << " ";
                current = current->next();
            }

            if (current) std::cout << "and more...";

            std::cout << std::endl;
        }
        else
        {
            std::cout << "(empty)" << std::endl;
        }
    }

    void swap(Stack& other)
    {
        std::swap(*this, other);
    }

private:
    Node<T>* head() { return m_head; }

    void clear()
    {
        m_head = nullptr;
        m_tail = nullptr;
        m_size = 0;
    }

    Node<T>* m_tail;
    Node<T>* m_head;
    std::size_t m_size;
};

class TryLocker
{
public:
    explicit TryLocker(std::atomic_flag& flag)
        : m_set(false)
        , m_flag(flag)
    { }

    ~TryLocker()
    {
        if (m_set) m_flag.clear();
    }

    bool tryLock()
    {
        m_set = !m_flag.test_and_set();
        return m_set;
    }

private:
    bool m_set;
    std::atomic_flag& m_flag;
};

template<typename T>
class SplicePool
{
public:
    typedef Node<T> NodeType;
    typedef Stack<T> StackType;

    SplicePool(std::size_t blockSize)
        : m_blockSize(blockSize)
        , m_stack()
        , m_mutex()
        , m_adding()
        , m_allocated(0)
    {
        m_adding.clear();
    }

    virtual ~SplicePool() { }

    std::size_t allocated() const
    {
        return m_allocated.load();
    }

    std::size_t available() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_stack.size();
    }

    void release(Node<T>* node)
    {
        reset(&node->val());

        std::lock_guard<std::mutex> lock(m_mutex);
        m_stack.push(node);
    }

    void release(Stack<T>& other)
    {
        Node<T>* node(other.head());
        while (node)
        {
            reset(&node->val());
            node = node->next();
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        m_stack.push(other);
    }

    template<class... Args>
    Node<T>* acquire(Args&&... args)
    {
        Node<T>* node(0);

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            node = m_stack.pop();
        }

        if (node)
        {
            if (!std::is_pointer<T>::value)
            {
                node->construct(std::forward<Args>(args)...);
            }

            return node;
        }
        else
        {
            allocate();
            return acquire(std::forward<Args>(args)...);
        }
    }

    Stack<T> acquireStack(const std::size_t count)
    {
        Stack<T> other;

        std::unique_lock<std::mutex> lock(m_mutex);
        if (count >= m_stack.size())
        {
            m_stack.swap(other);

            lock.unlock();

            if (count > other.size())
            {
                const std::size_t nodesNeeded(count - other.size());
                const std::size_t blocksNeeded(nodesNeeded / m_blockSize + 1);

                Stack<T> alloc(doAllocate(blocksNeeded));

                m_allocated += alloc.size();
                assert(alloc.size() == blocksNeeded * m_blockSize);

                Stack<T> taken(alloc.popStack(nodesNeeded));
                other.push(taken);

                lock.lock();
                m_stack.push(alloc);
            }
        }
        else
        {
            other = m_stack.popStack(count);
        }

        return other;
    }

protected:
    void allocate()
    {
        TryLocker locker(m_adding);

        if (locker.tryLock())
        {
            Stack<T> newStack(doAllocate(1));
            m_allocated += m_blockSize;

            std::lock_guard<std::mutex> lock(m_mutex);
            m_stack.push(newStack);
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    void reset(T* val)
    {
        destruct(val);
        construct(val);
    }

    virtual Stack<T> doAllocate(std::size_t blocks) = 0;
    virtual void construct(T*) const { }
    virtual void destruct(T*) const { }

    const std::size_t m_blockSize;

private:
    Stack<T> m_stack;
    mutable std::mutex m_mutex;

    std::atomic_flag m_adding;
    std::atomic_size_t m_allocated;
};

template<typename T>
class ObjectPool : public SplicePool<T>
{
public:
    ObjectPool(std::size_t blockSize = 4096)
        : SplicePool<T>(blockSize)
        , m_blocks()
        , m_mutex()
    { }

private:
    virtual Stack<T> doAllocate(std::size_t blocks) override
    {
        Stack<T> stack;
        std::deque<std::unique_ptr<std::vector<Node<T>>>> newBlocks;

        for (std::size_t i(0); i < blocks; ++i)
        {
            std::unique_ptr<std::vector<Node<T>>> newBlock(
                    new std::vector<Node<T>>(this->m_blockSize));
            newBlocks.push_back(std::move(newBlock));
        }

        for (auto& block : newBlocks)
        {
            auto& vec(*block);

            for (std::size_t i(0); i < vec.size(); ++i)
            {
                stack.push(&vec[i]);
            }
        }

        std::lock_guard<std::mutex> lock(m_mutex);

        m_blocks.insert(
                m_blocks.end(),
                std::make_move_iterator(newBlocks.begin()),
                std::make_move_iterator(newBlocks.end()));

        return stack;
    }

    virtual void construct(T* val) const override
    {
        new (val) T();
    }

    virtual void destruct(T* val) const override
    {
        val->~T();
    }

    std::deque<std::unique_ptr<std::vector<Node<T>>>> m_blocks;
    mutable std::mutex m_mutex;
};

template<typename T>
class BufferPool : public SplicePool<T*>
{
public:
    BufferPool(std::size_t bufferSize, std::size_t blockSize = 1)
        : SplicePool<T*>(blockSize)
        , m_bufferSize(bufferSize)
        , m_bytesPerBlock(m_bufferSize * this->m_blockSize)
        , m_bytes()
        , m_nodes()
        , m_mutex()
    { }

private:
    virtual Stack<T*> doAllocate(std::size_t blocks) override
    {
        Stack<T*> stack;

        std::deque<std::unique_ptr<std::vector<T>>> newBytes;
        std::deque<std::unique_ptr<std::vector<Node<T*>>>> newNodes;

        for (std::size_t i(0); i < blocks; ++i)
        {
            std::unique_ptr<std::vector<T>> newByteBlock(
                    new std::vector<T>(m_bytesPerBlock));

            std::unique_ptr<std::vector<Node<T*>>> newNodeBlock(
                    new std::vector<Node<T*>>(this->m_blockSize));

            newBytes.push_back(std::move(newByteBlock));
            newNodes.push_back(std::move(newNodeBlock));
        }

        for (std::size_t i(0); i < blocks; ++i)
        {
            std::vector<T>& newBytes(*m_bytes[i]);
            std::vector<Node<T*>>& newNodes(*m_nodes[i]);

            for (std::size_t i(0); i < this->m_blockSize; ++i)
            {
                Node<T*>& node(newNodes[i]);
                node.val() = &newBytes[m_bufferSize * i];
                stack.push(&node);
            }
        }

        std::lock_guard<std::mutex> lock(m_mutex);

        m_bytes.insert(
                m_bytes.end(),
                std::make_move_iterator(newBytes.begin()),
                std::make_move_iterator(newBytes.end()));

        m_nodes.insert(
                m_nodes.end(),
                std::make_move_iterator(newNodes.begin()),
                std::make_move_iterator(newNodes.end()));

        return stack;
    }

    virtual void construct(T** val) override
    {
        std::fill(*val, *val + m_bufferSize, 0);
    }

    const std::size_t m_bufferSize;
    const std::size_t m_bytesPerBlock;

    std::deque<std::unique_ptr<std::vector<T>>> m_bytes;
    std::deque<std::unique_ptr<std::vector<Node<T*>>>> m_nodes;
    mutable std::mutex m_mutex;
};

} // namespace splicer

