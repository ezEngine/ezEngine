This folder is for custom games.

If you want to build your own game, you can compile ez and then copy everything you need into your own project structure.
However, it may be easier to integrate your game code directly into the build structure of ezEngine.

This folder is meant to hold your custom game code. E.g. engine and editor plugins. To keep the two projects separate,
it is convenient to use a symlink to a folder somewhere else.

To create a symlink (softlink / junction) in Windows, use the following command:

	mklink /D C:\ezEngine\Trunk\Games\EnginePluginGame C:\CustomRepository\Trunk\Games\EnginePluginGame

Do this for every folder that you need to integrate into the SDK.

This allows to compile and run your game directly from the ezEngine build, but you can keep your source code in another location.