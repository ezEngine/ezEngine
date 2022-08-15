![ukraine](doc/ukraine.jpg)

![logo](doc/ads_logo.svg)

------------------

[![Build status](https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/workflows/linux-builds/badge.svg)](https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/actions?query=workflow%3Alinux-builds)
[![Build status](https://ci.appveyor.com/api/projects/status/qcfb3cy932jw9mpy/branch/master?svg=true)](https://ci.appveyor.com/project/githubuser0xFFFF/qt-advanced-docking-system/branch/master)
[![License: LGPL v2.1](https://img.shields.io/badge/License-LGPL%20v2.1-blue.svg)](gnu-lgpl-v2.1.md)

[What's new](https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/releases/latest)
[Documentation](doc/user-guide.md)

Qt Advanced Docking System lets you create customizable layouts using a full
featured window docking system similar to what is found in many popular
integrated development environments (IDEs) such as Visual Studio.

[![Video Advanced Docking](doc/advanced-docking_video.png)](https://www.youtube.com/watch?v=7pdNfafg3Qc)

## New and Noteworthy

The [release 3.8](https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/releases/latest)
adds the following features:

- option to close tabs with the middle mouse button
- `DeleteContentOnClose` flag for dynamic deletion and creation of dock widget
  content

The [release 3.7](https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/releases/tag/3.7.2)
adds the following features:

- support for **Qt6.**
- support for [empty dock area](doc/user-guide.md#empty-dock-area)

The [release 3.6](https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/releases/tag/3.6.3)
adds some nice new features:

- support for [central widget](doc/user-guide.md#central-widget) concept

![Central Widget](doc/central_widget.gif)

- support for [native floating widgets](doc/user-guide.md#floatingcontainerforcenativetitlebar-linux-only) on Linux

![FloatingContainerForceNativeTitleBar true](doc/cfg_flag_FloatingContainerForceNativeTitleBar_true.png)

Both features are contributions from ADS users. Read the [documentation](doc/user-guide.md)
to learn more about both new features.

The [release 3.5](https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/releases/tag/3.5.0)
adds the new [focus highlighting](doc/user-guide.md#focushighlighting) feature.
This optional feature enables highlighting of the focused dock widget like you
know it from Visual Studio.

![FocusHighlighting](doc/cfg_flag_FocusHighlighting.gif)

 [learn more...](doc/user-guide.md#focushighlighting)

## Features

### Overview

- [New and Noteworthy](#new-and-noteworthy)
- [Features](#features)
  - [Overview](#overview)
  - [Docking everywhere - no central widget](#docking-everywhere---no-central-widget)
  - [Docking inside floating windows](#docking-inside-floating-windows)
  - [Grouped dragging](#grouped-dragging)
  - [Perspectives for fast switching of the complete main window layout](#perspectives-for-fast-switching-of-the-complete-main-window-layout)
  - [Opaque and non-opaque splitter resizing](#opaque-and-non-opaque-splitter-resizing)
  - [Opaque and non-opaque undocking](#opaque-and-non-opaque-undocking)
  - [Tab-menu for easy handling of many tabbed dock widgets](#tab-menu-for-easy-handling-of-many-tabbed-dock-widgets)
  - [Many different ways to detach dock widgets](#many-different-ways-to-detach-dock-widgets)
  - [Supports deletion of dynamically created dock widgets](#supports-deletion-of-dynamically-created-dock-widgets)
  - [Python PyQt5 Bindings](#python-pyqt5-bindings)
- [Tested Compatible Environments](#tested-compatible-environments)
  - [Supported Qt Versions](#supported-qt-versions)
  - [Windows](#windows)
  - [macOS](#macos)
  - [Linux](#linux)
- [Build](#build)
- [Getting started / Example](#getting-started--example)
- [Developers](#developers)
- [License information](#license-information)
- [Alternative Docking System Implementations](#alternative-docking-system-implementations)
  - [KDDockWidgets](#kddockwidgets)
  - [QtitanDocking](#qtitandocking)
- [Donation](#donation)
- [Showcase](#showcase)
  - [Qt Creator IDE](#qt-creator-ide)
  - [Qt Design Studio](#qt-design-studio)
  - [CETONI Elements](#cetoni-elements)
  - [ezEditor](#ezeditor)
  - [D-Tect X](#d-tect-x)
  - [HiveWE](#hivewe)
  - [Ramses Composer](#ramses-composer)
  - [Plot Juggler](#plot-juggler)
  - [Notepad Next](#notepad-next)
  - [MetGem](#metgem)
  - [PRE Workbench](#pre-workbench)

### Docking everywhere - no central widget

There is no central widget like in the Qt docking system. You can dock on every
border of the main window or you can dock into each dock area - so you are
free to dock almost everywhere.

![Dropping widgets](doc/preview-dragndrop.png)

![Dropping widgets](doc/preview-dragndrop_dark.png)

### Docking inside floating windows

There is no difference between the main window and a floating window. Docking
into floating windows is supported.

![Docking inside floating windows](doc/floating-widget-dragndrop.png)

![Docking inside floating windows](doc/floating-widget-dragndrop_dark.png)

### Grouped dragging

When dragging the titlebar of a dock, all the tabs that are tabbed with it are 
going to be dragged. So you can move complete groups of tabbed widgets into
a floating widget or from one dock area to another one.

![Grouped dragging](doc/grouped-dragging.gif)

![Grouped dragging](doc/grouped-dragging_dark.png)

### Perspectives for fast switching of the complete main window layout

A perspective defines the set and layout of dock windows in the main
window. You can save the current layout of the dockmanager into a named
perspective to make your own custom perspective. Later you can simply
select a perspective from the perspective list to quickly switch the complete 
main window layout.

![Perspective](doc/perspectives.gif)

![Perspective](doc/perspectives_dark.png)

### Opaque and non-opaque splitter resizing

The advanced docking system uses standard QSplitters as resize separators and thus supports opaque and non-opaque resizing functionality of QSplitter. In some rare cases, for very complex widgets or on slow machines resizing via separator on the fly may cause flicking and glaring of rendered content inside a widget. The global dock manager flag `OpaqueSplitterResize` configures the resizing behaviour of the splitters. If this flag is set, then widgets are resized dynamically (opaquely) while interactively moving the splitters.

![Opaque resizing](doc/opaque_resizing.gif)

If this flag is cleared, the widget resizing is deferred until the mouse button is released - this is some kind of lazy resizing separator.

![Non-opaque resizing](doc/non_opaque_resizing.gif)

### Opaque and non-opaque undocking

By default, opaque undocking is active. That means, as soon as you drag a dock widget or a dock area with a number of dock widgets it will be undocked and moved into a floating widget and then the floating widget will be dragged around. That means undocking will take place immediatelly. You can compare this with opaque splitter resizing. If the flag `OpaqueUndocking` is cleared, then non-opaque undocking is active. In this mode, undocking is more like a standard drag and drop operation. That means, the dragged dock widget or dock area is not undocked immediatelly. Instead, a drag preview widget is created and dragged around to indicate the future position of the dock widget or dock area. The actual dock operation is only executed when the mouse button is released. That makes it possible, to cancel an active drag operation with the escape key.

The drag preview widget can be configured by a number of global dock manager flags:
- `DragPreviewIsDynamic`: if this flag is enabled, the preview will be adjusted dynamically to the drop area
- `DragPreviewShowsContentPixmap`: the created drag preview window shows a static copy of the content of the dock widget / dock are that is dragged
- `DragPreviewHasWindowFrame`: this flag configures if the drag preview is frameless like a QRubberBand or looks like a real window

The best way to test non-opaque undocking is to set the standard flags: `CDockManager::setConfigFlags(CDockManager::DefaultNonOpaqueConfig)`.

### Tab-menu for easy handling of many tabbed dock widgets

Tabs are a good way to quickly switch between dockwidgets in a dockarea. However, if the number of dockwidgets in a dockarea is too large, this may affect the usability of the tab bar. To keep track in this situation, you can use the tab menu. The menu allows you to quickly select the dockwidget you want to activate from a drop down menu.

![Tab menu](doc/tab_menu.gif)

### Many different ways to detach dock widgets

You can detach dock widgets and also dock areas in the following ways:

- by dragging the dock widget tab or the dock area title bar
- by double clicking the tab or title bar
- by using the detach menu entry from the tab and title bar drop down menu

### Supports deletion of dynamically created dock widgets

Normally clicking the close button of a dock widget will just hide the widget and the user can show it again using the toggleView() action of the dock widget. This is meant for user interfaces with a static amount of widgets. But the advanced docking system also supports dynamic dock widgets that will get deleted on close. If you set the dock widget flag `DockWidgetDeleteOnClose` for a certain dock widget, then it will be deleted as soon as you close this dock widget. This enables the implementation of user interfaces with dynamically created editors, like in word processing applications or source code development tools.

### Python PyQt5 Bindings

![Python Logo](doc/python_logo.png)

The Advanced Docking System comes with a complete Python integration based on
PyQt5 bindings. The package is available via [conda-forge](https://github.com/conda-forge/pyqtads-feedstock). The python integration has been contributed to this project
by the following people:

- [n-elie](https://github.com/n-elie)
- [Hugo Slepicka](https://github.com/hhslepicka)
- [K Lauer](https://github.com/klauer)

Latest working version: [3.5.2](https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/releases/tag/3.5.2)

## Tested Compatible Environments

### Supported Qt Versions

The library supports **Qt5** and **Qt6**.

### Windows

Windows 10 [![Build status](https://ci.appveyor.com/api/projects/status/qcfb3cy932jw9mpy/branch/master?svg=true)](https://ci.appveyor.com/project/githubuser0xFFFF/qt-advanced-docking-system/branch/master)

The library was developed on and for Windows. It is used in a commercial Windows application and is therefore constantly tested.

### macOS

macOS [![Build Status](https://travis-ci.org/githubuser0xFFFF/Qt-Advanced-Docking-System.svg?branch=master)](https://travis-ci.org/githubuser0xFFFF/Qt-Advanced-Docking-System)

The application can be compiled for macOS. A user reported, that the library works on macOS. If have not tested it.

![Advanced Docking on macOS](doc/macos.png)

### Linux

[![Build Status](https://travis-ci.org/githubuser0xFFFF/Qt-Advanced-Docking-System.svg?branch=master)](https://travis-ci.org/githubuser0xFFFF/Qt-Advanced-Docking-System)
[![Build status](https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/workflows/linux-builds/badge.svg)](https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/actions?query=workflow%3Alinux-builds)

Unfortunately, there is no such thing as a Linux operating system. Linux is a heterogeneous environment with a variety of different distributions. So it is not possible to support "Linux" like this is possible for Windows. It is only possible to support and test a small subset of Linux distributions. The library can be compiled for and has been developed and tested with the following Linux distributions:

- **Kubuntu 18.04 and 19.10**
- **Ubuntu 18.04, 19.10 and 20.04**

There are some requirements for the Linux distribution that have to be met:

- an X server that supports ARGB visuals and a compositing window manager. This is required to display the translucent dock overlays ([https://doc.qt.io/qt-5/qwidget.html#creating-translucent-windows](https://doc.qt.io/qt-5/qwidget.html#creating-translucent-windows)). If your Linux distribution does not support this, or if you disable this feature, you will very likely see issue [#95](https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/issues/95).
- Wayland is not properly supported by Qt yet. If you use Wayland, then you should set the session type to x11: `XDG_SESSION_TYPE=x11 ./AdvancedDockingSystemDemo`. You will find more details about this in issue [#288](https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/issues/288).

Screenshot Kubuntu:
![Advanced Docking on Kubuntu Linux](doc/linux_kubuntu_1804.png)

Screenshot Ubuntu:
![Advanced Docking on Ubuntu Linux](doc/linux_ubuntu_1910.png)

## Build

The Linux build requires private header files. Make sure that they are installed:

```bash
sudo apt install qtbase5-private-dev
```

Open the `ads.pro` file with QtCreator and start the build, that's it.
You can run the demo project and test it yourself.

## Getting started / Example

The following example shows the minimum code required to use the advanced Qt docking system.

*MainWindow.h*

```cpp
#include <QMainWindow>
#include "DockManager.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    
    // The main container for docking
    ads::CDockManager* m_DockManager;
};
```

*MainWindow.cpp*

```cpp
#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QLabel>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Create the dock manager after the ui is setup. Because the
    // parent parameter is a QMainWindow the dock manager registers
    // itself as the central widget as such the ui must be set up first.
    m_DockManager = new ads::CDockManager(this);

    // Create example content label - this can be any application specific
    // widget
    QLabel* l = new QLabel();
    l->setWordWrap(true);
    l->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    l->setText("Lorem ipsum dolor sit amet, consectetuer adipiscing elit. ");

    // Create a dock widget with the title Label 1 and set the created label
    // as the dock widget content
    ads::CDockWidget* DockWidget = new ads::CDockWidget("Label 1");
    DockWidget->setWidget(l);

    // Add the toggleViewAction of the dock widget to the menu to give
    // the user the possibility to show the dock widget if it has been closed
    ui->menuView->addAction(DockWidget->toggleViewAction());

    // Add the dock widget to the top dock widget area
    m_DockManager->addDockWidget(ads::TopDockWidgetArea, DockWidget);
}

MainWindow::~MainWindow()
{
    delete ui;
}
```

## Developers

- Uwe Kindler, Project Maintainer
- Manuel Freiholz

This work is based on and inspired by the 
[Advanced Docking System for Qt](https://github.com/mfreiholz/Qt-Advanced-Docking-System)
from Manuel Freiholz. I did an almost complete rewrite of his code to improve
code quality, readibility and to fix all issues from the issue tracker
of his docking system project.

## License information

[![License: LGPL v2.1](https://img.shields.io/badge/License-LGPL%20v2.1-blue.svg)](gnu-lgpl-v2.1.md)
This project uses the [LGPLv2.1 license](gnu-lgpl-v2.1.md)


## Alternative Docking System Implementations

If this Qt Advanced Docking System does not fit to your needs you may consider some of the alternative docking system solutions for Qt.

### KDDockWidgets

This is an advanced docking framework for Qt from [KDAB](https://www.kdab.com/). The interesting thing is, that they separated GUI code from logic, so they can easily provide a QtQuick backend in the future.

- [Blog post about KDDockWidgets](https://www.kdab.com/kddockwidgets/)
- [GitHub project](https://github.com/KDAB/KDDockWidgets)


### QtitanDocking

This is a commercial component from [Developer Machines](https://www.devmachines.com/) for Qt Framework that allows to create a Microsoft like dockable user interface. They also offer a lot of other interesting and useful components for Qt.

- [Product page](https://www.devmachines.com/qtitandocking-overview.html)

## Donation

If this project help you reduce time to develop or if you just like it, you can give me a cup of coffee :coffee::wink:.

<a href="https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=85R64TMMSY9T6">
  <img src="doc/donate.png" alt="Donate with PayPal" width="160"/>
</a>

## Showcase

### [Qt Creator IDE](https://www.qt.io/development-tools)

From version 4.12 on, Qt Creator uses the Advanced Docking Framework for its
Qt Quick Designer. This improves the usability when using multiple screens.

![Qt Creator](doc/showcase_qtcreator.png)

### [Qt Design Studio](https://www.qt.io/ui-design-tools)

Taken from the [Qt Blog](https://www.qt.io/blog/qt-design-studio-1.5-beta-released):

> The most obvious change in [Qt Design Studio 1.5](https://www.qt.io/blog/qt-design-studio-1.5-beta-released) is the integration of dock widgets using the Qt Advanced Docking System. This allows the user to fully customize the workspace and also to undock any view into its own top level window. This especially improves the usability when using multiple screens.

[![Qt Design Studio](doc/showcase_qt_design_studio_video.png)](https://youtu.be/za9KBWcFXEw?t=84)

### [CETONI Elements](https://www.cetoni.com/products/qmixelements/)

The CETONI Elements software from [CETONI](https://www.cetoni.com) is a comprehensive,
plugin-based and modular laboratory automation software for controlling CETONI devices using a joint graphical user interface. The software features a powerful script system to automate processes. The software uses the advanced docking system to give the user the freedom to arrange all the views and windows that are provided by the various plugins.

![CETONI_Elements](doc/showcase_qmix_elements.png)

### [ezEditor](https://github.com/ezEngine/ezEngine)

The ezEditor is a full blown graphical editor used for editing scenes and
importing and authoring assets for the [ezEngine](https://github.com/ezEngine/ezEngine) -
an open source C++ game engine in active development.

![ezEditor](doc/showcase_ezEngine_editor.png)

### [D-Tect X](https://www.duerr-ndt.com/products/ndt-software/d-tect-xray-inspection-software.html)

D-Tect X is a X-ray inspection software for industrial radiography. It is a state-of-the-art 64-bit application which supports GPU (Graphics Processing Unit) acceleration and takes full advantage of computers with multiple CPU cores. A large set of tools assist the user in image analysis and evaluation. Thanks to the Qt Advanced Docking System the flexible and intuitive user interface can be completely customized to  each user’s preference.

[learn more...](https://www.duerr-ndt.com/products/ndt-software/d-tect-xray-inspection-software.html)

![D-TectX](doc/showcase_d-tect-x.jpg)

### [HiveWE](https://github.com/stijnherfst/HiveWE)

HiveWE is a Warcraft III world editor. It focusses on speed and ease of use,
especially for large maps where the regular World Editor is often too slow and clunky.
It has a JASS editor with syntax hightlighting, tabs, code completion and more.
The JASS editor uses the Qt Advanced Docking System for the management and layout
of the open editor windows.

[learn more...](https://github.com/stijnherfst/HiveWE)

![HiveWE](doc/showcase_hivewe.png)

### [Ramses Composer](https://github.com/GENIVI/ramses-composer)

Ramses Composer is the authoring tool for the open source [RAMSES](https://github.com/GENIVI/ramses)
rendering ecosystem.

Ramses is a low-level rendering engine which is optimized for embedded hardware
mobile devices, automotive ECUs, IoT electronics. Ramses was initially developed
at the BMW Group and open-sourced in 2018 as part of a collaboration initiative
with the Genivi Alliance. It is an important part of the BMW infotainment cluster
and digital portfolio.

[learn more...](https://github.com/GENIVI/ramses-composer)

![RamsesComposer](doc/showcase_ramses_composer.png)

### [Plot Juggler](https://github.com/facontidavide/PlotJuggler)

PlotJuggler is a fast, powerful and intuitive tool to visualize time series.
It makes it easy to visualize data but also to analyze it. You can manipulate
your time series using a simple and extendable Transform Editor. Some of the
highlights are:

- Simple Drag & Drop user interface.
- Load data from file.
- Connect to live streaming of data.
- Save the visualization layout and configurations to re-use them later.
- Fast OpenGL visualization.
- Can handle thousands of timeseries and millions of data points.
- Transform your data using a simple editor: derivative, moving average, integral, etc…
- PlotJuggler can be easily extended using plugins.

[read more...](https://github.com/facontidavide/PlotJuggler)

[![Plot Juggler](doc/showcase_plot_juggler.png)](https://vimeo.com/480588113#t=46s)

### [Notepad Next](https://github.com/dail8859/NotepadNext)

Notepad Next is a cross-platform reimplementation of Notepad++ that uses the 
Advanced Docking System to arrange the open source files on the screen.

[read more...](https://github.com/dail8859/NotepadNext)

![NotepadNext](doc/showcase_notepad_next.png)

### [MetGem](https://metgem.github.io/)

MetGem is an open-source software for tandem mass-spectrometry data visualization.
It's key features are standalone molecular networking and t-SNE based projections.
MetGem uses the Qt-Advanced-Docking-System to manage docks and to create independent
molecular network views.

[read more...](https://metgem.github.io/)

![MetGem](doc/showcase_metgem.png)

### [PRE Workbench](https://luelista.github.io/pre_workbench/)

Protocol Reverse Engineering Workbench is a software to support researchers in reverse engineering protocols and documenting the results. It supports various sources to import protocol traffic from, helps the discovery process by displaying different views and heuristic-based highlighting on data, and aids in documenting and sharing findings.

PRE Workbench is a Python software and uses the ADS PyQt integration.

[read more...](https://luelista.github.io/pre_workbench/)

[![PRE Workbench](doc/showcase_pre_workbench.png)](https://youtu.be/U3op5UreV1Q)
