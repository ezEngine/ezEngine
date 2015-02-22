ezEngine Roadmap {#EngineRoadmap}
================

This is a list of our planned features for the foreseeable future.

Soon:
  * More Game Objects improvements (Clemens)
  * Rendering Abstraction (Marc (DX11))
  * Higher level rendering features (Jan, Clemens, Marc)
  * Improvements to the resource manager (Jan)

One of the next releases:
  * Math: Simd (Christian)
  * Integration of D as a scripting language (Benjamin)
  * Math: Random Number Generators
  
Unknown:
  * Per Object Type Memory Tracking
  * Replace ezMap, ezSet Comparer with the Comparer in Algorithm; use < and == better (-1, 0, 1 comparison)
  * Optimize container regarding construction/destruction, remove unnecessary operations
  * Type-safe printf
  * An Editor (Christopher, Jan)


Known Bugs
----------

\bug 
  * On some machines Visual Studio regularly complains about outdated precompiled headers and only a full recompile fixes it (briefly). If that occurs for you, uncheck 'EZ_USE_PCH' in the CMake GUI, then it will go away (but you also lose precompiled headers, so compilation will take longer). This is a known issue with many cmake projects and mostly seems to happen when you have both VS 2010 and VS 2012 installed and apparently some other (unknown) factors.

