#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <QWidget>

class QLineEdit;
class QPushButton;

/// A widget that contains a line edit for a search text and a button to clear the search text.
///
/// The clear button is only active when the line edit has text.
/// The text can be cleared by pressing ESC while the line edit has focus.
class EZ_GUIFOUNDATION_DLL ezQtSearchWidget : public QWidget
{
  Q_OBJECT
public:
  explicit ezQtSearchWidget(QWidget* pParent);

  /// Passed through to the QLineEdit
  void setText(const QString& sText);

  /// Passed through from the QLineEdit
  QString text() const;

  /// Passed through to the QLineEdit
  void setPlaceholderText(const QString& sText);

  /// Passed through to the QLineEdit
  void selectAll();

Q_SIGNALS:
  /// Passed through from the QLineEdit
  void textChanged(const QString& sText);

  /// The user pressed the enter key
  void enterPressed();

  /// This signal is sent when certain keys are pressed that could be used by external code to implement additional features.
  /// Currently exposed:
  ///   Qt::Key_Up, Qt::Key_Down
  ///   Qt::Key_Tab, Qt::Key_Backtab (that's SHIFT+Tab)
  ///   Qt::Key_F1, Qt::Key_F2, Qt::Key_F3
  void specialKeyPressed(Qt::Key key);

  void visibleEvent();

private Q_SLOTS:
  void onLineEditTextChanged(const QString& text);
  void onClearButtonClicked(bool checked = false);

protected:
  virtual bool eventFilter(QObject* obj, QEvent* e) override;
  virtual void showEvent(QShowEvent* e) override;

  QLineEdit* m_pLineEdit;
  QPushButton* m_pClearButton;
};
