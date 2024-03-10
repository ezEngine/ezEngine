#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class QTextEdit;
class QSyntaxHighlighter;

class ezQtTypeScriptAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtTypeScriptAssetDocumentWindow(ezAssetDocument* pDocument);
  ~ezQtTypeScriptAssetDocumentWindow();

  void UpdateFileContentDisplay();

  virtual const char* GetWindowLayoutGroupName() const override { return "TypeScriptAsset"; }

private:
  void TsDocumentEventHandler(const ezTypeScriptAssetDocumentEvent& e);

  ezTypeScriptAssetDocument* m_pAssetDoc = nullptr;
  QTextEdit* m_pSourceLabel = nullptr;
  QSyntaxHighlighter* m_pHighlighter = nullptr;
};

//////////////////////////////////////////////////////////////////////////

#include <QSyntaxHighlighter>

struct JSEdit
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

class JSBlockData : public QTextBlockUserData
{
public:
  QList<int> bracketPositions;
};

class JSHighlighter : public QSyntaxHighlighter
{
public:
  JSHighlighter(QTextDocument* pParent = 0);

protected:
  void highlightBlock(const QString& text) override;

private:
  QSet<QString> m_KeywordsBlue;
  QSet<QString> m_KeywordsPink;
  QSet<QString> m_KeywordsGreen;
  QSet<QString> m_BuiltIn;
  QColor m_Colors[JSEdit::Count];
};
