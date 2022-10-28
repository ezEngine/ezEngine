#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <GuiFoundation/ui_CurveEditDlg.h>
#include <QDialog>

class ezCurveGroupData;
class ezObjectAccessorBase;
class ezDocumentObject;

class EZ_GUIFOUNDATION_DLL ezQtCurveEditDlg : public QDialog, Ui_CurveEditDlg
{
  Q_OBJECT
public:
  ezQtCurveEditDlg(ezObjectAccessorBase* pObjectAccessor, const ezDocumentObject* pCurveObject, QWidget* parent);
  ~ezQtCurveEditDlg();

  static QByteArray GetLastDialogGeometry() { return s_LastDialogGeometry; }

  void SetCurveColor(const ezColor& color);
  void SetCurveExtents(double fLower, bool bLowerFixed, double fUpper, bool bUpperFixed);
  void SetCurveRanges(double fLower, double fUpper);

  virtual void reject() override;
  virtual void accept() override;

  void cancel();

Q_SIGNALS:

private Q_SLOTS:
  void OnCpMovedEvent(ezUInt32 curveIdx, ezUInt32 cpIdx, ezInt64 iTickX, double newPosY);
  void OnCpDeletedEvent(ezUInt32 curveIdx, ezUInt32 cpIdx);
  void OnTangentMovedEvent(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent);
  void OnInsertCpEvent(ezUInt32 uiCurveIdx, ezInt64 tickX, double value);
  void OnTangentLinkEvent(ezUInt32 curveIdx, ezUInt32 cpIdx, bool bLink);
  void OnCpTangentModeEvent(ezUInt32 curveIdx, ezUInt32 cpIdx, bool rightTangent, int mode); // ezCurveTangentMode

  void OnBeginCpChangesEvent(QString name);
  void OnEndCpChangesEvent();

  void OnBeginOperationEvent(QString name);
  void OnEndOperationEvent(bool commit);

  void on_actionUndo_triggered();
  void on_actionRedo_triggered();
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();

private:
  static QByteArray s_LastDialogGeometry;

  void RetrieveCurveState();
  void UpdatePreview();

  double m_fLowerRange = -ezMath::HighValue<double>();
  double m_fUpperRange = ezMath::HighValue<double>();
  double m_fLowerExtents = 0.0;
  double m_fUpperExtents = 1.0;
  bool m_bLowerFixed = false;
  bool m_bUpperFixed = false;
  bool m_bCurveLengthIsFixed = false;
  ezCurveGroupData m_Curves;
  ezUInt32 m_uiActionsUndoBaseline = 0;

  QShortcut* m_pShortcutUndo = nullptr;
  QShortcut* m_pShortcutRedo = nullptr;

  ezObjectAccessorBase* m_pObjectAccessor = nullptr;
  const ezDocumentObject* m_pCurveObject = nullptr;

protected:
  virtual void closeEvent(QCloseEvent* e) override;
  virtual void showEvent(QShowEvent* e) override;
};
