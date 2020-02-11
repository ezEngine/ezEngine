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
    case ezTypeScriptAssetDocumentEvent::Type::ScriptTransformed:
      UpdateFileContentDisplay();
      break;

    default:
      break;
  }
}

//////////////////////////////////////////////////////////////////////////

JSHighlighter::JSHighlighter(QTextDocument* parent)
  : QSyntaxHighlighter(parent)
{
  // default color scheme
  m_colors[JSEdit::Comment] = QColor("#6A8A35");
  m_colors[JSEdit::Number] = QColor("#B5CEA8");
  m_colors[JSEdit::String] = QColor("#CE916A");
  m_colors[JSEdit::Operator] = QColor("#808000");
  m_colors[JSEdit::KeywordBlue] = QColor("#569CCA");
  m_colors[JSEdit::KeywordPink] = QColor("#C586C0");
  m_colors[JSEdit::KeywordGreen] = QColor("#4EC9B0");
  m_colors[JSEdit::BuiltIn] = QColor("#DCDCAA");

  m_keywordsPink << "break";
  m_keywordsPink << "case";
  m_keywordsPink << "catch";
  m_keywordsPink << "continue";
  m_keywordsPink << "default";
  m_keywordsBlue << "delete";
  m_keywordsPink << "do";
  m_keywordsPink << "finally";
  m_keywordsPink << "for";
  m_keywordsBlue << "function";
  m_keywordsBlue << "in";
  m_keywordsBlue << "instanceof";
  m_keywordsBlue << "new";
  m_keywordsPink << "return";
  m_keywordsPink << "switch";
  m_keywordsBlue << "this";
  m_keywordsPink << "throw";
  m_keywordsPink << "try";
  m_keywordsBlue << "typeof";
  m_keywordsBlue << "var";
  m_keywordsBlue << "void";
  m_keywordsPink << "while";
  m_keywordsPink << "with";
  m_keywordsBlue << "true";
  m_keywordsBlue << "false";
  m_keywordsBlue << "null";
  m_keywordsBlue << "static";
  m_keywordsBlue << "let";
  m_keywordsBlue << "class";
  m_keywordsBlue << "extends";
  m_keywordsBlue << "implements";
  m_keywordsBlue << "const";
  m_keywordsBlue << "enum";
  m_keywordsBlue << "super";
  m_keywordsPink << "as";
  m_keywordsBlue << "private";
  m_keywordsBlue << "protected";
  m_keywordsBlue << "public";
  m_keywordsPink << "yield";
  m_keywordsBlue << "symbol";
  m_keywordsBlue << "type";
  m_keywordsBlue << "from";
  m_keywordsBlue << "of";
  m_keywordsGreen << "boolean";
  m_keywordsGreen << "string";
  m_keywordsGreen << "number";
  m_keywordsGreen << "any";
  m_keywordsBlue << "declare";
  m_keywordsBlue << "get";
  m_keywordsBlue << "set";
  m_keywordsGreen << "module";
  m_keywordsBlue << "namespace";
  m_keywordsPink << "async";
  m_keywordsPink << "await";
  m_keywordsPink << "export";
  m_keywordsPink << "import";
  m_keywordsPink << "interface";
  m_keywordsPink << "require";
  m_keywordsPink << "package";
  m_keywordsPink << "if";
  m_keywordsPink << "else";


  // built-in and other popular objects + properties
  m_builtIn << "Object";
  m_builtIn << "prototype";
  m_builtIn << "create";
  m_builtIn << "defineProperty";
  m_builtIn << "defineProperties";
  m_builtIn << "getOwnPropertyDescriptor";
  m_builtIn << "keys";
  m_builtIn << "getOwnPropertyNames";
  m_builtIn << "constructor";
  m_builtIn << "__parent__";
  m_builtIn << "__proto__";
  m_builtIn << "__defineGetter__";
  m_builtIn << "__defineSetter__";
  m_builtIn << "eval";
  m_builtIn << "hasOwnProperty";
  m_builtIn << "isPrototypeOf";
  m_builtIn << "__lookupGetter__";
  m_builtIn << "__lookupSetter__";
  m_builtIn << "__noSuchMethod__";
  m_builtIn << "propertyIsEnumerable";
  m_builtIn << "toSource";
  m_builtIn << "toLocaleString";
  m_builtIn << "toString";
  m_builtIn << "unwatch";
  m_builtIn << "valueOf";
  m_builtIn << "watch";

  m_builtIn << "Function";
  m_builtIn << "arguments";
  m_builtIn << "arity";
  m_builtIn << "caller";
  m_builtIn << "constructor";
  m_builtIn << "length";
  m_builtIn << "name";
  m_builtIn << "apply";
  m_builtIn << "bind";
  m_builtIn << "call";

  m_builtIn << "String";
  m_builtIn << "fromCharCode";
  m_builtIn << "length";
  m_builtIn << "charAt";
  m_builtIn << "charCodeAt";
  m_builtIn << "concat";
  m_builtIn << "indexOf";
  m_builtIn << "lastIndexOf";
  m_builtIn << "localCompare";
  m_builtIn << "match";
  m_builtIn << "quote";
  m_builtIn << "replace";
  m_builtIn << "search";
  m_builtIn << "slice";
  m_builtIn << "split";
  m_builtIn << "substr";
  m_builtIn << "substring";
  m_builtIn << "toLocaleLowerCase";
  m_builtIn << "toLocaleUpperCase";
  m_builtIn << "toLowerCase";
  m_builtIn << "toUpperCase";
  m_builtIn << "trim";
  m_builtIn << "trimLeft";
  m_builtIn << "trimRight";

  m_builtIn << "Array";
  m_builtIn << "isArray";
  m_builtIn << "index";
  m_builtIn << "input";
  m_builtIn << "pop";
  m_builtIn << "push";
  m_builtIn << "reverse";
  m_builtIn << "shift";
  m_builtIn << "sort";
  m_builtIn << "splice";
  m_builtIn << "unshift";
  m_builtIn << "concat";
  m_builtIn << "join";
  m_builtIn << "filter";
  m_builtIn << "forEach";
  m_builtIn << "every";
  m_builtIn << "map";
  m_builtIn << "some";
  m_builtIn << "reduce";
  m_builtIn << "reduceRight";

  m_builtIn << "RegExp";
  m_builtIn << "global";
  m_builtIn << "ignoreCase";
  m_builtIn << "lastIndex";
  m_builtIn << "multiline";
  m_builtIn << "source";
  m_builtIn << "exec";
  m_builtIn << "test";

  m_builtIn << "JSON";
  m_builtIn << "parse";
  m_builtIn << "stringify";

  m_builtIn << "decodeURI";
  m_builtIn << "decodeURIComponent";
  m_builtIn << "encodeURI";
  m_builtIn << "encodeURIComponent";
  m_builtIn << "eval";
  m_builtIn << "isFinite";
  m_builtIn << "isNaN";
  m_builtIn << "parseFloat";
  m_builtIn << "parseInt";
  m_builtIn << "Infinity";
  m_builtIn << "NaN";
  m_builtIn << "undefined";

  m_keywordsGreen << "Math";
  m_keywordsGreen << "E";
  m_builtIn << "LN2";
  m_builtIn << "LN10";
  m_builtIn << "LOG2E";
  m_builtIn << "LOG10E";
  m_keywordsGreen << "PI";
  m_builtIn << "SQRT1_2";
  m_builtIn << "SQRT2";
  m_builtIn << "abs";
  m_builtIn << "acos";
  m_builtIn << "asin";
  m_builtIn << "atan";
  m_builtIn << "atan2";
  m_builtIn << "ceil";
  m_builtIn << "cos";
  m_builtIn << "exp";
  m_builtIn << "floor";
  m_builtIn << "log";
  m_builtIn << "max";
  m_builtIn << "min";
  m_builtIn << "pow";
  m_builtIn << "random";
  m_builtIn << "round";
  m_builtIn << "sin";
  m_builtIn << "sqrt";
  m_builtIn << "tan";

  m_builtIn << "document";
  m_builtIn << "window";
  m_builtIn << "navigator";
  m_builtIn << "userAgent";

  m_keywordsGreen << "ez";
  m_keywordsGreen << "Vec3";
  m_keywordsGreen << "Quat";
  m_keywordsGreen << "Color";
  m_keywordsGreen << "Log";
  m_keywordsGreen << "ezMath";
  m_keywordsGreen << "Angle";
  m_keywordsGreen << "Time";
  m_keywordsGreen << "World";
  m_keywordsGreen << "GameObject";
  m_keywordsGreen << "Component";
  m_keywordsGreen << "Message";
  m_keywordsGreen << "TypescriptComponent";
  m_keywordsGreen << "TickedTypescriptComponent";
  m_keywordsGreen << "Tick";
  m_keywordsGreen << "Initialize";
  m_keywordsGreen << "Deinitialize";
  m_keywordsGreen << "OnActivated";
  m_keywordsGreen << "OnActivated";
  m_keywordsGreen << "OnSimulationStarted";
  m_keywordsGreen << "RegisterMessageHandlers";
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
          if (m_keywordsBlue.contains(token))
            setFormat(start, i - start, m_colors[JSEdit::KeywordBlue]);
          if (m_keywordsPink.contains(token))
            setFormat(start, i - start, m_colors[JSEdit::KeywordPink]);
          if (m_keywordsGreen.contains(token))
            setFormat(start, i - start, m_colors[JSEdit::KeywordGreen]);
          else if (m_builtIn.contains(token))
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
