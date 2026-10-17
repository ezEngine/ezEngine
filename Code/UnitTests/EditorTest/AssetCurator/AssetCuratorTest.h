#pragma once

#include <EditorTest/EditorTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorTest/TestClass/TestClass.h>

class ezAssetDocument;

/// Tests for the transform state, hashing and dependency handling of the asset curator.
class ezEditorAssetCuratorTest : public ezEditorTest
{
public:
  using SUPER = ezEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_PackageDependencyChange,
    ST_PackageDependencyBroken,
    ST_ErrorClearsWhenFixed,
    ST_RepeatedTransformError,
    ST_CircularPackageDependencies,
    ST_CircularTransformDependencies,
    ST_MixedDependencyCycle,
    ST_CollectionUnknownEntry,
    ST_CollectionSceneCycle,
    ST_BrokenAssetInPackageCycle,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  void PackageDependencyChange();
  void PackageDependencyBroken();
  void ErrorClearsWhenFixed();
  void RepeatedTransformError();
  void CircularPackageDependencies();
  void CircularTransformDependencies();
  void MixedDependencyCycle();
  void CollectionUnknownEntry();
  void CollectionSceneCycle();
  void BrokenAssetInPackageCycle();

  /// The cached curator data of an asset, read without triggering a re-computation.
  struct AssetSnapshot
  {
    bool m_bValid = false;
    ezAssetInfo::TransformState m_State = ezAssetInfo::TransformState::Unknown;
    ezUInt64 m_uiAssetHash = 0;
    ezUInt64 m_uiThumbHash = 0;
    ezUInt64 m_uiPackageHash = 0;
    ezUInt8 m_uiLastStateUpdate = 0;
  };

  /// Creates a document in the test project. All documents created this way are closed at the end of the sub-test.
  ezAssetDocument* CreateAsset(ezStringView sFileName, bool bRequestWindow = false);
  void CloseAsset(ezAssetDocument* pDoc);
  void CloseAllAssets();

  void SetProperty(ezAssetDocument* pDoc, ezStringView sProperty, const ezVariant& value);
  void AddCollectionEntry(ezAssetDocument* pDoc, ezStringView sAsset);

  /// Saves the document and runs the transform that saving an asset schedules for the end of the frame.
  void SaveAsset(ezAssetDocument* pDoc);
  void RunFrameTasks();

  ezTransformStatus TransformAsset(const ezUuid& assetGuid);
  /// Returns the state the curator computes, re-computing it if it is stale.
  ezAssetInfo::TransformState UpdateState(const ezUuid& assetGuid, bool bForce = false);
  AssetSnapshot GetSnapshot(const ezUuid& assetGuid);

  static ezString ToString(const ezUuid& guid);

  ezHybridArray<ezAssetDocument*, 8> m_Assets;
};
