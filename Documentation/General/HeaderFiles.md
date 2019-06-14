Header Files {#HeaderFiles}
===============

[TOC]

Types of Header Files
-------

The code in ezEngine differenciates between two types of header files:
* **Public Header Files**: Public header files are header files that can be included by third party. These header files should not leak any implementation details like platform headers. A third party is any library or executable outside of the currently compiled library / executable. For example when ezFoundation is compiled, everything else is considered a third party.
* **Internal Header Files**: Internal header files may include platform headers and leak implementation detail but can only be used within a subcomponent of ezEngine (for example only inside ezFoundation). Using them from outside of the component will cause a compiler error.

To mark up a header file as a internal header file first include the components internal.h file and then use the component specific macro. The components internal header file is called `ComponentInternal.h` and the macro is called `EZ_COMPONENT_INTERNAL_HEADER`

The following example shows how to mark a header file as internal for ezFoundation:

~~~c++
#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER
~~~



The Header Checker Tool
-------

The header checker tool will automatically be run by the continues integration to check for leakage of implementation detail. If a leak is found the build will fail. Usually you will see an error message such as:

    Including 'wrl/wrappers/corewrappers.h' in ezEngine/Code/Engine/Foundation/Strings/StringConversion.h:9 leaks underlying implementation detail. Including system or thirdparty headers in public easy header files is not allowed. Please use an interface, factory or pimpl to hide the implementation and avoid the include.

In this example including `wrl/wrappers/corewrappers.h` is illegal. This header file is included from `ezEngine/Code/Engine/Foundation/Strings/StringConversion.h` at line 9. To fix these issues follow one of the techniques below to hide implementation detail.

Hiding Implementation Detail
-------

To consider the different options of hiding implementation detail consider the following example

```c++
#include <d3d11.h>

class ezTexture2D
{
public:
    void Bind();

private:
    ID3D11Texture2D* m_ptr;
};
```

If a user includes this header file the underlying implementation detail is leaked as the user will need the d3d11.h header file in order to compile the code. Furthermore the user might need exactly the same version of the d3d11.h file in order for the code to compile. This is a leaky abstraction. Ideally classes that wrap functionality should not leak any of their implementation details to the user. The following techniques can be used to hide implementation detail.

### Forward Declarations
Forward declarations can be used to remove the need to include a header file, therefor removing the leaky abstraction. Consider the following fixed version of the ezTexture2D class:

```c++
struct ID3D11Texture2D; // Forward declare ID3D11Texture2D

class ezTexture2D
{
public:
    void Bind();

private:
    ID3D11Texture2D* m_ptr;
};
```

This header is no longer a leaky abstraction as the user is no longer required to have a copy of d3d11.h.

Forward declarations can be made for:
* Class or struct members if they are pointers or references.
* All types used as arguments to functions.
* Template arguments if the usage follows the two above rules.

Forward declarations can not be made for:
* Class or struct members that are 'inline' because the compiler needs to know the size and alignment.
* Base classes.

Enums can be forward declared if they are given an explicit storage type. So ideally to make enums forward declarable always manually specify a storage type.

```c++
enum MyEnum : int; // Forward declaration

enum MyEnum : int // declaration
{
    One,
    Two
};
```

Nested types can never be forward declared. A nested type is a type that is inside a class or struct.

```c++
// does not work
// struct Outer::Inner;

struct Outer
{
    struct Inner
    {
        int i;
    };
};
```

So prefer to put nested types into namespaces instead of structs or classes:
```c++
// Forward declaration
namespace Outer
{
    struct Inner;
}

// Declaration
namespace Outer
{
    struct Inner
    {
        int i;
    };
}
```

Templates can also be forward declared:

```c++

// forward declaration
template<typename> struct Example;

// Usage of forward declaration
void bar(const Example<int>& arg);

// declaration
template<typename T>
struct Example
{
    T t;
};
```

#### Advantages
* No runtime overhead

#### Disadvantages
* Forward declarations and actual declaration have to be kept in sync.

### Moving Implementation Detail Out Of Templates

Consider the following example which leaks implementation detail:

```c++
// Application.h

#include <roapi.h>

template <typename AppClass>
void RunApplication(AppClass& app)
{
    RoInitialize(RO_INIT_MULTITHREADED);

    app.Init();

    while(!app.Run()) {}

    app.DeInit();

    RoUninitialize();
}
```

The two functions `RoInitialize` and `RoUninitialize` are platform specific functions and require the include `roapi.h`. We can't move the function into a .cpp because the implementation for templates needs to be known when using them. As a result this template leaks its implementation detail.

To fix this issue we need to wrap the leaking function calls into seperate functions and forward declare these functions.

```c++
// Application.h

void InitPlatform();
void DeInitPlatform();

template <typename AppClass>
void RunApplication(AppClass& app)
{
    InitPlatform();

    app.Init();

    while(!app.Run()) {}

    app.DeInit();

    DeInitPlatform();
}
```

```c++
// Application.cpp
#include "Application.h"
#include <roapi.h>

void InitPlatform()
{
    RoInitialize(RO_INIT_MULTITHREADED);
}

void DeInitPlatform()
{
    RoUninitialize();
}
```

As you can see we removed the include to `roapi.h` from the header file and moved it into the cpp file. This way our header now longer leaks underlying implementation detail, as the user won't see the cpp file when using our library. If considerable parts of the template don't depend on the template arguments this pattern can also be used to reduce code bloat by moving the non dependand parts out into non templated functions.


### Pimpl Light
The pattern that I call "Pimpl light" can be used to hide implementation detail at the cost of an additional allocation:

Consider our original `ezTexture2D` example it would be modified like this:

```c++
// Texture2D.h
class ezTexture2D
{
public:
    ezTexture2D();
    ~ezTexture2D();
    void Bind();

private:
    struct Impl; // forward declration

    ezUniquePtr<Impl> m_pImpl;
};
```

```c++
// Texture2D.cpp
#include "Texture2D.h"
#include <d3d11.h>

// Declaration of ezTexture2D::Impl struct
struct ezTexture2D::Impl
{
    ID3D11Texture2D* m_ptr;
};

ezTexture2D::ezTexture2D()
: m_pImpl(EZ_DEFAULT_NEW(Impl))
{

}

// all constructors / destructors / assigmnet operators must be in .cpp file otherwise forward declaration will not work.
ezTexture2D::~ezTexture2D()
{

}

ezTexture2D::Bind()
{
    // Use the implementation detail
    m_pImpl->m_ptr->Bind();
}
```

This is a easy pattern to hide implementation detail.

#### Advantages
* Simple to implement, hides nasty implementation details well

#### Disadvantages
* Additional allocation
* Additional indirection

### Pimpl Inheritance
The Pimpl pattern can also be implemented by using inheritance instead of a forward declared struct. For our `ezTexture2D` exampel this would look like this:

```c++
// Texture2D.h
class ezTexture2D
{
public:
    ezUniquePtr<ezTexture2D> Make(); // factory function, could also return a shared ptr.
    virtual ~ezTexture2D();
    void Bind();

private:
    ezTexture2D(); // All constructors must be private

    friend class ezTexture2DImpl; // This is the only class allowed to derive from ezTexture2D
};
```

```c++
// Texture2D.cpp
#include "Texture2D.h"
#include <d3d11.h>

// Actual implementation
class ezTexture2DImpl : public ezTexture2D
{
public:
    ezTexture2DImpl() : ezTexture2D() {}
    ~ezTexture2DImpl(){}

    ID3D11Texture2D* m_ptr;
};


ezTexture2D::ezTexture2D() {}
ezTexture2D::~ezTexture2D() {}

ezUniquePtr<ezTexture2D> ezTexture2D::Make()
{
    return ezUniquePtr<ezTexture2D>(EZ_DEFAULT_NEW(ezTexture2DImpl));
}

ezTexture2D::Bind()
{
    // Use the implementation detail
    reinterpret_cast<ezTexture2DImpl*>(this)->m_ptr->Bind();
}
```

As you see this version of pimpl hides the implementation detail similar to pimpl light.

#### Advantages
* No additional indirection (compared to pimpl light)

#### Disadvantages
* Additional allocation
* Can no longer inherit from `ezTexture2D`
* `ezTexture2D` can not be final

### Opaque array of bytes

We can also place an opaque array of bytes large enough to store our implementation detail. Considering our `ezTexture2D` example this would look like this:

```c++
// ezTexture2D.h

class ezTexture2D
{
public:
    void Bind();

private:
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
    struct EZ_ALIGN(Impl, 4)
    {
        ezUInt8 m_Data[4];
    };
#else
    struct EZ_ALIGN(Impl, 8)
    {
        ezUInt8 m_Data[8];
    };
#endif
    Impl m_impl;
};
```

```c++
// ezTexture2D.cpp
#include "Texture2D.h"

struct ezTexture2DImpl
{
    D3D11Texture2D* m_ptr;
};

static_assert(sizeof(ezTexture2D::Impl) == sizeof(ezTexture2DImpl), "ezTexture2D::Impl has incorrect size");
static_assert(alignof(ezTexture2D::Impl) == alignof(ezTexture2DImpl), "ezTexture2D::Impl has incorrect alignment");

void ezTexture2D::Bind()
{
    // Use implementation detail
    reinterpret_cast<ezTexture2DImpl*>(&m_impl)->m_ptr->Bind();
}
```

This again hides the implementation detail in the header file.

#### Advantages
* No runtime overhead

#### Disadvantages
* High maintainance burden. Especially if implementation detail size varies on different platforms.

### Ignore the problem

You can choose to ignore the leaky abstraction issue and tell the header checker tool to ignore a certain file to be included or give a certain file the permission to include anything.

Each module in ezEngine that uses the header checker has a headerCkeckerIgnore.json file where you can add ignores. It looks like this:

```json
{
	"includeTarget" :
	{
		"byName" : [
			"a.h"
		]
	},
	"includeSource" : 
	{
		"byName" : [
			"b.h"
		]
	}
}
```

* In the above file everytime `a.h` is included and would generate an error in the header checker tool, that error will be ignored. 
* Everytime `b.h` includes a header file that would cause an error, this error will also be ignored.

#### Advantages
* Less work

#### Disadvantages
* Longer compile times
* Conflicts due to global namespace polution
* Requires users to have all header files for implementation details available