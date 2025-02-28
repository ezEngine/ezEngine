#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginTypeScript/EditorPluginTypeScriptPCH.h>

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

  m_pAssetDoc = static_cast<ezTypeScriptAssetDocument*>(pDocument);

  // Central Widget
  {
    QFont font;
    font.setFamily("Courier");
    font.setFixedPitch(true);
    font.setPointSize(10);

    m_pSourceLabel = new QTextEdit(this);
    m_pSourceLabel->setFont(font);
    m_pSourceLabel->setReadOnly(true);

    m_pHighlighter = new JSHighlighter(m_pSourceLabel->document());

    ezQtDocumentPanel* pCentral = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pCentral->setObjectName("TypeScriptView");
    pCentral->setWindowTitle("Script");
    pCentral->setWidget(m_pSourceLabel);

    m_pDockManager->setCentralWidget(pCentral);

    UpdateFileContentDisplay();
  }

  // Property Grid
  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("TypeScriptAssetDockWidget");
    pPropertyPanel->setWindowTitle("TypeScript Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new ezQtAssetStatusIndicator((ezAssetDocument*)GetDocument()));
    pWidget->layout()->addWidget(pPropertyGrid);

    pPropertyPanel->setWidget(pWidget, ads::CDockWidget::ForceNoScrollArea);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
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

JSHighlighter::JSHighlighter(QTextDocument* pParent)
  : QSyntaxHighlighter(pParent)
{
  // default color scheme
  m_Colors[JSEdit::Comment] = QColor("#6A8A35");
  m_Colors[JSEdit::Number] = QColor("#B5CEA8");
  m_Colors[JSEdit::String] = QColor("#CE916A");
  m_Colors[JSEdit::Operator] = QColor("#808000");
  m_Colors[JSEdit::KeywordBlue] = QColor("#569CCA");
  m_Colors[JSEdit::KeywordPink] = QColor("#C586C0");
  m_Colors[JSEdit::KeywordGreen] = QColor("#4EC9B0");
  m_Colors[JSEdit::BuiltIn] = QColor("#DCDCAA");

  m_KeywordsPink << "break";
  m_KeywordsPink << "case";
  m_KeywordsPink << "catch";
  m_KeywordsPink << "continue";
  m_KeywordsPink << "default";
  m_KeywordsBlue << "delete";
  m_KeywordsPink << "do";
  m_KeywordsPink << "finally";
  m_KeywordsPink << "for";
  m_KeywordsBlue << "function";
  m_KeywordsBlue << "in";
  m_KeywordsBlue << "instanceof";
  m_KeywordsBlue << "new";
  m_KeywordsPink << "return";
  m_KeywordsPink << "switch";
  m_KeywordsBlue << "this";
  m_KeywordsPink << "throw";
  m_KeywordsPink << "try";
  m_KeywordsBlue << "typeof";
  m_KeywordsBlue << "var";
  m_KeywordsBlue << "void";
  m_KeywordsPink << "while";
  m_KeywordsPink << "with";
  m_KeywordsBlue << "true";
  m_KeywordsBlue << "false";
  m_KeywordsBlue << "null";
  m_KeywordsBlue << "static";
  m_KeywordsBlue << "let";
  m_KeywordsBlue << "class";
  m_KeywordsBlue << "extends";
  m_KeywordsBlue << "implements";
  m_KeywordsBlue << "const";
  m_KeywordsBlue << "enum";
  m_KeywordsBlue << "super";
  m_KeywordsPink << "as";
  m_KeywordsBlue << "private";
  m_KeywordsBlue << "protected";
  m_KeywordsBlue << "public";
  m_KeywordsPink << "yield";
  m_KeywordsBlue << "symbol";
  m_KeywordsBlue << "type";
  m_KeywordsBlue << "from";
  m_KeywordsBlue << "of";
  m_KeywordsGreen << "boolean";
  m_KeywordsGreen << "string";
  m_KeywordsGreen << "number";
  m_KeywordsGreen << "any";
  m_KeywordsBlue << "declare";
  m_KeywordsBlue << "get";
  m_KeywordsBlue << "set";
  m_KeywordsGreen << "module";
  m_KeywordsBlue << "namespace";
  m_KeywordsPink << "async";
  m_KeywordsPink << "await";
  m_KeywordsPink << "export";
  m_KeywordsPink << "import";
  m_KeywordsPink << "interface";
  m_KeywordsPink << "require";
  m_KeywordsPink << "package";
  m_KeywordsPink << "if";
  m_KeywordsPink << "else";


  // built-in and other popular objects + properties
  m_BuiltIn << "Object";
  m_BuiltIn << "prototype";
  m_BuiltIn << "create";
  m_BuiltIn << "defineProperty";
  m_BuiltIn << "defineProperties";
  m_BuiltIn << "getOwnPropertyDescriptor";
  m_BuiltIn << "keys";
  m_BuiltIn << "getOwnPropertyNames";
  m_BuiltIn << "constructor";
  m_BuiltIn << "__parent__";
  m_BuiltIn << "__proto__";
  m_BuiltIn << "__defineGetter__";
  m_BuiltIn << "__defineSetter__";
  m_BuiltIn << "eval";
  m_BuiltIn << "hasOwnProperty";
  m_BuiltIn << "isPrototypeOf";
  m_BuiltIn << "__lookupGetter__";
  m_BuiltIn << "__lookupSetter__";
  m_BuiltIn << "__noSuchMethod__";
  m_BuiltIn << "propertyIsEnumerable";
  m_BuiltIn << "toSource";
  m_BuiltIn << "toLocaleString";
  m_BuiltIn << "toString";
  m_BuiltIn << "unwatch";
  m_BuiltIn << "valueOf";
  m_BuiltIn << "watch";

  m_BuiltIn << "Function";
  m_BuiltIn << "arguments";
  m_BuiltIn << "arity";
  m_BuiltIn << "caller";
  m_BuiltIn << "constructor";
  m_BuiltIn << "length";
  m_BuiltIn << "name";
  m_BuiltIn << "apply";
  m_BuiltIn << "bind";
  m_BuiltIn << "call";

  m_BuiltIn << "String";
  m_BuiltIn << "fromCharCode";
  m_BuiltIn << "length";
  m_BuiltIn << "charAt";
  m_BuiltIn << "charCodeAt";
  m_BuiltIn << "concat";
  m_BuiltIn << "indexOf";
  m_BuiltIn << "lastIndexOf";
  m_BuiltIn << "localCompare";
  m_BuiltIn << "match";
  m_BuiltIn << "quote";
  m_BuiltIn << "replace";
  m_BuiltIn << "search";
  m_BuiltIn << "slice";
  m_BuiltIn << "split";
  m_BuiltIn << "substr";
  m_BuiltIn << "substring";
  m_BuiltIn << "toLocaleLowerCase";
  m_BuiltIn << "toLocaleUpperCase";
  m_BuiltIn << "toLowerCase";
  m_BuiltIn << "toUpperCase";
  m_BuiltIn << "trim";
  m_BuiltIn << "trimLeft";
  m_BuiltIn << "trimRight";

  m_BuiltIn << "Array";
  m_BuiltIn << "isArray";
  m_BuiltIn << "index";
  m_BuiltIn << "input";
  m_BuiltIn << "pop";
  m_BuiltIn << "push";
  m_BuiltIn << "reverse";
  m_BuiltIn << "shift";
  m_BuiltIn << "sort";
  m_BuiltIn << "splice";
  m_BuiltIn << "unshift";
  m_BuiltIn << "concat";
  m_BuiltIn << "join";
  m_BuiltIn << "filter";
  m_BuiltIn << "forEach";
  m_BuiltIn << "every";
  m_BuiltIn << "map";
  m_BuiltIn << "some";
  m_BuiltIn << "reduce";
  m_BuiltIn << "reduceRight";

  m_BuiltIn << "RegExp";
  m_BuiltIn << "global";
  m_BuiltIn << "ignoreCase";
  m_BuiltIn << "lastIndex";
  m_BuiltIn << "multiline";
  m_BuiltIn << "source";
  m_BuiltIn << "exec";
  m_BuiltIn << "test";

  m_BuiltIn << "JSON";
  m_BuiltIn << "parse";
  m_BuiltIn << "stringify";

  m_BuiltIn << "decodeURI";
  m_BuiltIn << "decodeURIComponent";
  m_BuiltIn << "encodeURI";
  m_BuiltIn << "encodeURIComponent";
  m_BuiltIn << "eval";
  m_BuiltIn << "isFinite";
  m_BuiltIn << "isNaN";
  m_BuiltIn << "parseFloat";
  m_BuiltIn << "parseInt";
  m_BuiltIn << "Infinity";
  m_BuiltIn << "NaN";
  m_BuiltIn << "undefined";

  m_KeywordsGreen << "Math";
  m_KeywordsGreen << "E";
  m_BuiltIn << "LN2";
  m_BuiltIn << "LN10";
  m_BuiltIn << "LOG2E";
  m_BuiltIn << "LOG10E";
  m_KeywordsGreen << "PI";
  m_BuiltIn << "SQRT1_2";
  m_BuiltIn << "SQRT2";
  m_BuiltIn << "abs";
  m_BuiltIn << "acos";
  m_BuiltIn << "asin";
  m_BuiltIn << "atan";
  m_BuiltIn << "atan2";
  m_BuiltIn << "ceil";
  m_BuiltIn << "cos";
  m_BuiltIn << "exp";
  m_BuiltIn << "floor";
  m_BuiltIn << "log";
  m_BuiltIn << "max";
  m_BuiltIn << "min";
  m_BuiltIn << "pow";
  m_BuiltIn << "random";
  m_BuiltIn << "round";
  m_BuiltIn << "sin";
  m_BuiltIn << "sqrt";
  m_BuiltIn << "tan";

  m_BuiltIn << "document";
  m_BuiltIn << "window";
  m_BuiltIn << "navigator";
  m_BuiltIn << "userAgent";

  m_KeywordsGreen << "ez";
  m_KeywordsGreen << "Vec3";
  m_KeywordsGreen << "Quat";
  m_KeywordsGreen << "Color";
  m_KeywordsGreen << "Log";
  m_KeywordsGreen << "ezMath";
  m_KeywordsGreen << "Angle";
  m_KeywordsGreen << "Time";
  m_KeywordsGreen << "World";
  m_KeywordsGreen << "GameObject";
  m_KeywordsGreen << "Component";
  m_KeywordsGreen << "Message";
  m_KeywordsGreen << "TypescriptComponent";
  m_KeywordsGreen << "TickedTypescriptComponent";
  m_KeywordsGreen << "Tick";
  m_KeywordsGreen << "Initialize";
  m_KeywordsGreen << "Deinitialize";
  m_KeywordsGreen << "OnActivated";
  m_KeywordsGreen << "OnActivated";
  m_KeywordsGreen << "OnSimulationStarted";
  m_KeywordsGreen << "RegisterMessageHandlers";
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
          setFormat(start, text.length(), m_Colors[JSEdit::Comment]);
        }
        else if (ch == '/' && next != '*')
        {
          ++i;
          state = Regex;
        }
        else
        {
          if (!QString("(){}[]").contains(ch))
            setFormat(start, 1, m_Colors[JSEdit::Operator]);
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
          setFormat(start, i - start, m_Colors[JSEdit::Number]);
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
          if (m_KeywordsBlue.contains(token))
            setFormat(start, i - start, m_Colors[JSEdit::KeywordBlue]);
          if (m_KeywordsPink.contains(token))
            setFormat(start, i - start, m_Colors[JSEdit::KeywordPink]);
          if (m_KeywordsGreen.contains(token))
            setFormat(start, i - start, m_Colors[JSEdit::KeywordGreen]);
          else if (m_BuiltIn.contains(token))
            setFormat(start, i - start, m_Colors[JSEdit::BuiltIn]);
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
            setFormat(start, i - start, m_Colors[JSEdit::String]);
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
          setFormat(start, i - start, m_Colors[JSEdit::Comment]);
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
            setFormat(start, i - start, m_Colors[JSEdit::String]);
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
    setFormat(start, text.length(), m_Colors[JSEdit::Comment]);
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
