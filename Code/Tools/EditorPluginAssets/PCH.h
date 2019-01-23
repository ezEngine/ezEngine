#pragma once

#include <Foundation/Basics.h>
#include <QWidget>

// <StaticLinkUtil::StartHere>
// all include's before this will be left alone and not replaced by the StaticLinkUtil
// all include's AFTER this will be removed by the StaticLinkUtil and updated by what is actually used throughout the library

#include <Plugin.h>

#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Image/ImageConversion.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <Core/Assets/AssetFileHeader.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>

#include <QtWidgets>
#include <QWidget>
#include <QPainter>
#include <QPaintEngine>
#include <QLabel>
#include <QLayout>
#include <QSplitter>
#include <QTimer>
#include <QTextEdit>
#include <QItemSelectionModel>
#include <QToolBar>
#include <QPushButton>
#include <qevent.h>
#include <QInputDialog>
#include <QTreeView>
#include <QAbstractItemModel>
#include <QIcon>
#include <QStringList>
#include <QToolBar>
#include <QPushButton>
#include <qevent.h>
#include <QInputDialog>
