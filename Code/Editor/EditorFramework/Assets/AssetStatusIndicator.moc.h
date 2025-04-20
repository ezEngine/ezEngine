#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <QFrame>

struct ezAssetCuratorEvent;
class ezAssetDocument;
struct ezDocumentEvent;
class QPushButton;

/// \brief A small widget that displays the current transform status of an asset.
///
/// Clicking it allows to re-transform, or see the error log.
class EZ_EDITORFRAMEWORK_DLL ezQtAssetStatusIndicator : public QFrame
{
  Q_OBJECT

public:
  ezQtAssetStatusIndicator(ezAssetDocument* pDoc, QWidget* pParent = nullptr);
  ~ezQtAssetStatusIndicator();

private Q_SLOTS:
  void onClick(bool);
  void onHelp(bool);

private:
  void DocumentEventHandler(const ezDocumentEvent& e);
  void AssetEventHandler(const ezAssetCuratorEvent& e);

  void UpdateDisplay();

  enum class Action
  {
    None,
    Save,
    Transform,
    ShowErrors,
  };

  ezAssetDocument* m_pAsset = nullptr;
  QPushButton* m_pLabel = nullptr;
  QPushButton* m_pHelp = nullptr;
  Action m_Action = Action::None;
};
