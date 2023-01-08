#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Threading/TaskSystem.h>


struct ezAssetCuratorEvent;
class ezTask;
struct ezAssetInfo;

/// \brief Asset table class. Persistent cache for an asset table.
///
/// The following assumptions need to be true for this cache to work:
/// 1. ezAssetDocumentManager::AddEntriesToAssetTable does never change over time
/// 2. ezAssetDocumentManager::GetAssetTableEntry never changes over the lifetime of an asset.
struct ezAssetTable
{
  ezString m_sDataDir;
  ezString m_sTargetFile;
  const ezPlatformProfile* m_pProfile;
  bool m_bDirty = true;
  bool m_bReset = true;
  ezMap<ezString, ezString> m_GuidToPath;

  ezResult WriteAssetTable();
  void Remove(const ezSubAsset& subAsset, ezStringBuilder& sTemp);
  void Update(const ezSubAsset& subAsset, ezStringBuilder& sTemp);
};

/// \brief 
class EZ_EDITORFRAMEWORK_DLL ezAssetTableWriter
{
public:
  ezAssetTableWriter(const ezApplicationFileSystemConfig& fileSystemConfig);
  ~ezAssetTableWriter();

  /// \brief Needs to be called every frame. Handles update delays to allow compacting multiple changes.
  void MainThreadTick();

  /// \brief Marks an asset that needs to be reloaded in the engine process.
  /// The requests are batched and sent out via MainThreadTick.
  void NeedsReloadResource(const ezUuid& assetGuid);

  /// \brief
  ezResult WriteAssetTables(const ezPlatformProfile* pAssetProfile, bool bForce);

private:
  void AssetCuratorEvents(const ezAssetCuratorEvent& e);
  ezAssetTable* GetAssetTable(ezUInt32 uiDataDirIndex, const ezPlatformProfile* pAssetProfile);
  ezUInt32 FindDataDir(const ezSubAsset& asset);

private:
  struct ReloadResource
  {
    ezUInt32 m_uiDataDirIndex;
    ezString m_sResource;
    ezString m_sType;
  };

private:
  ezApplicationFileSystemConfig m_FileSystemConfig;
  ezDynamicArray<ezString> m_DataDirRoots;

  mutable ezCuratorMutex m_AssetTableMutex;
  bool m_bTablesDirty = true;
  bool m_bNeedToReloadResources = false;
  ezTime m_NextTableFlush;
  ezDynamicArray<ReloadResource> m_ReloadResources;
  ezDeque<ezMap<const ezPlatformProfile*, ezUniquePtr<ezAssetTable>>> m_DataDirToAssetTables;
};
