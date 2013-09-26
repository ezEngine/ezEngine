ezEngine Roadmap {#EngineRoadmap}
================

This is a list of our planned features for the foreseeable future.

  * Binary Search Algorithm
  * Time: Stopwatch
  * Variant Type
  * ezMap, ezSet Comparer mit dem Comparer in Algorithm ersetzen; auﬂerdem < und == sinnvoller verwenden (-1, 0, 1 vergleich etc.) (Clemens :-P )
  * Containers: Sorted Array (Requires Binary Search)
  * Math: Random Number Generators
  * Math: Frustum
  * Math: Simd (Clemens)
  * Game Timer (fixed time step, frame time smoothing)
  * Optimize container regarding construction/destruction, unnecessary operations (ask Clemens for more info)
  * StringId using hashed string
  * RTTI
  * Text Streams
  * Type-safe printf
  * JSON reader / writer
  * Game Object basics (Clemens)
  * Rendering Abstraction basics (Marc)
  * Resource Manager

Known Bugs
----------

  * \bug On some machines Visual Studio regularly complains about outdated precompiled headers and only a full recompile fixes it (briefly). If that occurs for you, uncheck 'EZ_USE_PCH' in the CMake GUI, then it will go away (but you also lose precompiled headers, so compilation will take longer). This is a known issue with many cmake projects and mostly seems to happen when you have both VS 2010 and VS 2012 installed and apparently some other (unknown) factors.
  * \bug TestFramework does not like it, when you remove a test -> can crash -> removing settings.txt 'fixes' the problem
