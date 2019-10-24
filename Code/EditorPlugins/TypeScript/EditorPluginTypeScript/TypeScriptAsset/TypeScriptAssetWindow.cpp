#include <EditorPluginTypeScriptPCH.h>

#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetObjects.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetWindow.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QLabel>
#include <QLayout>
#include <QTextEdit>

ezQtTypeScriptAssetDocumentWindow::ezQtTypeScriptAssetDocumentWindow(ezAssetDocument* pDocument)
  : ezQtDocumentWindow(pDocument)
{
  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "TypeScriptAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "TypeScriptAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("TypeScriptAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Property Grid
  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this);
    pPropertyPanel->setObjectName("TypeScriptAssetDockWidget");
    pPropertyPanel->setWindowTitle("TypeScript Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  m_pAssetDoc = static_cast<ezTypeScriptAssetDocument*>(pDocument);

  // central widget
  {
    QFont font;
    font.setFamily("Courier");
    font.setFixedPitch(true);
    font.setPointSize(10);

    m_pSourceLabel = new QTextEdit(this);
    m_pSourceLabel->setFont(font);
    m_pSourceLabel->setReadOnly(true);

    m_pHighlighter = new JSHighlighter(m_pSourceLabel->document());

    setCentralWidget(m_pSourceLabel);

    UpdateFileContentDisplay();
  }

  FinishWindowCreation();

  m_pAssetDoc->GetEvent().AddEventHandler(ezMakeDelegate(&ezQtTypeScriptAssetDocumentWindow::TsDocumentEventHandler, this));
}

ezQtTypeScriptAssetDocumentWindow::~ezQtTypeScriptAssetDocumentWindow()
{
  m_pAssetDoc->GetEvent().RemoveEventHandler(ezMakeDelegate(&ezQtTypeScriptAssetDocumentWindow::TsDocumentEventHandler, this));

  delete m_pHighlighter;
}

void ezQtTypeScriptAssetDocumentWindow::UpdateFileContentDisplay()
{
  const auto& sFile = m_pAssetDoc->GetProperties()->m_sScriptFile;

  if (sFile.IsEmpty())
  {
    m_pSourceLabel->setText("No script file specified.");
    return;
  }

  ezFileReader file;
  if (file.Open(sFile).Failed())
  {
    m_pSourceLabel->setText(QString("Script file '%1' does not exist. Click 'Edit Script' to create it.").arg(sFile.GetData()));
    return;
  }

  ezStringBuilder content;
  content.ReadAll(file);

  m_pSourceLabel->setText(content.GetData());
}

void ezQtTypeScriptAssetDocumentWindow::TsDocumentEventHandler(const ezTypeScriptAssetDocumentEvent& e)
{
  switch (e.m_Type)
  {
    case ezTypeScriptAssetDocumentEvent::Type::ScriptCreated:
    case ezTypeScriptAssetDocumentEvent::Type::ScriptOpened:
      UpdateFileContentDisplay();
      break;

    default:
      break;
  }
}

//////////////////////////////////////////////////////////////////////////

Highlighter::Highlighter(QTextDocument* parent)
  : QSyntaxHighlighter(parent)
{
  HighlightingRule rule;

  keywordFormat.setForeground(Qt::darkBlue);
  keywordFormat.setFontWeight(QFont::Bold);
  const QString keywordPatterns[] = {
    QStringLiteral("\\bchar\\b"), QStringLiteral("\\bclass\\b"), QStringLiteral("\\bconst\\b"),
    QStringLiteral("\\bdouble\\b"), QStringLiteral("\\benum\\b"), QStringLiteral("\\bexplicit\\b"),
    QStringLiteral("\\bfriend\\b"), QStringLiteral("\\binline\\b"), QStringLiteral("\\bint\\b"),
    QStringLiteral("\\blong\\b"), QStringLiteral("\\bnamespace\\b"), QStringLiteral("\\boperator\\b"),
    QStringLiteral("\\bprivate\\b"), QStringLiteral("\\bprotected\\b"), QStringLiteral("\\bpublic\\b"),
    QStringLiteral("\\bshort\\b"), QStringLiteral("\\bsignals\\b"), QStringLiteral("\\bsigned\\b"),
    QStringLiteral("\\bslots\\b"), QStringLiteral("\\bstatic\\b"), QStringLiteral("\\bstruct\\b"),
    QStringLiteral("\\btemplate\\b"), QStringLiteral("\\btypedef\\b"), QStringLiteral("\\btypename\\b"),
    QStringLiteral("\\bunion\\b"), QStringLiteral("\\bunsigned\\b"), QStringLiteral("\\bvirtual\\b"),
    QStringLiteral("\\bvoid\\b"), QStringLiteral("\\bvolatile\\b"), QStringLiteral("\\bbool\\b")};
  for (const QString& pattern : keywordPatterns)
  {
    rule.pattern = QRegularExpression(pattern);
    rule.format = keywordFormat;
    highlightingRules.append(rule);
  }

  classFormat.setFontWeight(QFont::Bold);
  classFormat.setForeground(Qt::darkMagenta);
  rule.pattern = QRegularExpression(QStringLiteral("\\bQ[A-Za-z]+\\b"));
  rule.format = classFormat;
  highlightingRules.append(rule);

  singleLineCommentFormat.setForeground(Qt::red);
  rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
  rule.format = singleLineCommentFormat;
  highlightingRules.append(rule);

  multiLineCommentFormat.setForeground(Qt::red);

  quotationFormat.setForeground(Qt::darkGreen);
  rule.pattern = QRegularExpression(QStringLiteral("\".*\""));
  rule.format = quotationFormat;
  highlightingRules.append(rule);

  functionFormat.setFontItalic(true);
  functionFormat.setForeground(Qt::blue);
  rule.pattern = QRegularExpression(QStringLiteral("\\b[A-Za-z0-9_]+(?=\\()"));
  rule.format = functionFormat;
  highlightingRules.append(rule);

  commentStartExpression = QRegularExpression(QStringLiteral("/\\*"));
  commentEndExpression = QRegularExpression(QStringLiteral("\\*/"));
}

void Highlighter::highlightBlock(const QString& text)
{
  for (const HighlightingRule& rule : qAsConst(highlightingRules))
  {
    QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
    while (matchIterator.hasNext())
    {
      QRegularExpressionMatch match = matchIterator.next();
      setFormat(match.capturedStart(), match.capturedLength(), rule.format);
    }
  }
  setCurrentBlockState(0);

  int startIndex = 0;
  if (previousBlockState() != 1)
    startIndex = text.indexOf(commentStartExpression);

  while (startIndex >= 0)
  {
    QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
    int endIndex = match.capturedStart();
    int commentLength = 0;
    if (endIndex == -1)
    {
      setCurrentBlockState(1);
      commentLength = text.length() - startIndex;
    }
    else
    {
      commentLength = endIndex - startIndex + match.capturedLength();
    }
    setFormat(startIndex, commentLength, multiLineCommentFormat);
    startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
  }
}

//////////////////////////////////////////////////////////////////////////

JSHighlighter::JSHighlighter(QTextDocument* parent)
  : QSyntaxHighlighter(parent)
  , m_markCaseSensitivity(Qt::CaseInsensitive)
{
  // default color scheme
  m_colors[JSEdit::Normal] = QColor("#000000");
  m_colors[JSEdit::Comment] = QColor("#808080");
  m_colors[JSEdit::Number] = QColor("#008000");
  m_colors[JSEdit::String] = QColor("#800000");
  m_colors[JSEdit::Operator] = QColor("#808000");
  m_colors[JSEdit::Identifier] = QColor("#000020");
  m_colors[JSEdit::Keyword] = QColor("#000080");
  m_colors[JSEdit::BuiltIn] = QColor("#008080");
  m_colors[JSEdit::Marker] = QColor("#ffff00");

  // https://developer.mozilla.org/en/JavaScript/Reference/Reserved_Words
  m_keywords << "break";
  m_keywords << "case";
  m_keywords << "catch";
  m_keywords << "continue";
  m_keywords << "default";
  m_keywords << "delete";
  m_keywords << "do";
  m_keywords << "else";
  m_keywords << "finally";
  m_keywords << "for";
  m_keywords << "function";
  m_keywords << "if";
  m_keywords << "in";
  m_keywords << "instanceof";
  m_keywords << "new";
  m_keywords << "return";
  m_keywords << "switch";
  m_keywords << "this";
  m_keywords << "throw";
  m_keywords << "try";
  m_keywords << "typeof";
  m_keywords << "var";
  m_keywords << "void";
  m_keywords << "while";
  m_keywords << "with";

  m_keywords << "true";
  m_keywords << "false";
  m_keywords << "null";

  // built-in and other popular objects + properties
  m_knownIds << "Object";
  m_knownIds << "prototype";
  m_knownIds << "create";
  m_knownIds << "defineProperty";
  m_knownIds << "defineProperties";
  m_knownIds << "getOwnPropertyDescriptor";
  m_knownIds << "keys";
  m_knownIds << "getOwnPropertyNames";
  m_knownIds << "constructor";
  m_knownIds << "__parent__";
  m_knownIds << "__proto__";
  m_knownIds << "__defineGetter__";
  m_knownIds << "__defineSetter__";
  m_knownIds << "eval";
  m_knownIds << "hasOwnProperty";
  m_knownIds << "isPrototypeOf";
  m_knownIds << "__lookupGetter__";
  m_knownIds << "__lookupSetter__";
  m_knownIds << "__noSuchMethod__";
  m_knownIds << "propertyIsEnumerable";
  m_knownIds << "toSource";
  m_knownIds << "toLocaleString";
  m_knownIds << "toString";
  m_knownIds << "unwatch";
  m_knownIds << "valueOf";
  m_knownIds << "watch";

  m_knownIds << "Function";
  m_knownIds << "arguments";
  m_knownIds << "arity";
  m_knownIds << "caller";
  m_knownIds << "constructor";
  m_knownIds << "length";
  m_knownIds << "name";
  m_knownIds << "apply";
  m_knownIds << "bind";
  m_knownIds << "call";

  m_knownIds << "String";
  m_knownIds << "fromCharCode";
  m_knownIds << "length";
  m_knownIds << "charAt";
  m_knownIds << "charCodeAt";
  m_knownIds << "concat";
  m_knownIds << "indexOf";
  m_knownIds << "lastIndexOf";
  m_knownIds << "localCompare";
  m_knownIds << "match";
  m_knownIds << "quote";
  m_knownIds << "replace";
  m_knownIds << "search";
  m_knownIds << "slice";
  m_knownIds << "split";
  m_knownIds << "substr";
  m_knownIds << "substring";
  m_knownIds << "toLocaleLowerCase";
  m_knownIds << "toLocaleUpperCase";
  m_knownIds << "toLowerCase";
  m_knownIds << "toUpperCase";
  m_knownIds << "trim";
  m_knownIds << "trimLeft";
  m_knownIds << "trimRight";

  m_knownIds << "Array";
  m_knownIds << "isArray";
  m_knownIds << "index";
  m_knownIds << "input";
  m_knownIds << "pop";
  m_knownIds << "push";
  m_knownIds << "reverse";
  m_knownIds << "shift";
  m_knownIds << "sort";
  m_knownIds << "splice";
  m_knownIds << "unshift";
  m_knownIds << "concat";
  m_knownIds << "join";
  m_knownIds << "filter";
  m_knownIds << "forEach";
  m_knownIds << "every";
  m_knownIds << "map";
  m_knownIds << "some";
  m_knownIds << "reduce";
  m_knownIds << "reduceRight";

  m_knownIds << "RegExp";
  m_knownIds << "global";
  m_knownIds << "ignoreCase";
  m_knownIds << "lastIndex";
  m_knownIds << "multiline";
  m_knownIds << "source";
  m_knownIds << "exec";
  m_knownIds << "test";

  m_knownIds << "JSON";
  m_knownIds << "parse";
  m_knownIds << "stringify";

  m_knownIds << "decodeURI";
  m_knownIds << "decodeURIComponent";
  m_knownIds << "encodeURI";
  m_knownIds << "encodeURIComponent";
  m_knownIds << "eval";
  m_knownIds << "isFinite";
  m_knownIds << "isNaN";
  m_knownIds << "parseFloat";
  m_knownIds << "parseInt";
  m_knownIds << "Infinity";
  m_knownIds << "NaN";
  m_knownIds << "undefined";

  m_knownIds << "Math";
  m_knownIds << "E";
  m_knownIds << "LN2";
  m_knownIds << "LN10";
  m_knownIds << "LOG2E";
  m_knownIds << "LOG10E";
  m_knownIds << "PI";
  m_knownIds << "SQRT1_2";
  m_knownIds << "SQRT2";
  m_knownIds << "abs";
  m_knownIds << "acos";
  m_knownIds << "asin";
  m_knownIds << "atan";
  m_knownIds << "atan2";
  m_knownIds << "ceil";
  m_knownIds << "cos";
  m_knownIds << "exp";
  m_knownIds << "floor";
  m_knownIds << "log";
  m_knownIds << "max";
  m_knownIds << "min";
  m_knownIds << "pow";
  m_knownIds << "random";
  m_knownIds << "round";
  m_knownIds << "sin";
  m_knownIds << "sqrt";
  m_knownIds << "tan";

  m_knownIds << "document";
  m_knownIds << "window";
  m_knownIds << "navigator";
  m_knownIds << "userAgent";
}

void JSHighlighter::setColor(JSEdit::ColorComponent component, const QColor& color)
{
  m_colors[component] = color;
  rehighlight();
}

void JSHighlighter::highlightBlock(const QString& text)
{
  // parsing state
  enum
  {
    Start = 0,
    Number = 1,
    Identifier = 2,
    String = 3,
    Comment = 4,
    Regex = 5
  };

  QList<int> bracketPositions;

  int blockState = previousBlockState();
  int bracketLevel = blockState >> 4;
  int state = blockState & 15;
  if (blockState < 0)
  {
    bracketLevel = 0;
    state = Start;
  }

  int start = 0;
  int i = 0;
  while (i <= text.length())
  {
    QChar ch = (i < text.length()) ? text.at(i) : QChar();
    QChar next = (i < text.length() - 1) ? text.at(i + 1) : QChar();

    switch (state)
    {

      case Start:
        start = i;
        if (ch.isSpace())
        {
          ++i;
        }
        else if (ch.isDigit())
        {
          ++i;
          state = Number;
        }
        else if (ch.isLetter() || ch == '_')
        {
          ++i;
          state = Identifier;
        }
        else if (ch == '\'' || ch == '\"')
        {
          ++i;
          state = String;
        }
        else if (ch == '/' && next == '*')
        {
          ++i;
          ++i;
          state = Comment;
        }
        else if (ch == '/' && next == '/')
        {
          i = text.length();
          setFormat(start, text.length(), m_colors[JSEdit::Comment]);
        }
        else if (ch == '/' && next != '*')
        {
          ++i;
          state = Regex;
        }
        else
        {
          if (!QString("(){}[]").contains(ch))
            setFormat(start, 1, m_colors[JSEdit::Operator]);
          if (ch == '{' || ch == '}')
          {
            bracketPositions += i;
            if (ch == '{')
              bracketLevel++;
            else
              bracketLevel--;
          }
          ++i;
          state = Start;
        }
        break;

      case Number:
        if (ch.isSpace() || !ch.isDigit())
        {
          setFormat(start, i - start, m_colors[JSEdit::Number]);
          state = Start;
        }
        else
        {
          ++i;
        }
        break;

      case Identifier:
        if (ch.isSpace() || !(ch.isDigit() || ch.isLetter() || ch == '_'))
        {
          QString token = text.mid(start, i - start).trimmed();
          if (m_keywords.contains(token))
            setFormat(start, i - start, m_colors[JSEdit::Keyword]);
          else if (m_knownIds.contains(token))
            setFormat(start, i - start, m_colors[JSEdit::BuiltIn]);
          state = Start;
        }
        else
        {
          ++i;
        }
        break;

      case String:
        if (ch == text.at(start))
        {
          QChar prev = (i > 0) ? text.at(i - 1) : QChar();
          if (prev != '\\')
          {
            ++i;
            setFormat(start, i - start, m_colors[JSEdit::String]);
            state = Start;
          }
          else
          {
            ++i;
          }
        }
        else
        {
          ++i;
        }
        break;

      case Comment:
        if (ch == '*' && next == '/')
        {
          ++i;
          ++i;
          setFormat(start, i - start, m_colors[JSEdit::Comment]);
          state = Start;
        }
        else
        {
          ++i;
        }
        break;

      case Regex:
        if (ch == '/')
        {
          QChar prev = (i > 0) ? text.at(i - 1) : QChar();
          if (prev != '\\')
          {
            ++i;
            setFormat(start, i - start, m_colors[JSEdit::String]);
            state = Start;
          }
          else
          {
            ++i;
          }
        }
        else
        {
          ++i;
        }
        break;

      default:
        state = Start;
        break;
    }
  }

  if (state == Comment)
    setFormat(start, text.length(), m_colors[JSEdit::Comment]);
  else
    state = Start;

  if (!m_markString.isEmpty())
  {
    int pos = 0;
    int len = m_markString.length();
    QTextCharFormat markerFormat;
    markerFormat.setBackground(m_colors[JSEdit::Marker]);
    markerFormat.setForeground(m_colors[JSEdit::Normal]);
    for (;;)
    {
      pos = text.indexOf(m_markString, pos, m_markCaseSensitivity);
      if (pos < 0)
        break;
      setFormat(pos, len, markerFormat);
      ++pos;
    }
  }

  if (!bracketPositions.isEmpty())
  {
    JSBlockData* blockData = reinterpret_cast<JSBlockData*>(currentBlock().userData());
    if (!blockData)
    {
      blockData = new JSBlockData;
      currentBlock().setUserData(blockData);
    }
    blockData->bracketPositions = bracketPositions;
  }

  blockState = (state & 15) | (bracketLevel << 4);
  setCurrentBlockState(blockState);
}

void JSHighlighter::mark(const QString& str, Qt::CaseSensitivity caseSensitivity)
{
  m_markString = str;
  m_markCaseSensitivity = caseSensitivity;
  rehighlight();
}

QStringList JSHighlighter::keywords() const
{
  return m_keywords.toList();
}

void JSHighlighter::setKeywords(const QStringList& keywords)
{
  m_keywords = QSet<QString>::fromList(keywords);
  rehighlight();
}
