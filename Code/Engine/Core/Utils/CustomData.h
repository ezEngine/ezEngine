#pragma once

#include <Core/CoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/World/Declarations.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Core/Assets/AssetFileHeader.h>


class EZ_CORE_DLL ezCustomData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCustomData, ezReflectedClass);

public:
  virtual void Load(ezStreamReader& inout_stream);
  virtual void Save(ezStreamWriter& inout_stream) const;
};

template<typename T>
class ezCustomDataResource : public ezResource
{
  EZ_RESOURCE_DECLARE_CREATEABLE(ezCustomDataResource<T>, T);

public:
  ezCustomDataResource();
  ~ezCustomDataResource();

  T& GetData() { return m_Data; }
  const T& GetData() const { return m_Data; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:

  T m_Data;
};

#define EZ_DECLARE_CUSTOM_DATA(SELF) \
  class SELF##Resource : public ezCustomDataResource<SELF> \
  { \
  EZ_ADD_DYNAMIC_REFLECTION(SELF##Resource, ezCustomDataResource<SELF>); \
  EZ_RESOURCE_DECLARE_COMMON_CODE(SELF##Resource); \
  }; \
  \
  using SELF##ResourceHandle = ezTypedResourceHandle<SELF##Resource>; \

#define EZ_DEFINE_CUSTOM_DATA(SELF) \
  EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(SELF##Resource, 1, ezRTTIDefaultAllocator<SELF##Resource>) \
  EZ_END_DYNAMIC_REFLECTED_TYPE; \
  \
  EZ_RESOURCE_IMPLEMENT_COMMON_CODE(SELF##Resource);\


#include <Core/Utils/Implementation/IntervalScheduler_inl.h>
