#pragma once

#include <EditorPluginFmod/SoundBankAsset/SoundBankAsset.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class QLabel;
class QScrollArea;
class QtImageWidget;

class ezSoundBankAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezSoundBankAssetDocumentWindow(ezDocument* pDocument);
  ~ezSoundBankAssetDocumentWindow();

  virtual const char* GetGroupName() const { return "SoundBankAsset"; }
  virtual const char* GetWindowLayoutGroupName() const override { return "SoundBankAsset"; }

private Q_SLOTS:


private:
  void UpdatePreview();
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

  ezSoundBankAssetDocument* m_pAssetDoc;
  QLabel* m_pLabelInfo;
};
