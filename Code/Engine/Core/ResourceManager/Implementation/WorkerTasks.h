#pragma once

#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/UniquePtr.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>

/// \brief [internal] Worker thread/task for loading resources from disk.
class EZ_CORE_DLL ezResourceManagerWorkerDiskRead : public ezTask
{
private:
  friend class ezResourceManager;

  ezResourceManagerWorkerDiskRead() = default;

  static void DoWork(bool bCalledExternally);

  virtual void Execute() override;
};

/// \brief [internal] Worker thread/task for loading on the main thread.
class EZ_CORE_DLL ezResourceManagerWorkerMainThread : public ezTask
{
public:
  ezResourceLoadData m_LoaderData;
  ezResource* m_pResourceToLoad = nullptr;
  ezResourceTypeLoader* m_pLoader = nullptr;
  // this is only used to clean up a custom loader at the right time, if one is used
  // m_pLoader is always set, no need to go through m_pCustomLoader
  ezUniquePtr<ezResourceTypeLoader> m_pCustomLoader;

private:
  friend class ezResourceManager;
  ezResourceManagerWorkerMainThread() = default;

  virtual void Execute() override;
};

