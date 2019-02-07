#pragma once

#include <Foundation/Basics.h>

// <StaticLinkUtil::StartHere>
// all include's before this will be left alone and not replaced by the StaticLinkUtil
// all include's AFTER this will be removed by the StaticLinkUtil and updated by what is actually used throughout the library

#include <EditorPluginProceduralPlacement/EditorPluginProceduralPlacementDLL.h>


#include <Foundation/Strings/StringBuilder.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/Utilities/Node.h>

#include <QLabel>
#include <QLayout>
