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

  virtual const char* GetWindowLayoutGroupName() const { return "TypeScriptAsset"; }

private:
  void TsDocumentEventHandler(const ezTypeScriptAssetDocumentEvent& e);

  ezTypeScriptAssetDocument* m_pAssetDoc = nullptr;
  QTextEdit* m_pSourceLabel = nullptr;
  QSyntaxHighlighter* m_pHighlighter = nullptr;
};

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class QTextDocument;

class Highlighter : public QSyntaxHighlighter
{
  Q_OBJECT

public:
  Highlighter(QTextDocument* parent = 0);

protected:
  void highlightBlock(const QString& text) override;

private:
  struct HighlightingRule
  {
    QRegularExpression pattern;
    QTextCharFormat format;
  };
  QVector<HighlightingRule> highlightingRules;

  QRegularExpression commentStartExpression;
  QRegularExpression commentEndExpression;

  QTextCharFormat keywordFormat;
  QTextCharFormat classFormat;
  QTextCharFormat singleLineCommentFormat;
  QTextCharFormat multiLineCommentFormat;
  QTextCharFormat quotationFormat;
  QTextCharFormat functionFormat;
};

//////////////////////////////////////////////////////////////////////////

struct JSEdit
{
  enum ColorComponent
  {
    Background,
    Normal,
    Comment,
    Number,
    String,
    Operator,
    Identifier,
    Keyword,
    BuiltIn,
    Sidebar,
    LineNumber,
    Cursor,
    Marker,
    BracketMatch,
    BracketError,
    FoldIndicator
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
  JSHighlighter(QTextDocument* parent = 0);
  void setColor(JSEdit::ColorComponent component, const QColor& color);
  void mark(const QString& str, Qt::CaseSensitivity caseSensitivity);

  QStringList keywords() const;
  void setKeywords(const QStringList& keywords);

protected:
  void highlightBlock(const QString& text);

private:
  QSet<QString> m_keywords;
  QSet<QString> m_knownIds;
  QHash<JSEdit::ColorComponent, QColor> m_colors;
  QString m_markString;
  Qt::CaseSensitivity m_markCaseSensitivity;
};
