#pragma once

#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/IPC/IPCObjectMirrorEditor.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>

class QWidget;
class QHBoxLayout;
class QPushButton;
class ezQtEngineViewWidget;
class ezAssetDocument;
class ezEditorEngineDocumentMsg;
struct ezObjectPickingResult;
struct ezEngineViewConfig;
struct ezCommonAssetUiState;


struct EZ_EDITORFRAMEWORK_DLL ezEngineWindowEvent
{
  enum class Type
  {
    ViewCreated,
    ViewDestroyed,
  };

  Type m_Type;
  ezQtEngineViewWidget* m_pView = nullptr;
};

/// \brief Base class for all document windows that need a connection to the engine process, and might want to render 3D content.
///
/// This class has an ezEditorEngineConnection object for sending messages between the editor and the engine process.
/// It also allows to embed ezQtEngineViewWidget objects into the UI, which enable 3D rendering by the engine process.
class EZ_EDITORFRAMEWORK_DLL ezQtEngineDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtEngineDocumentWindow(ezAssetDocument* pDocument);
  virtual ~ezQtEngineDocumentWindow();

  ezEditorEngineConnection* GetEditorEngineConnection() const;
  const ezObjectPickingResult& PickObject(ezUInt16 uiScreenPosX, ezUInt16 uiScreenPosY, ezQtEngineViewWidget* pView) const;

  ezAssetDocument* GetDocument() const;

  /// \brief Returns the ezQtEngineViewWidget over which the mouse currently hovers
  ezQtEngineViewWidget* GetHoveredViewWidget() const;

  /// \brief Returns the ezQtEngineViewWidget that has the input focus
  ezQtEngineViewWidget* GetFocusedViewWidget() const;

  ezQtEngineViewWidget* GetViewWidgetByID(ezUInt32 uiViewID) const;

  ezArrayPtr<ezQtEngineViewWidget* const> GetViewWidgets() const;

  void AddViewWidget(ezQtEngineViewWidget* pView);

  virtual void CreateImageCapture(const char* szOutputPath) override;

public:
  mutable ezEvent<const ezEngineWindowEvent&> m_EngineWindowEvent;

protected:
  friend class ezQtEngineViewWidget;
  ezHybridArray<ezQtEngineViewWidget*, 4> m_ViewWidgets;

  virtual void CommonAssetUiEventHandler(const ezCommonAssetUiState& e);

  virtual void ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg);
  void RemoveViewWidget(ezQtEngineViewWidget* pView);
  void DestroyAllViews();
  virtual void InternalRedraw() override;
};
