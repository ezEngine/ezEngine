What's New {#WhatsNew}
==========

Milestone 8
-----------

  * Added ezStaticCast and ezDynamicCast for easier and save casting of reflected types
  * Added ezAngle as a supported type to ezVariant
  * Added a random number generator, based on the WELL512 algorithm
  * Added a preliminary PhysX integration
  * Added selection highlight rendering to the editor
  * Added a play-the-game mode to the editor
  * Every ezWorld now has its own clock
  * Removed all global clocks except for one
  * Changed component initialization order a bit to make writing init functions easier
  * Simulation of ezWorlds can now be paused
  * Added ezWorldModule, which allows to hook into the initialization and update of ezWorld, enabling custom plugins like PhysX or Fmod
  * Added shape icon rendering in the editor, which also allows to select objects
  * Improved display of components in the editor (icons etc.)
  * Improved performance when syncing state between editor and engine (sending property diffs only)
  * Added support for Tag properties in the editor
  * Added support for clamping property values in the editor UI
  * Component managers do not need to be explicitly registered anymore
  * Added ezSpawnComponent for spawning prefabs at runtime
  * Added ezTimedDeathComponent for deleting nodes after a timeout
  * Added ezCameraComponent for managing runtime cameras
  * Added ezInputComponent, which sends input event messages to components on the same node
  * Added an editor to configure the input bindings and save it to JSON
  * Refactored ezGameApplication to handle all engine initialization
  * Added ezGameState which is the base class to build all C++ game logic in, works closely with ezGameApplication
  * ezGameApplication now automatically loads plugins for a project through a config file
  * ezGameApplication now automatically configures input bindings for a project through a config file
  * ezGameApplication now automatically sets up data directories for a project through a config file
  * Added a renderpipeline asset for data driven renderer configuration
  * Every ezWorld now has a default random number generator
  * Added ezWorldReader and ezWorldWriter for exporting and importing scene or prefab data in binary form
  * Added ezPrefabResource for binary prefab loading at runtime
  * Editor now supports drag&drop of material assets into the scene
  * Editor now renders bounding boxes of the selection
  * Added a "Duplicate Special" action that allows to make multiple copies of an object with different positions and rotations
  * Editor can now ignore unknown component types. Shows warnings when opening and saving such a scene, but does not crash.
  * Implemented line and point rendering in the D3D abstraction
  * RTTI types can now have attributes, e.g. for for categories in menus
  * Added Surface assets for defining physics properties (fricition etc.)
  * Added orthographic render modes and basic editing functionality (rotate, translate)
  * Added ezPlayer, a stand-alone tool for running scenes
  * Added scene and prefab binary export
  * Added ezDeferredFileWriter for only writing files to disk after everything succeeded
  * Added editor prefabs that can be instantiated in the scene. Changes can be merged into them at a later point.
  * Some aspects of the editor can now be localized
  * Objects can now be hidden in the editor
  * Added a shortcut editing dialog to the editor
  * Added 'settings' components that can be used to configure scene specific properties (e.g. for gravity in PhysX)
  * Lots of render pipeline related work
  * Resource handles can now be serialized easier and more efficiently
  * Added debug break on DX11 errors
  * Better renderdata sorting by separating batch id and sorting key
  * Added "uniform scaling factor" to ezGameObject
  * Implemented proper hashing and re-use of depth stencil, rasterizer and sampler states in GALDevice
  * Added hashing and re-use of shader resource views and render target views
  * Introduced ezSingletonRegistry and two macros to turn a class into a singleton
  * Editor now stores and restores all recently open documents one project loading
  * Added ezStringView, int vector and ezDataBuffer (typedef ezDynamicArray<ezUInt8> ezDataBuffer;) support ezVariant
  * Added support for the PhysX character controller
  * Added guarded memory allocator which can be easily activated by uncommenting the corresponding lines in UserConfig.h
  * Added code to properly do progressbars
  * Added visual manipulators for properties
  * Added visualizers for propertiers
  * Properties can now be hidden, deactivated or get custom tooltips, depending on the selected value
  * Added a tags editor
  * Tags are now automatically loaded by ezGameApplication
  * PhysX meshes can now have a different surface per sub-mesh
  * PhysX integration now supports raycasting
  * New container type ezHashSet
  * Improved support for line rendering
  * Surface assets can now define interactions, e.g. what prefab to spawn when a bullet hits it, etc.
  * ezGeometry now automatically handles flipped winding due to negative determinant in transform
  * Added a 'settings' base component and a simple component manager that allows to easily access the single settings component of a given type
  * Added ezPxSettingsComponent, which currently allows to configure the PhysX world gravity
  * Added a projectile component
  * Added a timed death component, that deletes an entity after a given timeout
  * Improved rendering infrastructure (materials, shaders)
  * Proper tangent generation for geometry
  * Messages can now use a sorting key for ordering
  * GameObjects now also have a unique key for looking up specific objects at runtime
  * Added ezTexConv, a tool to combine textures, generate mipmaps and compress to DXT format
  * Integrated ezTexConv as the new texture asset transformation
  * Editor viewport: Shift+Left Click now opens the picked material asset

Milestone 7
-----------
  * Added solid color texture resources. E.g. use a texture resource with name "CornflowerBlue.color" or "#FF00FF.color" or "FF00FF00.color" and a 4x4 color texture will be generated from memory, instead of read from disk.
  * Added RendererFoundation low-level rendering infrastructure
  * Added RendererCore mid-level rendering infrastructure, see ezRenderContext, ezRenderPipeline, ezRenderPipelinePass, ezView
  * Added DX11 implementation of RendererFoundation
  * Added tools infrastructure
  * Added ezEditor, a general purpose document based editor with undo/redo and out-of-process rendering
  * Added an asset management system
  * Added file redirection through ezDataDirectory::FolderType for asset management
  * Added ezDirectoryWatcher
  * Enhanced the reflection system to handle enums and bitfields
  * Reflection system now supports custom attributes for properties
  * Split up ezStringView and added ezStringIterator
  * Added ezTagRegistry, ezTag and ezTagSet to handle tags
  * Added support for more types to ezVariant
  * Named colors can now be converted back and forth using ezConversionUtils
  * Added some basic infrastructure for node graphs
  * Added GameFoundation library
  * Added some basic components for rotating and translating objects
  * Added rendering resources: ezConstantBufferResource, ezMaterialResource, ezMeshResource, ezShaderResource, ezTextureResource
  * Added (runtime) shader compilation and shader permutation generation
  * Added ezGPUResourcePool for sharing gpu resources
 

Milestone 6
-----------

  * Added ezOBJLoader to load OBJ/MTL files.
  * Renamed ezStringIterator to ezStringView.
  * Added ezGeometry, a class with functions to generate basic geometric shapes.
  * Added ezChunkStreamReader / ezChunkStreamWriter to work with 'chunked' file formats.
  * Added ezArchiveReader / ezArchiveWriter which implement a chunked format that also allows to store and restore reflected types and references between objects.
  * Added ezArrayMap container, which is an associative container that is typically more efficient than ezMap.
  * Added ezResourceManager, which allows to manage resources and stream data. This is still work in progress though.
  * Refactored ezColor a bit. The interface is more streamlined. sRGB features have been moved into separate classes ezColorLinearUB and ezColorGammaUB.
  * ezColorRgba8UNorm has been renamed to ezColorLinearUB (UB == unsigned byte).
  * Removed ezColorBgra8Unorm
  * Added the 140 predefined colors of the CSS specification to ezColor.
  * Added ezDirectoryWatcher. This class allows to watch a directory for file changes. It is currently only implemented on Windows.
  * Added a few utilty functions to ezStringBuilder: Set, SetSubString_FromTo, SetSubString_ElementCount, SetSubString_CharacterCount, RemoveFileExtension
  * Added ezUuid, a class that allows to generate uuid/guid values. Implemented on Windows and Posix systems.
  * Added a CMake file "FindezEngine.cmake" (in "Code\BuildSystem\CMake") which you can use for easier integration of ezEngine into your own CMake based project. For more details see this: \ref HowToUseCMake
  * The CMake builds now allow to configure some settings, that previously needed to be done through the UserConfig.h in Foundation. Use 'EZ_IGNORE_USERCONFIG_HEADER' for this. However, this will only work, when you mirror the same defines in your project setup.

Milestone 5
-----------

  * Added ezJSONParser which is a low level parser that reads JSON incrementally.
  * Added ezJSONReader which generates a high level representation of a JSON document, that can be used to lookup data easily.
  * Added ezExtendedJSONReader which generates a high level representation of a JSON document, but also uses the extended JSON syntax for more precise type information and values.
  * Added ezImage to the CoreUtils library, which allows to easily read / convert / write images of different file types.
    * Supported image file types: BMP, DDS, TGA
  * Added namespace ezIntersectionUtils with functions to do intersection tests.
  * Added ezTransform, a 4x3 matrix that can be used for object transformations that don't require a full 4x4 matrix.
  * Added a function to create a simple embedded ASCII font from code, so no file loading is required to get some simple text on screen.
  * Added ezConsole, a Quake-style ingame console for inspecting the log, modifying cvars and calling exposed functions.
  * Removed ezSharedString. Use ezString or ezHashedString instead.
  * Removed official support for Visual Studio 2010. Might still compile, but we won't ensure that anymore.
  * More extensive usage of C++ 11:
    * ez now uses nullptr directly, instead of a redefined NULL, which solves issues on platforms that already redefined NULL to some compiler specific intrinsic
    * Some container and string classes now support move semantics for improved performance when they are copied around. Note that it is still more efficient to NOT copy such objects around, at all, though.
  * Added ezDataTransfer, which allows ezInspector to pull arbitrary (custom) data from the connected application. This can be used to make screenshots, etc.
  * Improved the profiling system: When GPA is not used, it can now store the profiling history in a ringbuffer in memory and dump it on request. We are currently supporting a JSON file format that can be displayed by Google Chrome using chrome://tracing
  * Added ezTokenizer, a class that allows to easily tokenize a C language like piece of text.
  * The natvis Visual Studio Visualizer now also supports ezStringView, ezHashedString and ezHashTable.
  * GameObject improvements:
    * Parenting works now
    * Transformation propagation to child objects has been implemented
    * Message routing has been improved
    * The different update phases have been fixed
  * Changed some of the macros of the reflection system to be easier to use.
  * Fixed some issues compiling ez on Windows 8 and Windows 8.1
  * Added ezPreprocessor, a class that allows to run a C preprocessor on text and get the result, it supports all the standard features
  * ezVariant now supports more basic types and is more efficient by using move semantics when possible.

  
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

