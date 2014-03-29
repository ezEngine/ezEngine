ezEngine Roadmap {#EngineRoadmap}
================

This is a list of our planned features for the foreseeable future.

Soon:
  * Per Object Type Memory Tracking (Clemens)
  * Game Objects (Clemens)
  * Rendering Abstraction (Marc)
  * Resource Manager (Jan)

One of the next releases:
  * Math: Simd

Unknown:
  * Binary Search Algorithm
  * ezMap, ezSet Comparer mit dem Comparer in Algorithm ersetzen; auﬂerdem < und == sinnvoller verwenden (-1, 0, 1 vergleich etc.) (Clemens :-P )
  * Containers: Sorted Array (Requires Binary Search)
  * Math: Random Number Generators
  * Optimize container regarding construction/destruction, unnecessary operations (ask Clemens for more info)
  * Text Streams
  * Type-safe printf


Known Bugs
----------

\bug 
  * On some machines Visual Studio regularly complains about outdated precompiled headers and only a full recompile fixes it (briefly). If that occurs for you, uncheck 'EZ_USE_PCH' in the CMake GUI, then it will go away (but you also lose precompiled headers, so compilation will take longer). This is a known issue with many cmake projects and mostly seems to happen when you have both VS 2010 and VS 2012 installed and apparently some other (unknown) factors.

\bug
  * TestFramework does not like it, when you remove a test -> can crash -> removing settings.txt 'fixes' the problem

\bug
  * Running the FoundationTests three times in a row (or more) will make them fail in the thread local storage system.