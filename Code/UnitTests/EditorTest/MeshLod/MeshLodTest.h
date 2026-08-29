#pragma once

#include <EditorTest/EditorTestPCH.h>

#include <EditorTest/TestClass/TestClass.h>

class ezEditorMeshLodTest : public ezEditorTest
{
public:
  using SUPER = ezEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_SimplificationLadder,
    ST_CreateLods,
    ST_ContinuesFromSimplifiedBase,
    ST_TransferredSettings,
    ST_ExistingLods,
    ST_PrefabPicksThemUp,
    ST_PrimitiveMesh,
    ST_MultipleMeshes,
    ST_SubMeshVariantsDoNotShare,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  /// Creates a mesh asset at the given project relative path.
  ezUuid CreateMeshAsset(const char* szRelativePath, const char* szSourceFile = "Meshes/Cube.obj", const ezVariantDictionary* pExtraProperties = nullptr);

  /// Copies Cube.obj to a new name, so that a test can use a source file nothing else shares.
  /// Returns the project relative path to use as a mesh asset's MeshFile.
  ezString MakePrivateSourceMesh(const char* szName);

  /// Reads a property from a saved asset document, by opening and closing it.
  ezVariant ReadAssetProperty(ezStringView sAbsPath, ezStringView sProperty);

  void SimplificationLadder();
  void CreateLods();
  void ContinuesFromSimplifiedBase();
  void SubMeshVariantsDoNotShare();
  void TransferredSettings();
  void ExistingLods();
  void PrefabPicksThemUp();
  void PrimitiveMesh();
  void MultipleMeshes();
};
