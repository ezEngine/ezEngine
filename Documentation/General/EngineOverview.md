Engine Overview {#EngineOverview}
===============

This document gives an overview of the functionality that ezEngine provides.


Library Structure
-----------------

ezEngine is divided into multiple libraries that provide different functionality.

The most basic and also largest library is **Foundation**. It contains all the basic functionality on which the rest of the engine is built. **Foundation** is meant to be application agnostic. All its features can be used in any kind of application. Most of the platform abstractions are implemented here.

Foundation itself only depends on the **ThirdParty** library, which contains all the third party code that is used in ez.

The **Core** library is built on top of Foundation. This library contains engine specific features, such as the Game Object system. **Core** is where most of the actual engine infrastructure is implemented.

**CoreUtils** builds on top of **Core**. It contains 'nice to have' utility features, but no critical infrastructure. This is where you will find some interesting high-level code.

**System** is the library that is supposed to contain all the high-level platform specific code that might be difficult to abstract. Currently this mostly contains window creation code.

**InputXBox360** implements the XBox 360 controller code. This library can only be built on Windows systems.

**GameUtils** contains some code that might be useful when creating a game. This code is not tested thoroughly and there is no guarantee it is useful or correct. This code is mostly added to implement our own samples.

The **TestFramework** library implements code to manage our unit-tests. You can ignore this entirely.


In general the most interesting libraries to look at are **Foundation**, **Core** and **CoreUtils**. **GameUtils** might also contain some useful stuff, depending on the type of game you want to make.

  
  
  

Library Overview: Foundation
----------------------------

The Foundation library contains all the 'low-level' code that is used throughout the engine.

  

In **Foundation\\Containsers** you will find different types of container classes. These are heavily influenced by the STL, so a lot might look familiar. The most interesting class one should have a look at is ezHybridArray.

  

In **Foundation\\Strings** you will find all the string classes and utilities. ezEngine works with Utf-8 strings everywhere, which makes some things a bit more complicated, but these string classes make it pretty easy. There are utility classes to work with raw C strings and higher-level string classes to create and store strings efficiently.

  

In **Foundation\\Math** you will find lots of math classes, e.g. classes to do vector arithmetic (ezVec3, ezMat4, ezQuat, ezPlane, etc.), classes to work with colors (ezColor, ezColorGammaUB, ezColorLinear16f), classes to work with bounding volumes (ezBoundingBox, ezBoundingSphere) and do culling (ezFrustum), utility functions for intersection tests (ezIntersectionUtils) and a class to work with angles efficiently (ezAngle). There is even an implementation for a fixed point type (ezFixedPoint).

  

In **Foundation\\Time** you will find ezTime, which handles simple time values. Using ezTime::Now() you can access the current application time. To handle game time (e.g. for slow-motion, etc.) use ezClock. For profiling or timing ezStopwatch is available. And finally for system-independent timestamps, which might be useful when working with file modification times, ezTimestamp and ezDateTime are provided.

  

In **Foundation\\Threading** you will find functionality that is useful for threading. ezAtomicInteger provides lock-free integer values. ezLock and ezMutex implement the standard mechanisms for working with critical sections. ezThread is a platform independent implementation for threads and ezThreadSignal allows to send signals to other threads. ezThreadLocalStorage and ezThreadLocalPointer provide mechanisms to store thread local data. ezThreadUtils provides some of the low-level threading functions, such as ezThreadUtils::Sleep().
However, before you go ahead and create your own threads, you should have a look at ezTaskSystem, which is a thread-pool implementation that efficiently distributes tasks across multiple worker threads. It supports dependencies across tasks, different priorities, waiting/blocking for unfinished tasks, task canceling and load-balancing when tasks run over multiple frames. There should be only very few situtations where a task is not good enough and a custom thread is necessary.

  

In **Foundation\\Logging** you will find ezLog, the central class to output log information. There are various ways logging information can be output. ezLogWriter::HTML allows to write the data to an HTML file, ezLogWriter::Console and ezLogWriter::VisualStudio output the data to different console windows. Additionally the ezInspector tool will display all logged data as well. The logging system is extremely useful to get an insight into what your application is doing and what might be going wrong, so we suggest setting this up early and using it to log most of what your application is doing.

  

In **Foundation\\Algorithm** you will find some useful algorithms, mostly for sorting, searching and hashing.

  

In **Foundation\\Basics** you can find a lot of platform specific code, most of which might not be very interesting. You will also find EZ_ASSERT which you should be using frequently.

  

In **Foundation\\Basics\\Types** you will find some fundamental types that are used frequently in ez. ezDelegate is often used for callbacks. ezEnum is used for type-safe enum types and ezBitflags is used for type-safe and easy to use bitflags. ezArrayPtr is a 'fat pointer' that stores the start and length of an array. Finally ezVariant is a type that can store different types of data (float, int, string, vector, etc.) and knows which type it has stored. It can do conversions between related types and is often used in message passing.

  

In **Foundation\\Communication** you can find functionality to communicate with other code. ezEvent is a frequently used class to raise events and thus inform other code of changes. ezMessage is used for message passing, which is quite often used with the game object system (which you will find in the **Core** library). ezTelemetry is a system to broadcast information from the running application to other applications, usually tools for introspection, such as ezInspector.

  

In **Foundation\\Configuration** you will find tools to configure the application. ezStartup is a system to (de-)initialize different subsystems in your code in the right order. ezPlugin is used when you want to extend your application using plugin DLLs dynamically at runtime. ezStartup helps with this as well.
ezCVar is a 'configuration variable' that allows to easily change the state of the running application. Its state can be stored on disk and it can be modified either through an ezConsole or remotely through ezInspector. This allows to have lots of 'debug modes' that can be enabled on demand without recompilation or complicated integration into the input handling.

  

In **Foundation\\Utilties** you will find some utility functionality, such as ezCommandLineUtils for command-line argument parsing, ezConversionUtils for string / number conversions and ezStats for broadcasting the state of some internal code, which is useful for debugging game code.

  

In **Foundation\\IO** you will find lots of functionality for reading and writing data.

ezStreamReaderBase and ezStreamWriterBase are the interfaces for all IO operations. Derived from these classes you will find ezMemoryStreamReader / ezMemoryStreamWriter for working with data in-memory. ezCompressedStreamReader and ezCompressedStreamWriter allow to zip and unzip data and ezChunkStreamWriter / ezChunkStreamReader implement a 'chunked' format that can be used for building file formats that another application may not fully understand. Finally ezArchiveReader / ezArchiveWriter implement a high-level file format container for serializing complex objects.

ezOSFile is the low-level file abstraction, in most cases you should not need to work with this. Instead prefer ezFileSystem which adds functionality for virtual file systems through mount points. For example a compressed file or a remote folder may be mounted as a read-only directory. ezFileSystem is the central class to manage file accesses, but to actually read or write a file, use ezFileReader and ezFileWriter, which also implement the ezStream* interface.

To store data in a structured way, ezJSONWriter and ezExtendedJSONWriter are provided. For convenient retrieval ezJSONReader and ezExtendedJSONReader are available. For less convenient but more flexible and efficient JSON reading you can also use ezJSONParser.

In **Foundation\\Reflection** you will find the reflection system of the ezEngine. This is used by the game objects and some other high-level code for object type identification and properties. This may be used for scripting, for setting up objects from configuration files and for editors. Most notable classes are ezRTTI and ezReflectedClass.



Library Overview: Core
----------------------

In **Core** you will find the core engine infrastructure.


In **Core\\Application** you can find code to more easily set up your application loop in a platform independent way.

In **Core\\Input** you will find ezInputManager which can be used for retrieving input from various different devices, e.g. mouse, keyboard, gamepad or virtual thumbstick. The system is easily extensible to include custom devices.

In **Core\\ResourceManager** you will find the static class ezResourceManager which is the central class for resource loading (e.g. textures, shaders, etc.). For implementing custom resource types you need to derive from ezResource and for customizing the loading procedure you may need to implement a custom ezResourceTypeLoader. All resources are referenced through ezResourceHandle types, which implement reference counting.

In **Core\\World** you will find the game object system. ezGameObject is the class to use to manage entities, ezComponent is the base component class that allows to implement and attach components to your entities. All entities belong to an instance of ezWorld, which represents your scene graph.


Library Overview: CoreUtils
---------------------------

In **CoreUtils\\CodeUtils** you can find utitlies to work with code or text, such as ezTokenizer. You will also find a full implementation of a C preprocessor (ezPreprocessor).

**CoreUtils\\Console** contains code for a Quake-like ingame console that can be used for changing the game configuration (through ezCVar or custom functions) and to see the ezLog output.

**CoreUtils\\DataStructures** contains data structures that are too engine specific to be put into **Foundation**. Here you will find such things as octree and quadtree implementations.

In **CoreUtils\\Debugging** you can find tools to ease debugging. ezDataTransfer allows to broadcast custom data from your application to outside tools, such as ezInspector.

In **CoreUtils\\Geometry** you will find code to load certain formats (e.g. ezOBJLoader) or create geometric objects (ezGeometry).

In **CoreUtils\\Graphics** you can find a class to handle camera movements in 3D (ezCamera). ezGraphicsUtils provides additional utility functions, e.g. creation of a very simple font texture through code.

In **CoreUtils\\Image** you will find ezImage which can be used to read, write and convert images from various formats.

In **CoreUtils\\Scripting** you can find ezLuaWrapper that allows to easily work with Lua scripts.
