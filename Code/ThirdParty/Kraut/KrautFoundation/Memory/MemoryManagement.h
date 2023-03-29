#ifndef AE_FOUNDATION_MEMORY_MEMORYMANAGEMENT_H
#define AE_FOUNDATION_MEMORY_MEMORYMANAGEMENT_H

#include "../Defines.h"
#include "../Math/Math.h"
#include <memory>

namespace AE_NS_FOUNDATION
{
  //! All Memory Management should go through this class.
  class AE_FOUNDATION_DLL aeMemoryManagement
  {
  public:
    static void* Allocate(aeUInt32 uiBytes);
    static void Deallocate(void* pData);

    template <class TYPE>
    static TYPE* Allocate(aeUInt32 uiElements);

    //! Constructs uiElements of TYPE at pInPlace by copy-constructing the elements from pSource.
    template <class TYPE>
    static void CopyConstruct(void* pInPlace, const void* pSource, aeUInt32 uiElements);

    //! Constructs one TYPE at address pInPlace via copy-construction of "init".
    template <class TYPE>
    static void CopyConstruct(void* pInPlace, const TYPE& init);

    //! Constructs uiElements elements of TYPE at pInPlace via default construction.
    template <class TYPE>
    static void Construct(void* pInPlace, aeUInt32 uiElements = 0);

    //! Constructs uiElements elements of TYPE at pInPlace via default construction.
    template <class TYPE>
    static void Construct(void* pInPlace, aeUInt32 uiElements, const TYPE& Init);

    //! Destructs uiElements elements of TYPE at pInPlace.
    template <class TYPE>
    static void Destruct(void* pInPlace, aeUInt32 uiElements = 0);
  };

//! [internal] Do not use.
#define AE_MEMORY_ALLOCATE_NODEBUG(bytes, bNoDebug) aeMemoryManagement::Allocate(bytes)
//! [internal] Do not use.
#define AE_MEMORY_ALLOCATE_TYPE_NODEBUG(type, elements, bNoDebug) aeMemoryManagement::Allocate<type>(elements)
//! [internal] Do not use.
#define AE_MEMORY_DEALLOCATE_NODEBUG(ptr, bNoDebug) aeMemoryManagement::Deallocate(ptr)
} // namespace AE_NS_FOUNDATION

#include "Inline/MemoryManagement.inl"

#endif
