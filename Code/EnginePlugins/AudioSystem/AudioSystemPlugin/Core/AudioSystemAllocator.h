#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Memory/CommonAllocators.h>

/// \brief Default memory alignment used in the audio system.
#define EZ_AUDIOSYSTEM_MEMORY_ALIGNMENT 16

/// \brief An allocator that uses the heap to allocate memory for use by the audio system itself.
/// Audio middleware implementations are not recommended to use this allocator. This allocator will
/// be registered in the ezSingletonRegistry at initialization.
class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioSystemAllocator : public ezAlignedHeapAllocator
{
  EZ_DECLARE_SINGLETON(ezAudioSystemAllocator);

public:
  ezAudioSystemAllocator();
};

/// \brief An allocator that uses the heap to allocate memory for use by the audio middleware.
/// Audio middleware implementations should use this allocator to manage memory. This allocator will
class EZ_AUDIOSYSTEMPLUGIN_DLL ezAudioMiddlewareAllocator : public ezAlignedHeapAllocator
{
  EZ_DECLARE_SINGLETON(ezAudioMiddlewareAllocator);

public:
  /// \brief Constructor.
  /// \param pParentAllocator An instance to the AudioSystemAllocator which will act as the parent of this allocator.
  explicit ezAudioMiddlewareAllocator(ezAudioSystemAllocator* pParentAllocator);
};

struct ezAudioSystemAllocatorWrapper
{
  EZ_ALWAYS_INLINE static ezAllocatorBase* GetAllocator()
  {
    return ezAudioSystemAllocator::GetSingleton();
  }
};

struct ezAudioMiddlewareAllocatorWrapper
{
  EZ_ALWAYS_INLINE static ezAllocatorBase* GetAllocator()
  {
    return ezAudioMiddlewareAllocator::GetSingleton();
  }
};

/// \brief Creates a new instance of type using the audio system allocator.
#define EZ_AUDIOSYSTEM_NEW(type, ...) EZ_NEW(ezAudioSystemAllocatorWrapper::GetAllocator(), type, __VA_ARGS__)

/// \brief Deletes the instance stored in ptr using the audio system allocator and sets ptr to nullptr.
#define EZ_AUDIOSYSTEM_DELETE(ptr) EZ_DELETE(ezAudioSystemAllocatorWrapper::GetAllocator(), ptr)

/// \brief Creates a new array of type using the audio system allocator with count elements, calls default constructor for non-POD types
#define EZ_AUDIOSYSTEM_NEW_ARRAY(type, count) EZ_NEW_ARRAY(ezAudioSystemAllocatorWrapper::GetAllocator(), type, count)

/// \brief Calls destructor on every element for non-POD types and deletes the array stored in arrayPtr using the audio system allocator
#define EZ_AUDIOSYSTEM_DELETE_ARRAY(arrayPtr) EZ_DELETE_ARRAY(ezAudioSystemAllocatorWrapper::GetAllocator(), arrayPtr)

/// \brief Creates a raw buffer of type using the audio system allocator with count elements, but does NOT call the default constructor
#define EZ_AUDIOSYSTEM_NEW_RAW_BUFFER(type, count) EZ_NEW_RAW_BUFFER(ezAudioSystemAllocatorWrapper::GetAllocator(), type, count)

/// \brief Deletes a raw buffer stored in ptr using the audio system allocator, but does NOT call destructor
#define EZ_AUDIOSYSTEM_DELETE_RAW_BUFFER(ptr) EZ_DELETE_RAW_BUFFER(ezAudioSystemAllocatorWrapper::GetAllocator(), ptr)
