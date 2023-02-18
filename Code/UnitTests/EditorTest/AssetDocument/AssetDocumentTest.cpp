#include <EditorTest/EditorTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorTest/AssetDocument/AssetDocumentTest.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

static ezEditorAssetDocumentTest s_EditorAssetDocumentTest;

const char* ezEditorAssetDocumentTest::GetTestName() const
{
  return "Asset Document Tests";
}

void ezEditorAssetDocumentTest::SetupSubTests()
{
  AddSubTest("Async Save", SubTests::ST_AsyncSave);
  AddSubTest("Save on Transform", SubTests::ST_SaveOnTransform);
}

ezResult ezEditorAssetDocumentTest::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return EZ_FAILURE;

  if (SUPER::OpenProject("Data/UnitTests/EditorTest").Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult ezEditorAssetDocumentTest::DeInitializeTest()
{
  if (SUPER::DeInitializeTest().Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezTestAppRun ezEditorAssetDocumentTest::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_AsyncSave:
      AsyncSave();
      break;
    case SubTests::ST_SaveOnTransform:
      SaveOnTransform();
      break;
  }
  return ezTestAppRun::Quit;
}

void ezEditorAssetDocumentTest::AsyncSave()
{
  ezAssetDocument* pDoc = nullptr;
  ezStringBuilder sName;
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Create Document")
  {
    sName = m_sProjectPath;
    sName.AppendPath("mesh.ezMeshAsset");
    pDoc = static_cast<ezAssetDocument*>(m_pApplication->m_pEditorApp->CreateDocument(sName, ezDocumentFlags::RequestWindow));
    EZ_TEST_BOOL(pDoc != nullptr);
    ProcessEvents();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Save Document")
  {
    // Save doc twice in a row without processing messages and then close it.
    ezDocumentObject* pMeshAsset = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];
    ezObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();
    ezInt32 iOrder = 0;
    ezTaskGroupID id = pDoc->SaveDocumentAsync(
      [&iOrder](ezDocument* pDoc, ezStatus res) {
        EZ_TEST_INT(iOrder, 0);
        iOrder = 1;
      },
      true);

    pAcc->StartTransaction("Edit Mesh");
    EZ_TEST_BOOL(pAcc->SetValue(pMeshAsset, "MeshFile", "Meshes/Cube.obj").Succeeded());
    pAcc->FinishTransaction();

    // Saving while another save is in progress should block. This ensures the correct state on disk.
    ezString sFile = pAcc->Get<ezString>(pMeshAsset, "MeshFile");
    ezTaskGroupID id2 = pDoc->SaveDocumentAsync([&iOrder](ezDocument* pDoc, ezStatus res) {
      EZ_TEST_INT(iOrder, 1);
      iOrder = 2;
    });

    // Closing the document should wait for the async save to finish.
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
    EZ_TEST_INT(iOrder, 2);
    EZ_TEST_BOOL(ezTaskSystem::IsTaskGroupFinished(id));
    EZ_TEST_BOOL(ezTaskSystem::IsTaskGroupFinished(id2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Verify State of Disk")
  {
    pDoc = static_cast<ezAssetDocument*>(m_pApplication->m_pEditorApp->OpenDocument(sName, ezDocumentFlags::None));
    ezDocumentObject* pMeshAsset = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];
    ezObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();
    ezString sFile = pAcc->Get<ezString>(pMeshAsset, "MeshFile");
    EZ_TEST_STRING(sFile, "Meshes/Cube.obj");
  }
  pDoc->GetDocumentManager()->CloseDocument(pDoc);
}

void ezEditorAssetDocumentTest::SaveOnTransform()
{
  ezAssetDocument* pDoc = nullptr;
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Create Document")
  {
    ezStringBuilder sName = m_sProjectPath;
    sName.AppendPath("mesh2.ezMeshAsset");
    pDoc = static_cast<ezAssetDocument*>(m_pApplication->m_pEditorApp->CreateDocument(sName, ezDocumentFlags::RequestWindow));
    EZ_TEST_BOOL(pDoc != nullptr);
    ProcessEvents();
  }

  ezObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();
  const ezDocumentObject* pMeshAsset = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transform")
  {
    pAcc->StartTransaction("Edit Mesh");
    EZ_TEST_BOOL(pAcc->SetValue(pMeshAsset, "MeshFile", "Meshes/Cube.obj").Succeeded());
    pAcc->FinishTransaction();

    ezTransformStatus res = pDoc->SaveDocument();
    EZ_TEST_BOOL(res.Succeeded());

    // Transforming an asset in the background should fail and return NeedsImport as the asset needs to be modified which the background is not allowed to do, e.g. materials need to be created.
    res = ezAssetCurator::GetSingleton()->TransformAsset(pDoc->GetGuid(), ezTransformFlags::ForceTransform | ezTransformFlags::BackgroundProcessing);
    EZ_TEST_BOOL(res.m_Result == ezTransformResult::NeedsImport);

    // Transforming a mesh asset with a mesh reference will trigger the material import and update
    // the materials table which requires a save during transform.
    res = ezAssetCurator::GetSingleton()->TransformAsset(pDoc->GetGuid(), ezTransformFlags::ForceTransform | ezTransformFlags::TriggeredManually);
    EZ_TEST_BOOL(res.Succeeded());
    ProcessEvents();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Verify Transform")
  {
    // Transforming should have update the mesh asset with new material slots.
    ezInt32 iCount = 0;
    EZ_TEST_BOOL(pAcc->GetCount(pMeshAsset, "Materials", iCount).Succeeded());
    EZ_TEST_INT(iCount, 1);

    ezUuid subObject = pAcc->Get<ezUuid>(pMeshAsset, "Materials", (ezInt64)0);
    EZ_TEST_BOOL(subObject.IsValid());
    const ezDocumentObject* pSubObject = pAcc->GetObject(subObject);

    ezString sLabel = pAcc->Get<ezString>(pSubObject, "Label");
    EZ_TEST_STRING(sLabel, "initialShadingGroup");
  }
  pDoc->GetDocumentManager()->CloseDocument(pDoc);
}
