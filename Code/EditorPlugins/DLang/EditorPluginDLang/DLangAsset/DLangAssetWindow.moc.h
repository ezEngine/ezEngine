#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginDLang/DLangAsset/DLangAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class QTextEdit;
class QSyntaxHighlighter;

class ezQtDLangAssetDocumentWindow : public ezQtDocumentWindow
{
  Q_OBJECT

public:
  ezQtDLangAssetDocumentWindow(ezAssetDocument* pDocument);
  ~ezQtDLangAssetDocumentWindow();

  void UpdateFileContentDisplay();

  virtual const char* GetWindowLayoutGroupName() const override { return "DLangAsset"; }

private:
  void DocumentEventHandler(const ezDLangAssetDocumentEvent& e);

  ezDLangAssetDocument* m_pAssetDoc = nullptr;
  QTextEdit* m_pSourceLabel = nullptr;
  QSyntaxHighlighter* m_pHighlighter = nullptr;
};

//////////////////////////////////////////////////////////////////////////

#include <QSyntaxHighlighter>

struct DLangEdit
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
  };
};

class DLangBlockData : public QTextBlockUserData
{
public:
  QList<int> bracketPositions;
};

class DLangHighlighter : public QSyntaxHighlighter
{
public:
  DLangHighlighter(QTextDocument* parent = 0);

protected:
  void highlightBlock(const QString& text) override;

private:
  QSet<QString> m_keywordsBlue;
  QSet<QString> m_keywordsPink;
  QSet<QString> m_keywordsGreen;
  QSet<QString> m_builtIn;
  QHash<DLangEdit::ColorComponent, QColor> m_colors;
};

