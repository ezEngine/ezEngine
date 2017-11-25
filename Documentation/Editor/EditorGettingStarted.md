Getting Started with the Editor {#EditorGettingStarted}
===============================

This page describes how to get the editor up and running and how to configure a project.

Compiling
---------

The editor currently only builds on Windows and requires Qt. See \ref HowToSetupQt for details.


### Editor Code Structure

Once Qt support is enabled, there will be several additional libraries in the build solution:

  * **ToolsFoundation** implements general features that may be used by various tools. This library does not use any Qt code and is therefore independent from the UI system in use.

  * **GuiFoundation** implements various GUI functionality using Qt.

  * **EditorFramework** is the central library that implements the Editor specific feature set. It uses both **ToolsFoundation** and **GuiFoundation**. All other editor plugins will require to link against this library.

  * **EditorEngineProcess** is a separate process that is launched by the editor to handle all rendering and simulation related tasks. This is basically the game engine running, with some editor specific additions.

  * **EditorEngineProcessFramework** provides the functionality that plugins which run in the EditorEngineProcess need to communicate with the editor process.

  * **Editor** is the actual editor process. This does not contain much code, the main code is in **EditorFramework** and in the different plugins.

  * **EditorPluginScene** is the plugin for the editor process that adds all the functionality to handle 3D scenes and prefabs. This implements the most prominent editor features.

  * **EnginePluginScene** is the plugin for the engine process that implements the 'scene' related features.

  * **EditorPluginAssets** is a separate plugin that implements the most common asset types, such as meshes, material and textures.

All plugin names follow a scheme:
 * Regular runtime plugins use the postfix _Plugin_, e.g. _PhysXPlugin_
 * Plugins that run in the editor process use the prefix _EditorPlugin_, e.g. _EditorPluginPhysX_. This is very important, plugins that do not follow this naming rule will not be considered by the editor.
 * Plugins that run in the engine process of the editor use the prefix _EnginePlugin_, e.g. _EnginePluginPhysX_.


### Custom Editor Extensions

It is possible to extend the editor with custom plugins. This typically adds functionality for new (asset) document types, but it can also be used to add dialogs or panels to the existing scene plugin.

Although the entire scene editing is implemented through plugins, simpler examples to look at are the **EditorPluginPhysX** and **EditorPluginFmod**.

If one only wants to add custom component types, it is sufficient to only extend the engine side using engine plugins.

The **PhysXPlugin** and **FmodPlugin** projects are examples that show how to extend the engine runtime side.




First Time Opening the Sponza Sample Project
--------------------------------------------

The data for the Sponza project is very large which is why it is not checked in uncompressed. You need to extract some data manually first.

Go to **Trunk/Data/FreeContent/Sponza** and extract **Sponza.7z** into the same folder.
Go to **Trunk/Data/FreeContent/Trees** and extract **Trees.7z** into the same folder.

You now have all required source assets.

Run the Editor and select **Editor > Open Document** and open **Trunk/Data/EditorSamples/Sponza/Scenes/Sponza.ezScene**.

The editor will automatically open the project that the scene belongs to. It may complain about unknown types due to missing plugins. This is safe to ignore for now.

Most likely the viewport will only show **Missing Meshes** dummy objects at this point. That is because the engine runtime tries to load the final resource data, which is not yet available.

Make sure the **Asset Browser** panel is visible. You can toggle all panels from the **Panels** menu. In the Asset Browser toolbar there is a button showing a red arrow. The tooltip says "Transform All Assets". Click that and wait for the transformation process to finish. Afterwards the 3D viewport should refresh and the Sponza scene should show up.

If anything fails, you should find more information in the **Log** panel in the "Editor Log" section. If the Sponza scene does not show up you may have forgotten to extract the mesh or you extracted it to the wrong location.

If any asset data is missing, you can also inspect the **Asset Curator** panel for more detailed diagnostic information.



Setting Up a Custom Project
---------------------------

When you create a custom project, you must make a few configurations.

Select **Editor > Create Project** and select an empty folder where your project should end up.

Now select **Editor > Project Settings > Data Directories**. Click the button to add another folder. By default **Trunk/Data/Base** should already be included, because this folder contains all crucial shaders and materials for scene editing. You may want to add additional folders, such as the FreeContent folder.

After closing and reopening the project you are now good to go to create your first scene: **Editor > Create Document > MyFirst.ezScene**


Importing Assets
----------------

ezEngine represents assets with a dedicated document on the editor side. Typically to get e.g. a texture into the engine, you need to go to **Editor > Create Document** and choose **Texture Asset**. Then fill out the document properties to point to your source asset (e.g. a DDS or TGA file).

For common asset types (e.g. meshes and textures) you can automate parts of the process by selecting **Editor > Import Assets** and then browse for one or multiple source asset files. The following steps will automatically execute the basic steps, which saves a lot of time.






