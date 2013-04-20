ezEngine Roadmap {#EngineRoadmap}
================

This is a list of our planned features for the foreseeable future.
Add / Modify as you like.


Milestone 1
-----------

  * OSX Port (Marc)
  * Deactivate doxygen documentation for certain (internal/external) code

  * ezMemoryStreamReader: Rewind -> should change that
  * ezMemoryStreamWriter: Rewind -> should change that

  * Unit-Tests for:
    * ezMemoryStreamReader
      * SkipBytes
      * Rewind -> needs refactoring
    * ezMemoryStreamWriter
      * Rewind -> needs refactoring
    * ezIBinaryStreamReader
      * SkipBytes
    * ezEndianHelper
      * SwitchInPlace -> unfinished ?
    * ezThreadUtils
      * IsMainThread
    * ezThread
      * GetThreadStatus
      * IsRunning
    * ezTime
  
    * ezVec2
    * ezVec3
      * CalculateNormal
      * MakeOrthogonalTo
      * GetOrthogonalVector
      * GetReflectedVector
      * GetRefractedVector
    * ezFileSystemIterator
  
  * Missing Documentation:
    * ezBitflags -> Less "why this is better" documentation, more "how to use it". Also document the Macros at the bottom better and add examples.
  

Short Term
----------

  * Binary Search Algorithm
  * Finish Flags / Enum stuff (?)
  * Time: Stopwatch ?
  * Variant Type (Clemens / Jan)
  * ezMap, ezSet Comparer mit dem Comparer in Algorithm ersetzen; auﬂerdem < und == sinnvoller verwenden (-1, 0, 1 vergleich etc.) (Clemens :-P )

Mid Term
--------

  * Profiling System
  * Containers: Sorted Array (Requires Binary Search)
  * Thread / Task Scheduling System
  * Math: Random Number Generators
  * Math: Frustum
  * Math: Simd (Clemens)
  * Memory: Tracking Allocator (Clemens)
  * Game Timer (fixed time step, frame time smoothing)
  * Optimize container regarding construction/destruction, unnecessary operations (ask Clemens for more info)
  * StringId using hashed string
  * Id / Id Mapping Table
  * RTTI
  * Text Streams
  * Type-safe printf

Bugs
----

  * \bug TestFramework does not like it, when you remove a test -> can crash -> removing settings.txt 'fixes' the problem


Done
----

  * BuildSystem (CMake for now)
  * Basic Type abstractions
  * Basic Compiler / Platform abstractions (Windows)
  * Assert Macros
  * Compile Time Checks
  * Memory Manager / Allocators
  * Memory: In 64 Bit Debug allocate everything above 4 GB (Clemens)
  * Types: Enum / Bitflags / ezResult / ArrayPtr
  * Sorting: QuickSort / InsertionSort
  * Math:
    * Vec2, Vec3, Vec4
    * Mat3, Mat4, Quaternion
    * Plane, BoundingBox, BoundingSphere
  * Endian Helpers
  * Memory: Copy / Move / Compare/ Construct / etc.
  * Threading:
    * OS Thread abstraction
    * Thread class
    * Mutex / Lock
    * Thread Local Storage
  * Time: High Precision "Now"
  * Enumerable Class
  * Atomic Integers
  * Hash: Murmur Hash
  * Hash: CRC32 (Marc)
  * Stream Interface (Marc)
  * Blackmagic
  * TestFramework
  * Containers:
    * StaticArray / HybridArray / DynamicArray
    * Set / Map
    * Deque / List
    * StaticRingBuffer
    * HashTable
  * Profiling Interface Stubs (Marc) - macro EZ_PROFILE now available
  * Strings / String Helpers / Unicode handling  
  * Path Helper Functions
  * Callback Handlers (Jan)
  * Log System (Jan)
  * Startup Config System (Jan)
  * Activate static code analysis in VS 2010 (Jan)
  * Global Event System (Jan)
  * OS File System Abstraction (Jan)
  * "IncludeAll.cpp" -> to compile / reference all code
  * FileSystem (Jan)
  * Logging: HTML Writer (Jan)
  * Binary Stream Operators for all Math Classes (Jan)
  * Documentation: Guidelines about Container Usage (Jan)
  * Documentation: Guidelines about String Usage (Jan)
  * Sample Application (Code Line Counter) (Jan)



