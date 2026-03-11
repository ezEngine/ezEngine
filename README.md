# ezEngine

ezEngine is an open source C++ game engine.

Watch the trailer below for a quick feature overview:

[![ezEngine Trailer](https://img.youtube.com/vi/S342o8ZmPdA/0.jpg)](https://www.youtube.com/watch?v=S342o8ZmPdA)

Visit <http://ezEngine.net> for documentation, samples and detailed build instructions.

![ezEngine Screenshot](https://ezengine.net/pages/samples/media/showcase-1.jpg)

## Supported Platforms

The full engine functionality is currently only available on Windows, because the renderer uses Direct3D 11. Work on porting the renderer to Vulkan is ongoing. There is an experimental Linux port of the editor, but it is not yet functional enough for productive use.

All non-rendering related functionality compiles on Windows, Android, Linux and MacOS. See [this page](http://ezengine.net/pages/docs/build/supported-platforms.html) for details.

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

## Design Philosophy

EZ is built in a modular way, enabling users to either use all available functionality, or to pick and choose individual features and build the rest themselves. Larger features are implemented through engine and editor plugins and can therefore easily be removed or replaced. For instance sound (Fmod), physics (Jolt) and particle effects are all provided through plugins.

EZ puts a strong emphasis on a solid foundation that is both easy and efficient to use. Even if you do not use the rendering functionality, EZ has a lot to offer to build your own engine on top of.

Finally, EZ comes with a [feature rich editor](http://ezengine.net/pages/getting-started/editor-overview.html) that makes it possible to quickly prototype your game using [visual scripting](https://ezengine.net/pages/docs/custom-code/visual-script/visual-script-overview.html) and [custom C++ code](https://ezengine.net/pages/docs/custom-code/cpp/cpp-project-generation.html).

## Screenshots, Videos, Samples

* Here is [a variety of pretty pictures](https://ezengine.net/pages/samples/screenshots.html).
* For tutorial videos, see [our YouTube channel](https://www.youtube.com/@ezEngine).
* Available sample projects are [listed in our documentation](https://ezengine.net/pages/samples/samples-overview.html)

## Documentation and FAQ

* For high-level feature documentation see [our website](https://ezengine.net/pages/docs/docs-overview.html).
* Code API documentation is [available here](https://ezengine.github.io/api-docs/).
* Also see the page of [frequently asked questions](https://ezengine.net/pages/getting-started/faq.html).

## Contributing

Contributions are always welcome. Please see [this page](https://ezengine.net/pages/getting-started/how-to-contribute.html) for details on how you can contribute.

## Contact

If you have a question, [contact us](http://ezengine.net/pages/contact.html).
