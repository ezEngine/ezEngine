#pragma once

#include <Core/CoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/World/Declarations.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>


/// \brief A base class for user-defined data assets.
///
/// Allows users to define their own asset types that can be created, edited and referenced in the editor without writing an editor plugin.
///
/// In order to do that, subclass ezCustomData,
/// and put the macro EZ_DECLARE_CUSTOM_DATA_RESOURCE(YourCustomData) into the header next to your custom type.
/// Also put the macro EZ_DEFINE_CUSTOM_DATA_RESOURCE(YourCustomData) into the implementation file.
///
/// Those will also define resource and resource handle types, such as YourCustomDataResource and YourCustomDataResourceHandle.
///
/// For a full example see SampleCustomData in the SampleGamePlugin.
class EZ_CORE_DLL ezCustomData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCustomData, ezReflectedClass);

public:
  /// \brief Loads the serialized custom data using a robust serialization-based method.
  ///
  /// This function does not need to be overridden. It will work, even if the properties change.
  /// It is only virtual in case you want to hook into the deserialization process.
  virtual void Load(class ezAbstractObjectGraph& ref_graph, class ezRttiConverterContext& ref_context, const class ezAbstractObjectNode* pRootNode);
};

/// \brief Base class for resources that represent different implementations of ezCustomData
///
/// These resources are automatically generated using these macros:
///   EZ_DECLARE_CUSTOM_DATA_RESOURCE(YourCustomData)
///   EZ_DEFINE_CUSTOM_DATA_RESOURCE(YourCustomData)
///
/// Put the former into a header next to YourCustomData and the latter into a cpp file.
///
/// This builds these types:
///   YourCustomDataResource
///   YourCustomDataResourceHandle
///
/// You can then use these to reference this resource type for example in components.
/// For a full example search the SampleGamePlugin for SampleCustomDataResource and SampleCustomDataResourceHandle and see how they are used.
class EZ_CORE_DLL ezCustomDataResourceBase : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCustomDataResourceBase, ezResource);

public:
  ezCustomDataResourceBase();
  ~ezCustomDataResourceBase();

protected:
  virtual void CreateAndLoadData(ezAbstractObjectGraph& ref_graph, ezRttiConverterContext& ref_context, const ezAbstractObjectNode* pRootNode) = 0;
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  ezResourceLoadDesc UpdateContent_Internal(ezStreamReader* Stream, const ezRTTI& rtti);
};

/// \brief Template resource type for sub-classed ezCustomData types.
///
/// See ezCustomDataResourceBase for details.
template <typename T>
class ezCustomDataResource : public ezCustomDataResourceBase
{
public:
  ezCustomDataResource();
  ~ezCustomDataResource();

  /// \brief Provides read access to the custom data type.
  ///
  /// Returns nullptr, if the resource wasn't loaded successfully.
  const T* GetData() const { return GetLoadingState() == ezResourceState::Loaded ? reinterpret_cast<const T*>(m_Data) : nullptr; }

protected:
  virtual void CreateAndLoadData(ezAbstractObjectGraph& graph, ezRttiConverterContext& context, const ezAbstractObjectNode* pRootNode) override;

  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;

  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;

  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  struct alignas(alignof(T))
  {
    ezUInt8 m_Data[sizeof(T)];
  };
};

/// \brief Helper macro to declare a ezCustomDataResource<T> and a matching resource handle
///
/// See ezCustomDataResourceBase for details.
#define EZ_DECLARE_CUSTOM_DATA_RESOURCE(SELF)                              \
  class SELF##Resource : public ezCustomDataResource<SELF>                 \
  {                                                                        \
    EZ_ADD_DYNAMIC_REFLECTION(SELF##Resource, ezCustomDataResource<SELF>); \
    EZ_RESOURCE_DECLARE_COMMON_CODE(SELF##Resource);                       \
  };                                                                       \
                                                                           \
  using SELF##ResourceHandle = ezTypedResourceHandle<SELF##Resource>

/// \brief Helper macro to define a ezCustomDataResource<T>
///
/// See ezCustomDataResourceBase for details.
#define EZ_DEFINE_CUSTOM_DATA_RESOURCE(SELF)                                                 \
  EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(SELF##Resource, 1, ezRTTIDefaultAllocator<SELF##Resource>) \
  EZ_END_DYNAMIC_REFLECTED_TYPE;                                                             \
                                                                                             \
  EZ_RESOURCE_IMPLEMENT_COMMON_CODE(SELF##Resource)


#include <Core/Utils/Implementation/CustomData_inl.h>
