#pragma once

#include <EditorTest/EditorTestPCH.h>

#include <EditorTest/TestClass/TestClass.h>

class ezEditorSceneDocumentTest : public ezEditorTest
{
public:
  using SUPER = ezEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_LayerOperations,
    ST_PrefabOperations,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  ezResult CreateSimpleScene(const char* szSceneName);
  void CloseSimpleScene();
  void LayerOperations();
  void PrefabOperations();

private:
  ezScene2Document* m_pDoc = nullptr;
  ezLayerDocument* m_pLayer = nullptr;
  ezUuid m_sceneGuid;
  ezUuid m_layerGuid;
};
