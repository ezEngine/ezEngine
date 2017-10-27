#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <GuiFoundation/Widgets/CurveEditData.h>

class ezQtPropertyAnimModel;
class QTreeView;
class QItemSelection;
class QItemSelectionModel;
class ezQtCurve1DEditorWidget;

class ezQtPropertyAnimAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtPropertyAnimAssetDocumentWindow(ezDocument* pDocument);
  ~ezQtPropertyAnimAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const { return "PropertyAnimAsset"; }

private slots:
  void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

  void onInsertCpAt(ezUInt32 uiCurveIdx, ezInt64 tickX, double newPosY);
  void onCurveCpMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, ezInt64 iTickX, double newPosY);
  void onCurveCpDeleted(ezUInt32 curveIdx, ezUInt32 cpIdx);
  void onCurveTangentMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent);
  void onLinkCurveTangents(ezUInt32 curveIdx, ezUInt32 cpIdx, bool bLink);
  void onCurveTangentModeChanged(ezUInt32 curveIdx, ezUInt32 cpIdx, bool rightTangent, int mode);

  void onCurveBeginOperation(QString name);
  void onCurveEndOperation(bool commit);
  void onCurveBeginCpChanges(QString name);
  void onCurveEndCpChanges();

private:
  void UpdateCurveEditor();

  ezCurveGroupData m_CurvesToDisplay;
  ezDynamicArray<ezInt32> m_MapSelectionToTrack;
  QTreeView* m_pPropertyTreeView = nullptr;
  ezQtPropertyAnimModel* m_pPropertiesModel;
  QItemSelectionModel* m_pSelectionModel = nullptr;
  ezQtCurve1DEditorWidget* m_pCurveEditor = nullptr;
};
