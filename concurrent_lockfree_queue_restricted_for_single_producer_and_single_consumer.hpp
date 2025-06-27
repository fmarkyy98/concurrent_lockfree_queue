#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <vector>

#define trivial default

template<typename T>
class concurrent_lockfree_queue_restricted_for_single_producer_and_single_consumer
{
public:
    class producer_end
    {
        friend class concurrent_lockfree_queue_restricted_for_single_producer_and_single_consumer;

    public:
        producer_end( const producer_end& )            = delete;
        producer_end& operator=( const producer_end& ) = delete;

        producer_end( producer_end&& )            = trivial;
        producer_end& operator=( producer_end&& ) = trivial;

        ~producer_end()
        {
            m_is_acquired->store( false, std::memory_order_relaxed );
        }

        bool enqueue( const T& value )
        {
            auto capacity = m_vec->size();
            auto prod     = m_prod_pos->load( std::memory_order_relaxed );
            auto cons     = m_cons_pos->load( std::memory_order_acquire );

            auto next = ( prod + 1 ) % capacity;
            if( next == cons )
                return false; // full

            ( *m_vec )[prod] = value;
            m_prod_pos->store( next, std::memory_order_release );

            return true;
        }

    private:
        producer_end( const std::shared_ptr<std::vector<T>>& vec,
                      const std::shared_ptr<std::atomic_bool>& is_acquired,
                      const std::shared_ptr<std::atomic<std::size_t>>& pos,
                      const std::shared_ptr<std::atomic<std::size_t>>& other_pos ) :
            m_vec( vec ),
            m_is_acquired( is_acquired ),
            m_prod_pos( pos ),
            m_cons_pos( other_pos )
        {
        }

        std::shared_ptr<std::vector<T>> m_vec;
        std::shared_ptr<std::atomic_bool> m_is_acquired;
        std::shared_ptr<std::atomic<std::size_t>> m_prod_pos;
        std::shared_ptr<std::atomic<std::size_t>> m_cons_pos;
    };

    class consumer_end
    {
        friend class concurrent_lockfree_queue_restricted_for_single_producer_and_single_consumer;

    public:
        consumer_end( const producer_end& )            = delete;
        consumer_end& operator=( const consumer_end& ) = delete;

        consumer_end( consumer_end&& )            = trivial;
        consumer_end& operator=( consumer_end&& ) = trivial;

        ~consumer_end()
        {
            m_is_acquired->store( false, std::memory_order_relaxed );
        }

        std::optional<T> dequeue()
        {
            auto capacity = m_vec->size();
            auto cons     = m_cons_pos->load( std::memory_order_relaxed );
            auto prod     = m_prod_pos->load( std::memory_order_acquire );

            if( cons == prod )
                return std::nullopt; // empty

            T value   = ( *m_vec )[cons];
            auto next = ( cons + 1 ) % capacity;
            m_cons_pos->store( next, std::memory_order_release );

            return value;
        }

    private:
        consumer_end( const std::shared_ptr<std::vector<T>>& vec,
                      const std::shared_ptr<std::atomic_bool>& is_acquired,
                      const std::shared_ptr<std::atomic<std::size_t>>& pos,
                      const std::shared_ptr<std::atomic<std::size_t>>& other_pos ) :
            m_vec( vec ),
            m_is_acquired( is_acquired ),
            m_prod_pos( pos ),
            m_cons_pos( other_pos )
        {
        }

        std::shared_ptr<std::vector<T>> m_vec;
        std::shared_ptr<std::atomic_bool> m_is_acquired;
        std::shared_ptr<std::atomic<std::size_t>> m_prod_pos;
        std::shared_ptr<std::atomic<std::size_t>> m_cons_pos;
    };

    concurrent_lockfree_queue_restricted_for_single_producer_and_single_consumer( std::size_t size = 256 ) :
        m_vec( std::make_shared<std::vector<T>>( size + 1 ) )
    {
    }

    concurrent_lockfree_queue_restricted_for_single_producer_and_single_consumer(
        const concurrent_lockfree_queue_restricted_for_single_producer_and_single_consumer& ) = delete;
    concurrent_lockfree_queue_restricted_for_single_producer_and_single_consumer&
    operator=( const concurrent_lockfree_queue_restricted_for_single_producer_and_single_consumer& ) = delete;

    concurrent_lockfree_queue_restricted_for_single_producer_and_single_consumer(
        concurrent_lockfree_queue_restricted_for_single_producer_and_single_consumer&& ) = trivial;
    concurrent_lockfree_queue_restricted_for_single_producer_and_single_consumer&
    operator=( concurrent_lockfree_queue_restricted_for_single_producer_and_single_consumer&& ) = trivial;

    std::unique_ptr<producer_end> acquire_producer_end()
    {
        bool expected = false;
        if( !m_is_producer_acquired->compare_exchange_strong( expected, true, std::memory_order_acq_rel ) )
            return nullptr; // Already acquired

        return std::unique_ptr<producer_end>(
            new producer_end( m_vec, m_is_producer_acquired, m_prod_pos, m_cons_pos ) );
    }

    std::unique_ptr<consumer_end> acquire_consumer_end()
    {
        bool expected = false;
        if( !m_is_consumer_acquired->compare_exchange_strong( expected, true, std::memory_order_acq_rel ) )
            return nullptr; // Already acquired

        return std::unique_ptr<consumer_end>(
            new consumer_end( m_vec, m_is_consumer_acquired, m_prod_pos, m_cons_pos ) );
    }

private:
    std::shared_ptr<std::vector<T>> m_vec;

    std::shared_ptr<std::atomic_bool> m_is_producer_acquired = std::make_shared<std::atomic_bool>( false );
    std::shared_ptr<std::atomic_bool> m_is_consumer_acquired = std::make_shared<std::atomic_bool>( false );

    std::shared_ptr<std::atomic<std::size_t>> m_prod_pos = std::make_shared<std::atomic<std::size_t>>( 0 );
    std::shared_ptr<std::atomic<std::size_t>> m_cons_pos = std::make_shared<std::atomic<std::size_t>>( 0 );
};
