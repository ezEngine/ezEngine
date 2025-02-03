#pragma once

#include <Foundation/Basics.h>

// <StaticLinkUtil::StartHere>
// all include's before this will be left alone and not replaced by the StaticLinkUtil
// all include's AFTER this will be removed by the StaticLinkUtil and updated by what is actually used throughout the library

#include <EditorPluginAngelScript/EditorPluginAngelScriptDLL.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionManager.h>

#include <QLabel>
#include <QLayout>
#include <QSyntaxHighlighter>
#include <QTextEdit>
