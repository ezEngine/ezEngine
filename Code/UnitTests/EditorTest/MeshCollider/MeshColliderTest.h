#pragma once

#include <EditorTest/EditorTestPCH.h>

#include <EditorTest/TestClass/TestClass.h>

class ezEditorMeshColliderTest : public ezEditorTest
{
public:
  using SUPER = ezEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_TriangleMesh,
    ST_ConvexMesh,
    ST_TransferredSettings,
    ST_ExistingCollider,
    ST_PrimitiveMesh,
    ST_KeepsOpenDocuments,
    ST_SimplificationSettings,
    ST_DataDirRelativePath,
    ST_MultipleMeshes,
    ST_SharedSourceFile,
    ST_Surface,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  /// Creates a mesh asset at the given project relative path.
  ///
  /// The values that are set here are the ones the collider is expected to inherit, so that a test
  /// can tell a transferred value from a coincidental default.
  ezUuid CreateMeshAsset(const char* szRelativePath, const char* szSourceFile = "Meshes/Cube.obj", const ezVariantDictionary* pExtraProperties = nullptr);

  /// Copies Cube.obj to a new name, so that a test can use a source file nothing else shares.
  /// Returns the project relative path to use as a mesh asset's MeshFile.
  ezString MakePrivateSourceMesh(const char* szName);

  /// Reads a property from a saved asset document, by opening and closing it.
  ezVariant ReadColliderProperty(ezStringView sAbsPath, ezStringView sProperty);

  void TriangleMesh();
  void ConvexMesh();
  void TransferredSettings();
  void ExistingCollider();
  void PrimitiveMesh();
  void KeepsOpenDocuments();
  void SimplificationSettings();
  void DataDirRelativePath();
  void MultipleMeshes();
  void SharedSourceFile();
  void Surface();
};
