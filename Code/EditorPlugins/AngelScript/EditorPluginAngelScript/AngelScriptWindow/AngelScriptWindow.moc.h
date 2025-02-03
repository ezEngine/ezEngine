#pragma once

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <QTimer>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezAngelScriptAssetDocument;
struct ezAssetCuratorEvent;
class ezEditorEngineDocumentMsg;
class QTextEdit;

class ezQtAngelScriptAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtAngelScriptAssetDocumentWindow(ezAngelScriptAssetDocument* pDocument);
  ~ezQtAngelScriptAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "AngelScript"; }

private Q_SLOTS:
  void onTextEditTextChanged();
  void onEditTimer();

private:
  void AssetEventHandler(const ezAssetCuratorEvent& e);
  void UpdateFileContentDisplay();
  virtual void ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg) override;
  void RetrieveScriptInfos();
  void DocumentObjectEventHandler(const ezDocumentObjectPropertyEvent& e);
  void StoreInlineDocState();

  struct ExposedParam
  {
    ezString m_sName;
    ezVariant m_DefaultValue;
    bool m_bExpose = false;
  };

  ezDynamicArray<ExposedParam> m_ExposedParams;

  ezAngelScriptAssetDocument* m_pAssetDoc = nullptr;

  bool m_bIgnoreCodeChange = false;
  QTextEdit* m_pSourceLabel = nullptr;
  QSyntaxHighlighter* m_pHighlighter = nullptr;
  QTimer m_EditTimer;
  ezTime m_LastEdit;
  ezTime m_LastSave;
};


//////////////////////////////////////////////////////////////////////////

#include <QSyntaxHighlighter>

struct ASEdit
{
  enum ColorComponent
  {
    Comment,
    Number,
    String,
    Operator,
    KeywordBlue,
    KeywordPink,
    KeywordGreen,
    BuiltIn,

    Count,
  };
};

class ASBlockData : public QTextBlockUserData
{
public:
  QList<int> bracketPositions;
};

class ASHighlighter : public QSyntaxHighlighter
{
public:
  ASHighlighter(QTextDocument* pParent = 0);

protected:
  void highlightBlock(const QString& text) override;

private:
  QColor m_Colors[ASEdit::Count];
};
