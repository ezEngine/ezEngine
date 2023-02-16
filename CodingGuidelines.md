# Coding Guidelines

The coding guidelines in ezEngine are enforced through clang-tidy. You can either run clang-tidy locally or use the automated CI process that runs in every PR into ezEngine. CI will provide a git patch with all suggested changes which you can apply locally. After applying the suggested changes please make sure everything still compiles. The fixes done by clang-tidy are not garantueed to work in all cases.

## Type dependent prefixes 

These type dependent prefix is currently only mandatory for member variables. Function / Method parameter names and variables in Function / Method bodies can be named to the programmers liking.

 * If the variable / member is a `ezInt8`, `ezInt16`, `ezInt32`, `ezInt64`, `ezAtomicInteger32`, `ezAtomicInteger64`, `ptrdiff_t` or any other signed integer type the prefix is 'i': `ezInt32 iMyVar;`
 * If the variable / member is a `ezUint8`, `ezUInt16,` `ezUInt32`, `ezUInt64`, `size_t` or any other unsigned integer type the prefix is 'ui': `ezUInt32 uiMyVar;`
 * If the variable / member is a `float` or `double` the prefix is 'f': `float fMyVar;`
 * If the variable / member is a `bool` or `ezAtomicBool` the prefix is 'b': `bool bMyVar;`
 * If the variable / member is a handle, the prefix is 'h': `ezSpatialDataHandle hMyVar;`
 * If the variable / member is a raw pointer, `ezSharedPtr`,`ezUniquePtr`,`std::shared_ptr`,`std::unique_ptr` or `QPointer` the prefix is 'p': `ezUInt32* pMyVar;`
 * If the variable / member is a `const char*` the prefix is 'sz' if it represent a zero terminated string, 'p' otherwise: `const char* szMyVar;`
 * If the variable / member is a ezEngine string (`ezString`, `ezStringView`, etc) the prefix is 's': `ezString sMyVar;`
 * If the variable / member is a ezEngine vector (`ezVec3`, `ezVec4`, `ezSimdVec4f`, etc) the prefix is 'v': `ezVec3 vMyVar;`
 * If the variable / member is a ezEngine quaternion (`ezQuat`, `ezQuatd`, `ezSimdQuat`) the prefix is 'q': `eqQuat qMyVar;`
 * If the variable / member is a ezEngine matrix (`ezMat3`, `ezMat4`, `ezSimdMat4f`, etc) the prefix is 'm': `ezMat3 mMyVar;`
 * If the variable / member is a fixed size array the prefix can be choosen freely. E.g. `bool m_bSomeBools[3];` or `bool m_SomeBools[3];`
 * In all other cases no prefix should be used.

## Members of structs and Classes

### Non-Static

If the member is public, no rules apply.

For private and protected members, the following rules apply:
 * All members must start with 'm_' (this comes before the type dependent prefix)
 * The name of the static member must be in PascalCase: `m_MyMember` 



 ### Static members
If the member is public, no rules apply.

For private and protected members, the following rules apply:
 * If the member is a constant, it should be marked `constexpr`: `static constexpr ezInt32 MyConstant = 5;`
 * Otherwise the member must start with 's_' (this comes before the type dependent prefix)
 * The name of the static member must be in PascalCase: `s_MyMember` 
