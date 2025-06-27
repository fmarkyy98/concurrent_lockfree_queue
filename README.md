# concurrent_lockfree_queue
Implementation of a concurrent lockfree queue restricted for single producer and single consumer.

Banchmark result:
--- lockfree ---
producer finished: 7440ms
consumer finished: 7440ms

--- mutex approach ---
producer finished: 18486ms
consumer finished: 24744ms
