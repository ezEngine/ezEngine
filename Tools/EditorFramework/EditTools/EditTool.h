#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Document/GameObjectDocument.h>

class ezGameObjectDocument;
class ezQtGameObjectDocumentWindow;
class ezObjectAccessorBase;
class ezEditorInputContext;

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

  virtual ezEditorInputContext* GetEditorInputContextOverride() { return nullptr; }
  virtual ezEditToolSupportedSpaces GetSupportedSpaces() const { return ezEditToolSupportedSpaces::WorldSpaceOnly; }
  virtual bool GetSupportsMoveParentOnly() const { return false; }
  virtual void GetGridSettings(ezGridSettingsMsgToEngine& outGridSettings) {}

protected:
  virtual void OnConfigured() = 0;
  virtual void OnActiveChanged(bool bIsActive) {}

private:
  bool m_bIsActive = false;
  ezGameObjectDocument* m_pDocument = nullptr;
  ezQtGameObjectDocumentWindow* m_pWindow = nullptr;
  ezGameObjectGizmoInterface* m_pInterface = nullptr;
};

