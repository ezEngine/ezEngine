#pragma once

#include <EditorTest/EditorTestPCH.h>

#include <EditorTest/TestClass/TestClass.h>

// class ezMaterialAssetDocument;

class ezMaterialDocumentTest : public ezEditorTest
{
public:
  using SUPER = ezEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_CreateNewMaterialFromShader,
    ST_CreateNewMaterialFromBase,
    ST_CreateNewMaterialFromVSE,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  ezResult CreateMaterial(const char* szSceneName);
  void CloseMaterial();
  const ezDocumentObject* GetShaderProperties(const ezDocumentObject* pMaterialProperties);
  void CaptureMaterialImage();

  void CreateMaterialFromShader();
  void CreateMaterialFromBase();
  void CreateMaterialFromVSE();

private:
  ezAssetDocument* m_pDoc = nullptr;
  ezUuid m_MaterialGuid;
};
