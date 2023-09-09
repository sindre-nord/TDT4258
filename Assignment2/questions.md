
# Optional Questions

### Question 1
Why can a cache respond faster to the reads and writes from
the processor than the main memory? How many reasons do you find?
Why don’t we just use cache if it is faster?

### Answer 1
The cache typically utelizes a much faster memory technology. Reading and writing is much faster. Also, it's
commonly placed much closer to the physical processor. The cavieat is price.
#### Pros giving speed
- It's based on faster technology
- It's closer
#### Cons, why not always use it:
- It's far more expensive

### Question 2
Why do we want to simulate the cache in software instead
of running it on actual hardware and just observe how the hardware
behaves?
### Answer 2
If we where to try to simulate the cache in hardware, we would have to build a physical model of the cache. This would be very expensive and time consuming. Also, we would have to build a model for each different type of cache. By simulating it in software, we can easily change the model and run it on any computer. This is much cheaper and faster.

