# ezEngine

ezEngine is an open source C++ game engine.

## Prebuilt Binaries

See the [releases](https://github.com/ezEngine/ezEngine/releases) for recent changes and prebuilt binaries. Note, however, that releases are infrequent and you are generally expected to build the engine yourself.

## Building the Engine

These are the instructions for Windows. For other platforms [see this page](https://ezengine.net/pages/docs/build/building-ez.html).

Open a Windows Terminal to clone and build the engine:

1. `git clone https://github.com/ezEngine/ezEngine.git`
1. `cd ezEngine`
1. `.\GenerateWin64vs2022.bat`
1. Open the Visual Studio solution `Workspace\vs2022x64\ezEngine_vs2022x64.sln` and build everything.
1. Launch the `Editor` project from Visual Studio and open one of the sample projects.

## Description

ezEngine is currently mainly developed on Windows, and higher level functionality such as rendering and the tools are only available there, but the core libraries are also available for [other platforms](http://ezengine.net/pages/docs/build/supported-platforms.html) such as Mac and Linux.

EZ is built in a modular way, enabling users to either use all available functionality, or to pick and choose individual features and build the rest yourself. Larger features are implemented through engine and editor plugins and can therefore be easily removed or replaced. For instance sound (Fmod), physics (PhysX) and particle effects are all provided through runtime plugins.

The [ezEditor](http://ezengine.net/pages/getting-started/editor-overview.html) is a full blown editor used for editing scenes and working with assets.

If you want to learn more, please [have a look at the documentation](http://ezengine.net/). It is very straight-forward to [build EZ](http://ezengine.net/pages/docs/build/building-ez.html) and there are a [couple of samples](http://ezengine.net/pages/samples/samples-overview.html), [screenshots](http://ezengine.net/pages/samples/screenshots.html) and [videos](http://ezengine.net/pages/getting-started/videos.html) to get you started. Finally, you can also get the ezEngine [API documentation](http://ezengine.net/pages/docs/api-docs.html).

If you have a question, [contact us](http://ezengine.net/pages/contact.html).

## Screenshots

[ezEditor](https://ezengine.net/pages/getting-started/editor-overview.html):

![ezEditor](https://ezengine.net/pages/docs/editor/media/ezEditor.jpg)

![Testing Chamber](https://ezengine.net/pages/samples/media/tc1.jpg)

[RTS Sample](https://ezengine.net/pages/samples/rts.html):

![RTS1](https://ezengine.net/pages/samples/media/rts1.jpg)

[Asteroids Sample](https://ezengine.net/pages/samples/asteroids.html):

![Asteroids](https://ezengine.net/pages/samples/media/asteroids1.jpg)

[ezInspector](https://ezengine.net/pages/docs/tools/inspector.html):

![ezInspector](https://ezengine.net/pages/docs/tools/media/inspector.jpg)
