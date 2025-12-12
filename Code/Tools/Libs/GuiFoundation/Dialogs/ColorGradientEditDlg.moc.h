#pragma once

#include <Foundation/Tracks/ColorGradient.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_ColorGradientEditDlg.h>
#include <QDialog>

class ezObjectAccessorBase;
class ezDocumentObject;

class EZ_GUIFOUNDATION_DLL ezQtColorGradientEditDlg : public QDialog, public Ui_ColorGradientEditDlg
{
  Q_OBJECT

public:
  ezQtColorGradientEditDlg(ezObjectAccessorBase* pObjectAccessor, const ezDocumentObject* pGradientObject, QWidget* pParent, ezStringView sTitle = {});
  ~ezQtColorGradientEditDlg();

  static QByteArray GetLastDialogGeometry() { return s_LastDialogGeometry; }

  virtual void reject() override;
  virtual void accept() override;

  void cancel();

private Q_SLOTS:
  // Color CP operations
  void OnColorCpAdded(double fPosX, const ezColorGammaUB& color);
  void OnColorCpMoved(ezInt32 iIndex, double fNewPosX);
  void OnColorCpDeleted(ezInt32 iIndex);
  void OnColorCpChanged(ezInt32 iIndex, const ezColorGammaUB& color);

  // Alpha CP operations
  void OnAlphaCpAdded(double fPosX, ezUInt8 uiAlpha);
  void OnAlphaCpMoved(ezInt32 iIndex, double fNewPosX);
  void OnAlphaCpDeleted(ezInt32 iIndex);
  void OnAlphaCpChanged(ezInt32 iIndex, ezUInt8 uiAlpha);

  // Intensity CP operations
  void OnIntensityCpAdded(double fPosX, float fIntensity);
  void OnIntensityCpMoved(ezInt32 iIndex, double fNewPosX);
  void OnIntensityCpDeleted(ezInt32 iIndex);
  void OnIntensityCpChanged(ezInt32 iIndex, float fIntensity);

  // Operation boundaries
  void OnBeginOperation();
  void OnEndOperation(bool bCommit);

  // UI actions
  void OnNormalizeRange();
  void on_actionUndo_triggered();
  void on_actionRedo_triggered();
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();
  void on_ButtonUndo_clicked();
  void on_ButtonRedo_clicked();

private:
  static QByteArray s_LastDialogGeometry;

  void RetrieveGradientState();
  void UpdatePreview();
  void UpdateUndoRedoState();

  ezColorGradient m_Gradient;
  ezUInt32 m_uiActionsUndoBaseline = 0;

  QShortcut* m_pShortcutUndo = nullptr;
  QShortcut* m_pShortcutRedo = nullptr;

  ezObjectAccessorBase* m_pObjectAccessor = nullptr;
  const ezDocumentObject* m_pGradientObject = nullptr;

protected:
  virtual void closeEvent(QCloseEvent* e) override;
  virtual void showEvent(QShowEvent* e) override;
};
