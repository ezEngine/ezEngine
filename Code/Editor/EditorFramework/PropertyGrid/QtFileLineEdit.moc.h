#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <QLineEdit>

class ezQtFilePropertyWidget;

/// \brief A QLineEdit that is used by ezQtFilePropertyWidget
class EZ_EDITORFRAMEWORK_DLL ezQtFileLineEdit : public QLineEdit
{
  Q_OBJECT

public:
  explicit ezQtFileLineEdit(ezQtFilePropertyWidget* pParent = nullptr);
  virtual void dragMoveEvent(QDragMoveEvent* e) override;
  virtual void dragEnterEvent(QDragEnterEvent* e) override;
  virtual void dropEvent(QDropEvent* e) override;

  ezQtFilePropertyWidget* m_pOwner = nullptr;
};
