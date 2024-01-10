#pragma once

#include <Core/CoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/World/Declarations.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>


class EZ_CORE_DLL ezCustomData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCustomData, ezReflectedClass);

public:
  virtual void Load(class ezAbstractObjectGraph& graph, class ezRttiConverterContext& context, const class ezAbstractObjectNode* pRootNode);
};


class EZ_CORE_DLL ezCustomDataResourceBase : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCustomDataResourceBase, ezResource);

public:
  ezCustomDataResourceBase();
  ~ezCustomDataResourceBase();

protected:
  virtual void CreateAndLoadData(ezAbstractObjectGraph& graph, ezRttiConverterContext& context, const ezAbstractObjectNode* pRootNode) = 0;
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  ezResourceLoadDesc UpdateContent_Internal(ezStreamReader* Stream, const ezRTTI& rtti);
};


template<typename T>
class ezCustomDataResource : public ezCustomDataResourceBase
{
public:
  ezCustomDataResource();
  ~ezCustomDataResource();

  const T* GetData() const { return GetLoadingState() == ezResourceState::Loaded ? reinterpret_cast<const T*>(m_Data) : nullptr; }

  T* GetData() { return GetLoadingState() == ezResourceState::Loaded ? reinterpret_cast<T*>(m_Data) : nullptr; }

protected:
  virtual void CreateAndLoadData(ezAbstractObjectGraph& graph, ezRttiConverterContext& context, const ezAbstractObjectNode* pRootNode) override;

  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;

  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;

  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  struct alignas(EZ_ALIGNMENT_OF(T))
  {
    ezUInt8 m_Data[sizeof(T)];
  };
};


#define EZ_DECLARE_CUSTOM_DATA_RESOURCE(SELF) \
class SELF##Resource : public ezCustomDataResource<SELF> \
{ \
EZ_ADD_DYNAMIC_REFLECTION(SELF##Resource, ezCustomDataResource<SELF>); \
EZ_RESOURCE_DECLARE_COMMON_CODE(SELF##Resource); \
}; \
\
using SELF##ResourceHandle = ezTypedResourceHandle<SELF##Resource>; \

#define EZ_DEFINE_CUSTOM_DATA_RESOURCE(SELF) \
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(SELF##Resource, 1, ezRTTIDefaultAllocator<SELF##Resource>) \
EZ_END_DYNAMIC_REFLECTED_TYPE; \
\
EZ_RESOURCE_IMPLEMENT_COMMON_CODE(SELF##Resource);\


#include <Core/Utils/Implementation/CustomData_inl.h>
