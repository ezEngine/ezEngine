#pragma once

#include <Core/Basics.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/Timestamp.h>
#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/ResourceManager/ResourceManager.h>

/// \brief The class from which all resource types need to derive.
///
/// Pass the resources own type as the first template parameter.
/// Pass the type name of the resource descriptor struct as the second template parameter.
/// This may be any custom struct that stores the required information for creating a resource.
template<typename SELF, typename SELF_DESCRIPTOR>
class ezResource : public ezResourceBase
{
public:
  typedef SELF_DESCRIPTOR DescriptorType;

  /// \brief Sets the fallback resource that can be used while this resource is not yet loaded.
  ///
  /// By default there is no fallback resource, so all resource will block the application when requested for the first time.
  void SetFallbackResource(const ezResourceHandle<SELF>& hResource)
  {
    m_hFallback = hResource;
    m_Flags.AddOrRemove(ezResourceFlags::ResourceHasFallback, m_hFallback.IsValid());
  }

  static void SetTypeFallbackResource(const ezResourceHandle<SELF>& hResource)
  {
    if (!s_bAddedManagerEventHandler && !s_TypeFallbackResource.IsValid() && !s_TypeMissingResource.IsValid() && hResource.IsValid())
    {
      s_bAddedManagerEventHandler = true;
      ezResourceManager::s_ManagerEvents.AddEventHandler(ManagerEventHandler);
    }

    s_TypeFallbackResource = hResource;
  }

  static void SetTypeMissingResource(const ezResourceHandle<SELF>& hResource)
  {
    if (!s_bAddedManagerEventHandler && !s_TypeFallbackResource.IsValid() && !s_TypeMissingResource.IsValid() && hResource.IsValid())
    {
      s_bAddedManagerEventHandler = true;
      ezResourceManager::s_ManagerEvents.AddEventHandler(ManagerEventHandler);
    }

    s_TypeMissingResource = hResource;
  }

  static const ezResourceHandle<SELF>& GetTypeFallbackResource() { return s_TypeFallbackResource; }
  static const ezResourceHandle<SELF>& GetTypeMissingResource() { return s_TypeMissingResource; }

protected:

  friend class ezResourceManager;

  ezResource(DoUpdate ResourceUpdateThread, ezUInt8 uiQualityLevelsLoadable) : ezResourceBase(ResourceUpdateThread, uiQualityLevelsLoadable)
  {
  }

  ~ezResource() { }

private:
  static void ManagerEventHandler(const ezResourceManager::ManagerEvent& e)
  {
    if (e.m_EventType == ezResourceManager::ManagerEventType::ManagerShuttingDown)
    {
      s_TypeFallbackResource.Invalidate();
      s_TypeMissingResource.Invalidate();
    }
  }

  static bool s_bAddedManagerEventHandler;
  static ezResourceHandle<SELF> s_TypeFallbackResource;
  static ezResourceHandle<SELF> s_TypeMissingResource;

  void CallCreateResource(const SELF_DESCRIPTOR& descriptor)
  {
    ezResourceLoadDesc ld = CreateResource(descriptor);

    EZ_ASSERT_DEV(ld.m_State != ezResourceState::Invalid, "CreateResource() did not return a valid resource load state");
    EZ_ASSERT_DEV(ld.m_uiQualityLevelsDiscardable != 0xFF, "CreateResource() did not fill out m_uiQualityLevelsDiscardable correctly");
    EZ_ASSERT_DEV(ld.m_uiQualityLevelsLoadable != 0xFF, "CreateResource() did not fill out m_uiQualityLevelsLoadable correctly");

    m_LoadingState = ld.m_State;
    m_uiQualityLevelsDiscardable = ld.m_uiQualityLevelsDiscardable;
    m_uiQualityLevelsLoadable = ld.m_uiQualityLevelsLoadable;

    // Update Memory Usage
    {
      ezResourceBase::MemoryUsage MemUsage;
      MemUsage.m_uiMemoryCPU = 0xFFFFFFFF;
      MemUsage.m_uiMemoryGPU = 0xFFFFFFFF;
      UpdateMemoryUsage(MemUsage);

      EZ_ASSERT_DEV(MemUsage.m_uiMemoryCPU != 0xFFFFFFFF, "Resource '%s' did not properly update its CPU memory usage", GetResourceID().GetData());
      EZ_ASSERT_DEV(MemUsage.m_uiMemoryGPU != 0xFFFFFFFF, "Resource '%s' did not properly update its GPU memory usage", GetResourceID().GetData());

      m_MemoryUsage = MemUsage;
    }

    ezResourceManager::ResourceEvent e;
    e.m_pResource = this;
    e.m_EventType = ezResourceManager::ResourceEventType::ResourceContentUpdated;
    ezResourceManager::BroadcastResourceEvent(e);
  }

  /// \brief Override this function to implement resource creation. This is called by ezResourceManager::CreateResource.
  ///
  /// 'Creating' a resource is different from 'loading' a resource. Most resource types are never created. For example a mesh
  /// or a sound might not support being created and thus your implementation for such resources should be empty, except for an assert
  /// that always fires.
  /// However, some resources can be created (or even can only be created, but not loaded). For example textures might support to be
  /// created as render targets. In this case the descriptor needs to provide all the necessary information.
  /// Another example would be a procedurally generated mesh. The descriptor needs to provide all the parameters, such that 'CreateResource'
  /// can set up the resource.
  /// Note that created resources should always set its loading state to 'Loaded' and its current and max quality to 1, otherwise
  /// the resource manager might try to load even more into the resource afterwards.
  /// However, since this might be a valid use case for some resource types, it is not enforced by the resource manager.
  virtual ezResourceLoadDesc CreateResource(const SELF_DESCRIPTOR& descriptor)
  {
    EZ_REPORT_FAILURE("The resource type '%s' does not support resource creation", GetDynamicRTTI()->GetTypeName());

    return ezResourceLoadDesc();
  }

  ezResourceHandle<SELF> m_hFallback;
};

template<typename SELF, typename SELF_DESCRIPTOR>
bool ezResource<SELF, SELF_DESCRIPTOR>::s_bAddedManagerEventHandler = false;

template<typename SELF, typename SELF_DESCRIPTOR>
ezResourceHandle<SELF> ezResource<SELF, SELF_DESCRIPTOR>::s_TypeFallbackResource;

template<typename SELF, typename SELF_DESCRIPTOR>
ezResourceHandle<SELF> ezResource<SELF, SELF_DESCRIPTOR>::s_TypeMissingResource;


