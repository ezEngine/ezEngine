Container Usage Guidelines {#ContainerUsageGuidelines}
==========================

ezEngine has the following container classes:

  * `ezStaticArray`
  * `ezHybridArray`
  * `ezDynamicArray`
  * `ezStaticRingBuffer`
  * `ezDeque`
  * `ezList`
  * `ezMap`
  * `ezSet`
  * `ezHashTable`
  * `ezArrayMap`

The following containers store their data as contiguous arrays:

  * `ezStaticArray`
  * `ezHybridArray`
  * `ezDynamicArray`
  * `ezStaticRingBuffer`
  * `ezArrayMap`

The following containers are built on top of `ezDeque` and thus share some performance characteristics:

  * `ezList`
  * `ezMap`
  * `ezSet`

When to use which Container Type
--------------------------------

### Arrays ###
  
  #### General Guidelines ####
  
    For arrays, `ezHybridArray`, `ezDynamicArray` and `ezDeque` are the most important containers.
  
    If you know or have a guess how much data you will need, always use this information in a call to `Reserve` to ensure that the containers can allocate data once (or at least much less), and do not need to reallocate several times.
    
    Never remove an element in between (using `RemoveAt`), unless there is really no other way (and hopefully your array is small). Prefer `RemoveAtSwap` to replace the removed element by the last element in the array instead (this will destroy the order though).
    Similarly, never insert elements anywhere else than at the end.
    The only exception is `ezDeque`, which is very efficient at insertion and removal of elements at both ends.
  
  
  
  #### `ezHybridArray` ####
  
    `ezHybridArray` uses an internal fixed size cache (which you can specify as a template argument). When you create an `ezHybridArray` on the stack, that data is also allocated on the stack. This is the most important container for writing performance critical yet safe code. `ezHybridArray` allows to implement many algorithms without the need to use dynamic allocations.
  
    Prefer `ezHybridArray` when you have a use case where you typically have a small set of elements, but it might be larger in some situations. `ezHybridArray` will give you the performance of an immediately available array on the stack, combined with the safety of a dynamically allocated array on the heap, as it will reallocate data dynamically, whenever its internal storage is insufficient.
  
    However, be careful not to make the internal buffer too large. When creating `ezHybridArrays` on the stack, you should not use more than a few KB for the internal cache, as you increase the risk for stack overflows. You should usually try to stay below `(2KB / sizeof(Type))` for the number of elements in the `ezHybridArray` cache.
  
  
  
  #### `ezDynamicArray` ####
  
    `ezDynamicArray` always allocates its data on the heap. The upside is, that it has a very low memory overhead, as long as it is empty, and it can handle any number of elements.
  
    Prefer `ezDynamicArray` if your working set is generally larger and when you know how many elements it needs to hold before you fill it up. Use `Reserve` or `SetCount` BEFORE you start adding data to it, to prevent unnecessary reallocation later, as those are very costly.

    Also prefer `ezDynamicArray` if the memory overhead in the empty state is of concern.


  #### `ezDeque` ####
  
    `ezDeque` stores its data as several chunks of contiguous arrays. An additional "redirection array" is used to know how to index into these chunks.
    `ezDeque` requires one pointer indirection to make a lookup into its data.
  
    The deque is the only array container that does not store its data in one contiguous block of memory. Be aware of that, you cannot `memcpy` or `memcmp` data in a deque with more than one element, as the next element might be stored somewhere completely different.
    
    However a deque NEVER relocates an existing element in memory, either. That means once an element is inserted into a deque, you can safely store pointers to it, as it will not move around in memory (unless it is deleted, of course).

    Deques allocate their data in chunks. So whenever the memory is insufficient, a new chunk that can hold a fixed number of elements is allocated. One chunk is typically 4 KB in size. That means if you store `floats` in a deque, one chunk can hold 1000 values. Thus this is the 'minimum' memory usage of a deque, unless it is completely empty still.
    
    As such `ezDeques` are very efficient for iteration (most of the data is contiguous) and they are very dynamic, as their size can grow dynamically without the need to reallocate and copy previous data, as the other array types would need to do.
    
    Therefore prefer `ezDeques` whenever you need to have nearly the performance of an array, but have very dynamic data sets, that are difficult to predict in size, or that vary all the time.
    
    As deques are optimized for insertion and removal at both ends, `ezDeques` are perfectly suited as fifo queues and dynamic ring-buffers.
    
    The memory overhead of `ezDeques` is rather high, so do not use it for small data-sets (here `ezHybridArray` is typically the best container).
    
    `ezDeques` are also very well suited, whenever you have large objects to store that are very costly to construct or copy around, and you want to prevent those operations by all means, such that you do not want a container reallocation to trigger that.
    
    


  #### `ezStaticArray` ####
  
    This is a container that only stores a static array internally and cannot resize itself to be larger than that. Use this in code that has a definite upper limit of elements and whenever you must prevent the usage of any allocator by all means (such as for global variables).
    
    Typically there should be no need for this container, as `ezHybridArray` delivers the same performance advantages and the safety of reallocating to the required size dynamically.



  #### `ezStaticRingBuffer` ####
  
    Use this when you need a ring-buffer that shall have a fixed size. Use `ezDeque` if you need a dynamically resizing ring-buffer.



### Lists ###

  There is only one implementation of a doubly linked list: `ezList`
  
  `ezList` is internally built on top of a `ezDeque`. As such the memory requirements are the same.
  
  `ezList` is optimized for inserting and removing objects frequently. Iterating over its elements might be slow due to excessive cache misses.
  
  You should typically not use `ezList` in code that is performance critical. Use it when you have data-sets of unpredictable size that need to be sorted or rearranged very dynamically. For example when you read a complex data set which then needs to be processed and sorted by different criteria, which might delete and insert elements at random positions, then prefer an `ezList`. However, once that step is finished, you should copy your sorted data into some array container for faster access.
  
  Nodes in an `ezList` are never relocated in memory, as such iterators stay valid as long as the element is still alive.



### Associative Containers ###

  #### `ezMap` and `ezSet` ####

    Both containers are basically the same, except that `ezMap` stores a 'value' for each 'key', whereas a set only stores 'keys'.
    
    Use `ezMap` whenever you need to be able to look up an entry with a key. Use `ezSet` whenever you simply need to know whether some element is present or to merge data-sets down to all the unique elements.
    
    `ezMap` and `ezSet` are built on top of `ezDeque`, similar to `ezList`, that means they are quite heavy-weight in their memory consumption, however they can grow in size efficiently. As with `ezList`, iterators (and pointers) to elements stay valid, as long as the element is alive, i.e. the data is never relocated.
    
    Insertion, lookup and removal are all `O(log n)` operations, since they are red-black trees internally and thus always perfectly balanced.
    
    `ezMap` and `ezSet` are well suited for very dynamic data sets (where a lot of insertions and removals are done, while also using it for lookup). If you have a use-case where you insert once and then lookup often, a sorted array, such as `ezArrayMap`, or an `ezHashTable` might be more efficient.
    
    `ezMap` and `ezSet` only require a simple comparer to be able to sort elements in a strictly weak ordering. As such they are well suited to handle objects that can be difficult to be hashed.
    
    Note that the nodes in the Map/Set each contain one element of their key/value type and those are stored in an `ezDeque`. As such, when you put an `ezHybridArray` (or an `ezString`) into an `ezMap`, only one allocation is needed to allocate all the memory for a chunk (in the `ezDeque`) of data, which holds a large number of nodes, which already embed the data of their keys/values (e.g. `ezHybridArray`). Thus you can get away with very few memory allocations.
    If however you store an `ezDynamicArray` in an `ezMap`, each element still needs to allocate its own internal storage, which means you will get one additional allocation per element.

  #### `ezArrayMap` ####
  
    This container provides similar functionality as `ezMap` but should be more efficient in scenarios where elements are looked up more often than they are inserted or removed. The implementation simply uses an array that is kept sorted, such that lookups can be done in a more cache friendly manner.
    
    If all you need is an associative container and your use case consists of changing the container infrequently, but looking up elements frequently (which is very often the case), you should prefer this container.
    
    Note, however, that this container will rearrange elements in memory whenever it needs to be sorted. In contrast an `ezMap` guarantees that elements never move in memory, allowing to store pointers to the memory locations. Likewise the iterators of an `ezMap` stay valid as long as an elements resides in the map. For `ezArrayMap` this is not true, the index at which an element is stored can change whenever any element is added or removed.

  #### `ezHashTable` ####

    The `ezHashTable` is optimized for very fast lookup, which should typically be a `O(1)` operation. Prefer the hash table whenever you will have a data set that is modified infrequently, but lookups will be done often and need to be as fast as possible.
    
    `ezHashTable` may relocate its internal memory, which means iterators to its elements may not stay valid when the table is modified.
    Make sure to `Reserve` how much elements you probably will put into the `ezHashTable`, to ensure fewer reallocations, but also to avoid hash-collisions and thus to improve overall performance.


