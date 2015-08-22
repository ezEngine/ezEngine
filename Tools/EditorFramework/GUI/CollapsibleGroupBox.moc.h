#pragma once

#include <Plugin.h>
#include <QGroupBox>
#include <QLayout>
#include <Tools/EditorFramework/ui_CollapsibleGroupBox.h>
#include <EditorFramework/GUI/PropertyEditorBaseWidget.moc.h>

class EZ_EDITORFRAMEWORK_DLL ezCollapsibleGroupBox : public QWidget, public Ui_CollapsibleGroupBox
{
  Q_OBJECT

public:
  explicit ezCollapsibleGroupBox(QWidget* pParent, bool bShowElementButtons = false);

  void setTitle(QString sTitle);

  void setInnerLayout(QLayout* pLayout) { Content->setLayout(pLayout); }

  enum class ElementAction
  {
    MoveElementUp,
    MoveElementDown,
    DeleteElement,
  };

signals:
  void ElementActionTriggered(ElementAction a);

private slots:
  virtual void on_MoveUp_clicked() { emit ElementActionTriggered(ElementAction::MoveElementUp); }
  virtual void on_MoveDown_clicked() { emit ElementActionTriggered(ElementAction::MoveElementDown); }
  virtual void on_Delete_clicked() { emit ElementActionTriggered(ElementAction::DeleteElement); }

protected:
  virtual bool eventFilter(QObject* object, QEvent* event) override;

};


class EZ_EDITORFRAMEWORK_DLL ezElementGroupBox : public ezCollapsibleGroupBox
{
  Q_OBJECT

public:
  ezElementGroupBox(QWidget* pParent);

  void SetItems(const ezHybridArray<ezPropertyEditorBaseWidget::Selection, 8>& items, const ezPropertyPath& path);

private slots:
  virtual void on_MoveUp_clicked();
  virtual void on_MoveDown_clicked();
  virtual void on_Delete_clicked();

private:
  void Move(ezInt32 iMove);

  ezHybridArray<ezPropertyEditorBaseWidget::Selection, 8> m_Items;
  ezPropertyPath m_PropertyPath;

};


