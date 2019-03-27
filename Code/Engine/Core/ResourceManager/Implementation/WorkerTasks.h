#pragma once

#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/UniquePtr.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>

/// \brief [internal] Worker task for loading resources (typically from disk).
class EZ_CORE_DLL ezResourceManagerWorkerDataLoad : public ezTask
{
private:
  friend class ezResourceManager;

  ezResourceManagerWorkerDataLoad();
  ~ezResourceManagerWorkerDataLoad();

  static void DoWork(bool bCalledExternally);

  virtual void Execute() override;
};

/// \brief [internal] Worker task for uploading resource data.
/// Depending on the resource type, this may get scheduled to run on the main thread or on any thread.
class EZ_CORE_DLL ezResourceManagerWorkerUpdateContent : public ezTask
{
public:
  ~ezResourceManagerWorkerUpdateContent();

  ezResourceLoadData m_LoaderData;
  ezResource* m_pResourceToLoad = nullptr;
  ezResourceTypeLoader* m_pLoader = nullptr;
  // this is only used to clean up a custom loader at the right time, if one is used
  // m_pLoader is always set, no need to go through m_pCustomLoader
  ezUniquePtr<ezResourceTypeLoader> m_pCustomLoader;

private:
  friend class ezResourceManager;
  ezResourceManagerWorkerUpdateContent();

  virtual void Execute() override;
};

