#pragma once

#include <Foundation/Basics.h>

// <StaticLinkUtil::StartHere>
// all include's before this will be left alone and not replaced by the StaticLinkUtil
// all include's AFTER this will be removed by the StaticLinkUtil and updated by what is actually used throughout the library

#include <EditorPluginFileserve/EditorPluginFileserveDLL.h>

#include <FileservePlugin/Fileserver/Fileserver.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <GuiFoundation/GuiFoundationDLL.h>

#include <QAbstractSocket>
#include <QHostAddress>
#include <QInputDialog>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QSettings>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>
