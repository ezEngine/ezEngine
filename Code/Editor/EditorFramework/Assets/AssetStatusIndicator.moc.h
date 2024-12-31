#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <QFrame>

struct ezAssetCuratorEvent;
class ezAssetDocument;
struct ezDocumentEvent;
class QPushButton;

class EZ_EDITORFRAMEWORK_DLL ezQtAssetStatusIndicator : public QFrame
{
  Q_OBJECT

public:
  ezQtAssetStatusIndicator(ezAssetDocument* pDoc, QWidget* pParent = nullptr);
  ~ezQtAssetStatusIndicator();

private Q_SLOTS:
  void onClick(bool);

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
  Action m_Action = Action::None;
};
