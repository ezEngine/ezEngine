#pragma once

#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <CoreUtils/DataStructures/ObjectMetaData.h>
#include <EditorFramework/Assets/AssetDocument.h>

enum class ActiveGizmo
{
  None,
  Translate,
  Rotate,
  Scale,
  DragToPosition,
};

class ezSceneObjectMetaData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneObjectMetaData, ezReflectedClass);

public:

  enum ModifiedFlags
  {
    HiddenFlag = EZ_BIT(0),
    PrefabFlag = EZ_BIT(1),
    CachedName = EZ_BIT(2),

    AllFlags = 0xFFFFFFFF
  };

  ezSceneObjectMetaData()
  {
    m_bHidden = false;
  }

  bool m_bHidden;
  ezUuid m_CreateFromPrefab;
  ezUuid m_PrefabSeedGuid;
  ezString m_sBasePrefab;
  ezString m_CachedNodeName;
};

class ezSceneDocument : public ezAssetDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneDocument, ezAssetDocument);

public:
  ezSceneDocument(const char* szDocumentPath, bool bIsPrefab);
  ~ezSceneDocument();

  virtual const char* GetDocumentTypeDisplayString() const override { return "Scene"; }

  void SetActiveGizmo(ActiveGizmo gizmo);
  ActiveGizmo GetActiveGizmo() const;

  enum class ShowOrHide
  {
    Show,
    Hide
  };

  void TriggerShowSelectionInScenegraph();
  void TriggerFocusOnSelection(bool bAllViews);
  void TriggerSnapPivotToGrid();
  void TriggerSnapEachObjectToGrid();
  void GroupSelection();
  void DuplicateSelection();
  void ShowOrHideSelectedObjects(ShowOrHide action);
  void ShowOrHideAllObjects(ShowOrHide action);
  void HideUnselectedObjects();
  void UpdatePrefabs();
  void TriggerExportScene();
  void RevertPrefabs(const ezDeque<const ezDocumentObject*>& Selection);

  bool IsPrefab() const { return m_bIsPrefab; }
  ezString GetBinaryTargetFile() const;

  void SetGizmoWorldSpace(bool bWorldSpace);
  bool GetGizmoWorldSpace() const;

  virtual bool Copy(ezAbstractObjectGraph& out_objectGraph) override;
  virtual bool Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition) override;
  bool Duplicate(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph);
  bool Copy(ezAbstractObjectGraph& graph, ezMap<ezUuid, ezUuid>* out_pParents);
  bool PasteAt(const ezArrayPtr<PasteInfo>& info, const ezVec3& vPos);
  bool PasteAtOrignalPosition(const ezArrayPtr<PasteInfo>& info);

  const ezTransform& GetGlobalTransform(const ezDocumentObject* pObject);
  void SetGlobalTransform(const ezDocumentObject* pObject, const ezTransform& t);

  void SetPickingResult(const ezObjectPickingResult& res) { m_PickingResult = res; }

  static ezTransform QueryLocalTransform(const ezDocumentObject* pObject);
  static ezTransform ComputeGlobalTransform(const ezDocumentObject* pObject);

  const ezString& GetCachedPrefabGraph(const ezUuid& AssetGuid);
  ezString ReadDocumentAsString(const char* szFile) const;

  struct SceneEvent
  {
    enum class Type
    {
      ActiveGizmoChanged,
      ShowSelectionInScenegraph,
      FocusOnSelection_Hovered,
      FocusOnSelection_All,
      SnapSelectionPivotToGrid,
      SnapEachSelectedObjectToGrid,
      ExportScene,
      SimulateModeChanged,
    };

    Type m_Type;
  };

  ezEvent<const SceneEvent&> m_SceneEvents;
  ezObjectMetaData<ezUuid, ezSceneObjectMetaData> m_ObjectMetaData;

  ezStatus CreatePrefabDocumentFromSelection(const char* szFile);
  ezStatus CreatePrefabDocument(const char* szFile, const ezDocumentObject* pRootObject, const ezUuid& invPrefabSeed, ezUuid& out_NewDocumentGuid);
  void ReplaceByPrefab(const ezDocumentObject* pRootObject, const char* szPrefabFile, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed);

  bool GetSimulateWorld() const { return m_bSimulateWorld; }
  void SetSimulateWorld(bool b);

protected:
  virtual void InitializeAfterLoading() override;

  template<typename Func>
  void ApplyRecursive(const ezDocumentObject* pObject, Func f)
  {
    f(pObject);

    for (auto pChild : pObject->GetChildren())
    {
      ApplyRecursive<Func>(pChild, f);
    }
  }

  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) override;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph) override;

private:
  void ObjectPropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void ObjectStructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void ObjectEventHandler(const ezDocumentObjectEvent& e);

  void InvalidateGlobalTransformValue(const ezDocumentObject* pObject);

  void UpdatePrefabsRecursive(ezDocumentObject* pObject);
  void UpdatePrefabObject(ezDocumentObject* pObject, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed, const char* szBasePrefab);

  virtual const char* QueryAssetType() const override;

  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) override;

  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform) override;

  virtual ezStatus InternalRetrieveAssetInfo(const char* szPlatform) override;

  virtual ezUInt16 GetAssetTypeVersion() const override;

  virtual ezStatus InternalSaveDocument() override;


  bool m_bIsPrefab;
  bool m_bGizmoWorldSpace; // whether the gizmo is in local/global space mode
  bool m_bSimulateWorld;

  ActiveGizmo m_ActiveGizmo;
  ezObjectPickingResult m_PickingResult;

  ezHashTable<const ezDocumentObject*, ezTransform> m_GlobalTransforms;

  ezMap<ezUuid, ezString> m_CachedPrefabGraphs;

};
