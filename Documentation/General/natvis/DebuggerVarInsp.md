Visual Studio debugger variable inspection using natvis {#DebuggerVarInsp}
===============

![Example image for Variable inspection in VS2012 Debugger using provided natvis file](../../../Documentation/General/natvis/natvissample.png)

About natvis
---------------
By default it is very tiresome to view the actual content of some base types like ezDynamicArray or ezString because the interesting values are often hidden in multiple abstraction layer and pointer indirections. In Visual Studio you can use the provided *.natvis file to display most of ezEngine's string and containter types like their equivalent in the standard library.

How to Use
---------------

ezEngine comes with a *.natvis file for Visual Studio. You can find the file under [Utilities/Visual Studio Visualizer/ezEngine.natvis](../../../Utilities/Visual Studio Visualizer/ezEngine.natvis)

The CMake generated solution will automatically reference the natvis file, so it will work out of the box. In case you do not use CMake, you can copy the file manually to "%USERPROFILE%/My Documents/Visual Studio XYZW/Visualizers/"

Have a fun debug session! No restart of Visual Studio needed!

Supported Types
---------------
Some of them are implicit by parent types, more types could be affected.
"Alias" means that the file handles this type explicitly, but has it noted as alternative name for the type listed above

**Strings**
- ezHybridStringBase
    - ezStringBuilder (alias)
    - ezString
    - ezHybridString
- ezStringView
- ezHashedString

**Containers**
- ezArrayBase
    - ezDynamicArray
    - ezDynamicArrayBase
    - ezStaticArray
- ezHashTableBase (partial due to natvis limitations)
    - ezHashTable
- ezListBase
    - ezList
- ezDequeBase
    - ezDeque
- ezMapBase
    - ezSet (alias)
    - ezMap
- ezStaticRingBuffer (wrong element order)
- ezArrayPtr

**Basic Types**
- ezVariant
- ezEnum
- ezBitflags
- ezVec2Template
    - ezVec2
    - ezVec2d
- ezVec3Template
    - ezVec3
    - ezVec3d
- ezVec4Template
    - ezVec4
    - ezVec4d
- ezQuatTemplate
    - ezQuat
    - ezQuatd
- ezPlaneTemplate
    - ezPlane
    - ezPlaned
- ezMat3Template
    - ezMat3
    - ezMat3d
- ezMat4Template
    - ezMat4
    - ezMat4d
- ezColor
- ezTime


natvis_samplecode.cpp
---------------
[Code/Utilities/Visual Studio Visualizer/natvis_samplecode.cpp](../../../Code/Utilities/Visual Studio Visualizer/natvis_samplecode.cpp) contains also sample code that compares debug display of ez-classes with equivalent classes of the C++ standard library.

Extending/writing natvis definitions
---------------
You can find more informations about VS natvis files in the MSDN: [Creating custom views of native objects in the debugger](http://msdn.microsoft.com/en-us/library/vstudio/jj620914.aspx)