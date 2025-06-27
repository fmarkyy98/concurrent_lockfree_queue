// ConcurrentLockfreeQueue.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <chrono>
#include <iostream>
#include <thread>

#include <queue>
#include <shared_mutex>

#include "concurrent_lockfree_queue_restricted_for_single_producer_and_single_consumer.hpp"

using namespace std::chrono;

constexpr std::size_t N = 50'000'000;

int main()
{
    std::cout << "--- lockfree ---\n";
    {
        concurrent_lockfree_queue_restricted_for_single_producer_and_single_consumer<int> q;
        auto prod_end = q.acquire_producer_end();
        auto cons_end = q.acquire_consumer_end();

        auto prod_job = std::jthread(
            [&prod_end]()
            {
                auto start = high_resolution_clock::now();

                for( size_t i = 1; i <= N; ++i )
                {
                    while( !prod_end->enqueue( i ) )
                    {
                        // std::cout << "queue is full, producer yields\n";
                        std::this_thread::yield();
                    }
                }

                auto end = high_resolution_clock::now();

                std::cout << "producer finished: " << duration_cast<milliseconds>( end - start ) << "\n";
            } );

        auto cons_job = std::jthread(
            [&cons_end]()
            {
                auto start = high_resolution_clock::now();

                for( size_t i = 1; i <= N; ++i )
                {
                    std::optional<int> val;
                    while( ( val = cons_end->dequeue() ) == std::nullopt )
                    {
                        // std::cout << "queue is empty, consumer yields\n";
                        std::this_thread::yield();
                    }

                    if( val != i ) [[unlikely]]
                    {
                        std::cout << "Error: unexpected value: (" << val.value() << ") expected: (" << i << ")\n";
                    }
                }

                auto end = high_resolution_clock::now();

                std::cout << "consumer finished: " << duration_cast<milliseconds>( end - start ) << "\n";
            } );
    }

    std::cout << "--- mutex approach ---\n";
    {
        std::queue<int> q;
        std::shared_mutex m;

        auto prod_job = std::jthread(
            [&q, &m]()
            {
                auto start = high_resolution_clock::now();

                for( size_t i = 1; i <= N; ++i )
                {
                    std::unique_lock lock( m );
                    q.push( i );
                }

                auto end = high_resolution_clock::now();

                std::cout << "producer finished: " << duration_cast<milliseconds>( end - start ) << "\n";
            } );

        auto cons_job = std::jthread(
            [&q, &m]()
            {
                auto start = high_resolution_clock::now();

                for( size_t i = 1; i <= N; ++i )
                {
                    int val;
                    {
                        std::shared_lock s_lock( m );
                        while( q.empty() )
                        {
                            s_lock.unlock();

                            // std::cout << "queue is empty, consumer yields\n";
                            std::this_thread::yield();

                            s_lock.lock();
                        }

                        val = q.front();

                        s_lock.unlock();

                        std::unique_lock lock( m );

                        q.pop();
                    }

                    if( val != i ) [[unlikely]]
                    {
                        std::cout << "Error: unexpected value: (" << val << ") expected: (" << i << ")\n";
                    }
                }

                auto end = high_resolution_clock::now();

                std::cout << "consumer finished: " << duration_cast<milliseconds>( end - start ) << "\n";
            } );
    }
}
