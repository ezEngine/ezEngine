#pragma once

#include <EditorTest/EditorTestPCH.h>

#include <EditorTest/TestClass/TestClass.h>

class ezEditorMeshPrefabTest : public ezEditorTest
{
public:
  using SUPER = ezEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_SimpleMesh,
    ST_LodMesh,
    ST_BoxCollider,
    ST_CollisionMesh,
    ST_ConvexAndDefaults,
    ST_ExistingPrefab,
    ST_KeepsOpenDocuments,
    ST_DataDirRelativePath,
    ST_MultiplePrefabs,
    ST_AnimatedLodComponent,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  /// Creates a mesh asset at the given project relative path and transforms it, so that its bounds
  /// are recorded.
  ezUuid CreateMeshAsset(const char* szRelativePath, ezUInt8 uiSimplification = 0, const char* szSourceFile = "Meshes/Cube.obj");

  /// Copies Cube.obj to a new name, so that a test can use a source file nothing else shares.
  /// Returns the project relative path to use as a mesh asset's MeshFile.
  ezString MakePrivateSourceMesh(const char* szName);

  void SimpleMesh();
  void LodMesh();
  void BoxCollider();
  void CollisionMesh();
  void ConvexAndDefaults();
  void ExistingPrefab();
  void KeepsOpenDocuments();
  void PrefabDataDirRelativePath();
  void MultiplePrefabs();
  void AnimatedLodComponent();
};
