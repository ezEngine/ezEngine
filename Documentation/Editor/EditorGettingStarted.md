Getting Started with the Editor {#EditorGettingStarted}
===============================

This page describes how to get the editor up and running and how to configure a project.

Compiling
---------

The editor currently only builds on Windows. 

To compile the editor you need to have Qt 5 and you need to enable **EZ_ENABLE_QT_SUPPORT** in the CMake settings.


### Editor Code Structure

Once Qt support is enabled, there will be several additional libraries in the build solution:

  * **ToolsFoundation** implements general features that may be used by various tools. This library does not use any Qt code and is therefore independent from the UI system in use.
  * **GuiFoundation** implements various GUI functionality using Qt.
  * **EditorFramework** is the central library that implements the Editor specific feature set. It uses both **ToolsFoundation** and **GuiFoundation**. All other editor plugins will require to link against this library.
  
  * **EditorEngineProcess** is a separate process that is launched by the Editor to handle all rendering and simulation related tasks. This is basically the game engine running, with some editor specific additions.
  * **Editor** is the actual Editor process. This does not contain much code, the main code is in **EditorFramework** and in the different plugins.
  
  * **EditorPluginScene** is the plugin for the Editor process that adds all the functionality to handle 3D scenes and prefabs. This implements the most prominent editor features.
  * **EnginePluginScene** is the plugin for the Engine process that implements the 'scene' related features.
  
  * **EditorPluginAssets** is a separate plugin that implements the most common asset types, such as meshes, material and textures.


  
### Custom Editor Extensions

It is possible to extend the editor with custom plugins. This typically adds functionality for new (asset) document types, but it can also be used to add dialogs or panels to the existing scene plugin.

Although the entire scene editing is implemented through plugins, simpler examples to look at are the **EditorPluginPhysX** and **EditorPluginFmod**.

If one only wants to add custom component types, it is sufficient to only extend the engine side using engine plugins.

The **PhysXPlugin** and **FmodPlugin** projects are examples that show how to extend the engine runtime side.




First Time Running the Editor
-----------------------------

The editor has several dependencies that are not automatically handled by the build system.

  * **assimp.dll** is required by several Editor plugins. Copy the file from **Trunk/Code/ThirdParty/AssImp/binXY** into the proper output directory (**Trunk/Output/Bin/XYZ**). Make sure to copy the correct 32 or 64 bit DLL.
  * If you intend to use the PhysX or Fmod plugins, you also need to copy all their correct DLLs into the same folder. If you forget this, the editor will complain that it could not load certain plugin DLLs, although THOSE DLLs are in the right place.

When the editor launches for the first time, it will not load any plugins and therefore it will have basically no features. Select the menu item **Editor > Editor Settings > Plugins**. Check at least **EditorPluginAssets** and **EditorPluginScene**. If you have additional plugins compiled, you may enable these as well.

Afterwards you need to restart the editor. If you forgot to copy all required DLLs (e.g. Assimp, Fmod or PhysX) to the output directory, the editor may then complain that it could not load all plugins.

You can now either create a new empty project or open a sample project.




First Time Opening the Sponza Sample Project
--------------------------------------------

The data for the Sponza project is very large which is why it is not checked in uncompressed. You need to extract some data manually first.

Go to **Trunk/Data/FreeContent/Sponza** and extract **Sponza.7z** into the same folder.
Go to **Trunk/Data/FreeContent/Trees** and extract **Trees.7z** into the same folder.

You now have all required source assets.

Run the Editor and select **Editor > Open Document** and open **Trunk/Data/EditorSamples/Sponza/Scenes/Sponza.ezScene**.

The editor will automatically open the project that the scene belongs to. It may complain about unknown types due to missing plugins. This is safe to ignore for now.

Most likely the viewport will only show **Missing Meshes** dummy objects at this point. That is because the engine runtime tries to load the final resource data, which is not yet available.

Make sure the **Asset Browser** panel is visible. You can toggle all panels from the **Panels** menu. In the Asset Browser toolbar there is a button showing a white box with a red arrow. The tooltip says "Transform All Assets". Click that and wait for the transformation process to finish. Afterwards the 3D viewport should refresh and the Sponza scene should show up.

If anything fails, you should find more information in the **Log** panel in the "Editor Log" section (NOT the "Engine Log" section). If the Sponza scene does not show up you may have forgotten to extract the mesh or you extracted it to the wrong location.



Setting Up a Custom Project
---------------------------

When you create a custom project, you must make a few configurations.

Select **Editor > Create Project** and select an empty folder where your project should end up.

Now select **Editor > Project Settings > Data Directories**. Click the button to add another folder and choose **Trunk/Data/Base**. This folder contains all crucial shaders and materials for scene editing. Without it, the scene plugin will not function correctly. This step may get automated at some point.

After closing and reopening the project you are now good to go to create your first scene: **Editor > Create Document > MyFirst.ezScene**






  
  
  