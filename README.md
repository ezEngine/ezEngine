# ezEngine

ezEngine is an open source C++ game engine. It is currently mainly developed on Windows, and higher level functionality such as rendering and the tools are only available there, but the core libraries are also available for [other platforms](http://ezengine.net/build/supported-platforms.html) such as Mac and Linux.

EZ is built in a modular way, enabling users to either use all available functionality, or to pick and choose individual features and build the rest yourself. Larger features are implemented through engine and editor plugins and can therefore be easily removed or replaced. For instance sound (Fmod), physics (PhysX) and particle effects are all provided through runtime plugins.

The [ezEditor](https://ezengine.net/getting-started/editor-overview.html) is a full blown editor used for editing scenes and working with assets.

If you want to learn more, please [have a look at the documentation](http://ezengine.net/). It is very straight-forward to [build EZ](http://ezengine.net/build/building-ez.html) and there are a [couple of samples](http://ezengine.net/#samples) and [videos](http://ezengine.net/getting-started/videos.html) to get you started. Finally, you can also get the ezEngine [API documentation](http://ezengine.net/getting-started/api-docs.html).

If you have a question, [contact us](https://ezengine.net/getting-started/contact.html).

## Build Status

| Platform        | `dev` branch  |
| ------          | ------        |
| Windows-x64     | ![Windows-x64](https://img.shields.io/azure-devops/build/ezEngineCI/3be75850-43b5-4a0a-a9b3-9b1693a5beed/1/dev?style=flat-square) |
| WindowsUWP-x86  | ![WindowsUWP-x86](https://img.shields.io/azure-devops/build/ezEngineCI/3be75850-43b5-4a0a-a9b3-9b1693a5beed/4/dev?style=flat-square) |
| Linux-x64       | ![Linux-x64](https://img.shields.io/azure-devops/build/ezEngineCI/3be75850-43b5-4a0a-a9b3-9b1693a5beed/2/dev?style=flat-square) |
| MacOS-x64       | ![MacOS-x64](https://img.shields.io/azure-devops/build/ezEngineCI/3be75850-43b5-4a0a-a9b3-9b1693a5beed/3/dev?style=flat-square) |
| Android-x86     | ![Android-x86](https://img.shields.io/azure-devops/build/ezEngineCI/3be75850-43b5-4a0a-a9b3-9b1693a5beed/5/dev?style=flat-square) |

> **Note:**
> 
> Please be aware that features such as rendering and the editor are only available on Windows-x64. On all other platforms we only compile and test the core engine functionality.

## Screenshots

[ezEditor](http://ezengine.net/getting-started/editor-overview.html):

![ezEditor](http://ezengine.net/editor/media/ezEditor.jpg)

![Testing Chamber](https://ezengine.github.io/docs/samples/media/tc1.jpg)

[RTS Sample](http://ezengine.net/samples/media/tc1.jpg):

![RTS1](http://ezengine.net/samples/media/rts1.jpg)

[Asteroids Sample](http://ezengine.net/samples/asteroids.html):

![Asteroids](http://ezengine.net/samples/media/asteroids1.jpg)

[ezInspector](http://ezengine.net/tools/inspector.html):

![ezInspector](http://ezengine.net/tools/media/inspector.jpg)
