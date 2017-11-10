#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>
#include <EditorFramework/Document/GameObjectDocument.h>

class ezGameObjectDocument;
class ezQtGameObjectDocumentWindow;
class ezDocumentObject;
class ezObjectAccessorBase;
class ezPreferences;
struct ezEngineWindowEvent;
struct ezGameObjectEvent;
struct ezDocumentObjectStructureEvent;
struct ezManipulatorManagerEvent;
struct ezSelectionManagerEvent;
struct ezCommandHistoryEvent;

class EZ_EDITORFRAMEWORK_DLL ezGameObjectGizmoInterface
{
public:
  virtual ezObjectAccessorBase* GetObjectAccessor() = 0;
  virtual bool CanDuplicateSelection() const = 0;
  virtual void DuplicateSelection() = 0;
};

//////////////////////////////////////////////////////////////////////////

enum class ezEditToolSupportedSpaces
{
  LocalSpaceOnly,
  WorldSpaceOnly,
  LocalAndWorldSpace,
};

class EZ_EDITORFRAMEWORK_DLL ezGameObjectEditTool : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameObjectEditTool, ezReflectedClass);

public:
  ezGameObjectEditTool();

  void ConfigureTool(ezGameObjectDocument* pDocument, ezQtGameObjectDocumentWindow* pWindow, ezGameObjectGizmoInterface* pInterface);

  ezGameObjectDocument* GetDocument() const { return m_pDocument; }
  ezQtGameObjectDocumentWindow* GetWindow() const { return m_pWindow; }
  ezGameObjectGizmoInterface* GetGizmoInterface() const { return m_pInterface; }
  bool IsActive() const { return m_bIsActive; }
  void SetActive(bool active);

  virtual ezEditToolSupportedSpaces GetSupportedSpaces() const { return ezEditToolSupportedSpaces::WorldSpaceOnly; }
  virtual bool GetSupportsMoveParentOnly() const { return false; }
  virtual void GetGridSettings(ezGridSettingsMsgToEngine& outGridSettings) {}

protected:
  virtual void OnConfigured() = 0;

private:
  bool m_bIsActive = false;
  ezGameObjectDocument* m_pDocument = nullptr;
  ezQtGameObjectDocumentWindow* m_pWindow = nullptr;
  ezGameObjectGizmoInterface* m_pInterface = nullptr;
};

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORFRAMEWORK_DLL ezGameObjectGizmoEditTool : public ezGameObjectEditTool
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameObjectGizmoEditTool, ezGameObjectEditTool);

public:
  ezGameObjectGizmoEditTool();
  ~ezGameObjectGizmoEditTool();

  void TransformationGizmoEventHandler(const ezGizmoEvent& e);

protected:
  virtual void OnConfigured() override;

  void UpdateGizmoSelectionList();

  void UpdateGizmoVisibleState();
  virtual void ApplyGizmoVisibleState(bool visible) = 0;

  void UpdateGizmoTransformation();
  virtual void ApplyGizmoTransformation(const ezTransform& transform) = 0;

  virtual void TransformationGizmoEventHandlerImpl(const ezGizmoEvent& e) = 0;

  ezDeque<ezSelectedGameObject> m_GizmoSelection;
  bool m_bInGizmoInteraction = false;
  bool m_bMergeTransactions = false;

private:
  void UpdateManipulatorVisibility();
  void GameObjectEventHandler(const ezGameObjectEvent& e);
  void CommandHistoryEventHandler(const ezCommandHistoryEvent& e);
  void SelectionManagerEventHandler(const ezSelectionManagerEvent& e);
  void ManipulatorManagerEventHandler(const ezManipulatorManagerEvent& e);
  void EngineWindowEventHandler(const ezEngineWindowEvent& e);
  void ObjectStructureEventHandler(const ezDocumentObjectStructureEvent& e);
};

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORFRAMEWORK_DLL ezTranslateGizmoEditTool : public ezGameObjectGizmoEditTool
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTranslateGizmoEditTool, ezGameObjectGizmoEditTool);

public:
  ezTranslateGizmoEditTool();
  ~ezTranslateGizmoEditTool();

  virtual ezEditToolSupportedSpaces GetSupportedSpaces() const override { return ezEditToolSupportedSpaces::LocalAndWorldSpace; }
  virtual bool GetSupportsMoveParentOnly() const override { return true; }
  virtual void GetGridSettings(ezGridSettingsMsgToEngine& outGridSettings) override;

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const ezTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const ezGizmoEvent& e) override;

private:
  void OnPreferenceChange(ezPreferences* pref);

  ezTranslateGizmo m_TranslateGizmo;
};

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORFRAMEWORK_DLL ezRotateGizmoEditTool : public ezGameObjectGizmoEditTool
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRotateGizmoEditTool, ezGameObjectGizmoEditTool);

public:
  ezRotateGizmoEditTool();
  ~ezRotateGizmoEditTool();

  virtual ezEditToolSupportedSpaces GetSupportedSpaces() const override { return ezEditToolSupportedSpaces::LocalAndWorldSpace; }
  virtual bool GetSupportsMoveParentOnly() const override { return true; }

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const ezTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const ezGizmoEvent& e) override;

private:
  ezRotateGizmo m_RotateGizmo;
};

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORFRAMEWORK_DLL ezScaleGizmoEditTool : public ezGameObjectGizmoEditTool
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScaleGizmoEditTool, ezGameObjectGizmoEditTool);

public:
  ezScaleGizmoEditTool();
  ~ezScaleGizmoEditTool();

  virtual ezEditToolSupportedSpaces GetSupportedSpaces() const override { return ezEditToolSupportedSpaces::LocalSpaceOnly; }

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const ezTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const ezGizmoEvent& e) override;

private:
  ezScaleGizmo m_ScaleGizmo;
};

//////////////////////////////////////////////////////////////////////////

class EZ_EDITORFRAMEWORK_DLL ezDragToPositionGizmoEditTool : public ezGameObjectGizmoEditTool
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDragToPositionGizmoEditTool, ezGameObjectGizmoEditTool);

public:
  ezDragToPositionGizmoEditTool();
  ~ezDragToPositionGizmoEditTool();

  virtual ezEditToolSupportedSpaces GetSupportedSpaces() const override { return ezEditToolSupportedSpaces::LocalSpaceOnly; }
  virtual bool GetSupportsMoveParentOnly() const override { return true; }

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const ezTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const ezGizmoEvent& e) override;

private:
  ezDragToPositionGizmo m_DragToPosGizmo;
};

