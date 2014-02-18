What's New {#WhatsNew}
==========

Milestone 5
-----------

  * Added ezJSONParser which is a low level parser that reads JSON incrementally.
  * Added ezJSONReader which generates a high level representation of a JSON document, that can be used to lookup data easily.
  * Added ezExtendedJSONReader which generates a high level representation of a JSON document, but also uses the extended JSON syntax for more precise type information and values.

Milestone 4
-----------

  * Added Linux support, which is on par with OS X port. Tested with Ubuntu 12.4 and GCC 4.8.
  * Removed the InputWindows plugin and merged the functionality into the ezWindow code (System library).
  * Added an SFML port for Window creation and Input handling to ezWindow, which works on non-Windows platforms.
  * Added code and a small tool that fixes problems with static linking.
  * Added ezDynamicOctree and ezDynamicQuadtree to the CoreUtils library, which are data structures to optimize range queries.
  * Added Lua to the ThirdParty library.
  * Added ezHashedString and ezTempHashedString for storing strings that rarely change but where comparisons should be really fast.
  * Added ezFrustum for object culling.
  * Added ezLuaWrapper to CoreUtils for easier working with Lua scripts.
  * Added ezGraphicsUtils with functions for mapping points to and from world-space and screen-space.
  * Added ezCamera, a lightweight class to do standard camera actions (moving, rotating, etc.)
  * In debug builds all math classes do NaN checks, to detect the usage of uninitialized data.
  * Added ezGameUtils library which contains some general purpose functions that might be useful for different types of games.
  * Added a reflection system for runtime type information (see ezRTTI etc.).
  * ezInspector can show some information about reflected types.
  * ezInspector will now stay in the foreground while active.
  * Allocators are now implemented in such a way that ezStaticAllocatorWrapper etc. are not needed anymore. Global variables can now be created without any workarounds.
  * Added a new sample that implements some of the basics for an RTS-style game. Work in progress.
  * Moved ezSystemTime::Now() into ezTime.
  * The ezTaskSystem now computes the worker thread utilization, which can be queried manually and is also transmitted to ezInspector.
  * ezInspector can now display the history (up to 10 minutes) of every stat variable, as long as it contains a number.

Milestone 3
-----------
  
  * Added ezTimestamp, a platform independent timestamp for file-time and system-time stored in microseconds since Unix epoch.
  * Added ezDateTime, a class that converts an ezTimestamp into human-readable form.
  * Added an input abstraction system (ezInputManager, ezInputDevice, etc.).
     * Implements mouse/keyboard and XBox 360 controller on Windows.
     * Contains ezVirtualThumbStick, which can be used on touch devices for 'controller input'.
     * Can be extended through additional ezInputDevice's to handle more platforms and device types.
  * Added the basics of a game object system (ezWorld, ezGameObject, ezComponent). The Asteroids game sample already shows how to do a simple game with it. The system is not yet complete but good enough for now.     
  * Added a sample game, similar to Asteroids, to show how to use the Game Object system and the Input System.
  * Added ezCompressedStreamReader and ezCompressedStreamWriter, which allow to (un-)compress data while writing/reading a file or memory stream.
  * Asserts can now be intercepted with an Assert Handler
  * Many subsystems (ezOSFile, ezInputManager, ezCVar) now broadcast more info about the status via ezEvent so that introspection becomes possible.
  * ezEvent can now be used with a locking policy, if thread-safety is required.
  * Added ezTelemetry, a system that allows to broadcast information about the application to interested other applications via network. Once connected, communication can go both ways to modify the application behavior.
  * Added ezStats, an easy system to store information about the current application state (player health, ammo, AI status, ...). This can be transmitted via ezTelemetry (or other tools) and displayed in a convenient way for better debugging.
  * Added ezInspector, a tool that can display information about ezStats, ezCVar, ezLog messages, ezSubSystem, ezPlugin, ezGlobalEvent, ezOSFile, memory allocations and asserts. It can be run in parallel to a game, either on the same PC or a different one and connect via TCP to the application that will then transmit that information.
  * Added ezInspectorPlugin, the engine-side plugin that enables the introspection features required to communicate with ezInspector.
  * Added a Qt GUI for the testframework.
  * Added ezApplication, a basic application abstraction.
  * Added ezWindow, a basic window abstraction.
  * Changed ezMemoryStreamStorage, ezMemoryStreamReader and ezMemoryStreamWriter to allow to use them in more complicated code and without allocations in many situations.
  * Added ezRectTemplate and ezSizeTemplate for rect and size types.
  * Added ezMemoryUtils::DefaultConstruct which will always default construct objects, even POD types such as int, float, etc.
  * Added ezMemoryUtils::ByteCompare to do a real byte compare even on class types.
  * Added 'ThirdParty' library.
    * Included zlib for zip compression.
    * Included enet for network communication.
  * Added 'System' library.
    * Currently only includes ezWindow.
  * Added ezDelegate for callbacks that work with static functions and member functions.
  * Fixed some code analysis warnings with Visual Studio 2012.
  * Fixed several bugs.
  * New ezVariant type that is much more powerful and useful than the previous implementation.
  * Added ezConversionUtils class for converting strings to int/float/bool.
  * Added optional *.natvis support for Visual Studio 2012. This enables much more comfortable inspection of basic types in the debugger.
  * The logging system has been improved. You can now log locally into another system (useful for tools) or you can replace the default implementation entirely.
  * Added ezClock for managing game time updates.
  * Added ezStopwatch for measuring time.
  * Added the ezAngle class to better deal with angles in degree, radians (easier to use, typesafe and more efficient!)
  * Added ezJSONWriter interface and two derived implementations for writing standard JSON and 'extended' JSON.
  * Added ezColor, a class to convert color values back and forth between different color spaces.
  * Added ezColor8UNorm to store 8-Bit RGBA color values.
  * Added ezColor16f to store 16-Bit RGBA color values.
  * Added ezFloat16 to convert float values to 16-Bit half floats and back.
  * Added ezTimestamp which stores a timestamp in microseconds since unix epoch.
  * Added ezDateTime which converts an ezTimestamp into human readable form and back.

Milestone 2
-----------

  * Added CVars (global configuration variables)
  * Added Plugin loading / unloading / reloading
  * Added GPA integration for profiling
  * Added Reloadable global variables for plugins
  * Added Bitfield Container
  * Added generic Id class and Id mapping table
  * Added Stack Tracing for better memory leak detection
  * Added Mac Port (mostly)
  * Added Application Entry Point abstraction (main / WinMain)
  * Added Task Pool System
  * Added BMP Writer
  * Added Fixed Point type
  * All math classes are now templated, allowing double as internal type
  * Added ezVec2d, ezVec3d, ezVec4d, ezPlaned, ezQuatd, ezMat3d, ezMat4d, ezBoundingBoxd, ezBoundingSphered
  * New Libraries: Core, CoreUtils
  * Added ezLargeBlockAllocator
  * Implemented a proper hashing function for strings
  * Added ezSystemInformation to report CPU core count, Available RAM, etc.
  * Added features to the build system to work with Qt5
  * Added ezThreadSignal for inter-thread synchronization
  * Fixed several bugs.

