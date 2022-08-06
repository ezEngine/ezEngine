#pragma once

#include <Foundation/Basics.h>

// <StaticLinkUtil::StartHere>
// all includes before this will be left alone and not replaced by the StaticLinkUtil
// all includes AFTER this will be removed by the StaticLinkUtil and updated by what is actually used throughout the library

#include <EditorPluginAudioSystem/EditorPluginAudioSystemDLL.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

#include <QLabel>
#include <QLayout>
