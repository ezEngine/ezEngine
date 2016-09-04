#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class QCurve1DEditorWidget;

class ezCurve1DAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezCurve1DAssetDocumentWindow(ezDocument* pDocument);
  ~ezCurve1DAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const { return "Curve1DAsset"; }

private slots:
  void onCurveCpMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY);
  void onCurveCpDeleted(ezUInt32 curveIdx, ezUInt32 cpIdx);
  void onCurveTangentMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent);

  void onCurveBeginOperation();
  void onCurveEndOperation(bool commit);
  void onCurveBeginCpChanges();
  void onCurveEndCpChanges();

private:
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);

  QCurve1DEditorWidget* m_pCurveEditor;
};