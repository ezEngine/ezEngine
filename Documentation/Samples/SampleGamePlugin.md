Sample Game Plugin {#SampleGamePlugin}
======================

The *SampleGamePlugin* demonstrates the basics of how to build a custom plugin for game code that can run both in a stand-alone application (such as *ezPlayer*) as well as inside the editor.

**Note:** The project is only available when the solution is built with **EZ_BUILD_GAMES** activated.

GameState
---------

The *SampleGameState* class shows how to implement a simple game state that adds high-level game logic, such as handling a game UI. See ezGameState and ezGameApplication for further details.

Components
----------

* The *DemoComponent* shows how to modify the transform of an object dynamically.

* The *RecursiveGrowthComponent* shows how to create new objects at runtime, specifically when the component gets activated for the first time.

For further details see ezComponent.

Project
-------

Under *Code/Games/SampleGame* you will find an editor project which uses the SampleGamePlugin. Note that the project references the plugin as a runtime plugin (under *Editor > Project Settings > Engine Plugins*). This makes the custom components available to the editor. The effect of the *RecursiveGrowthComponent* can only be observed by stepping through the code, it has no visual output.

When you press 'Play' in the editor, the scene will be simulated, thus the custom components, such as the DemoComponent, will take effect.

When you press 'Play the Game' a full game window is launched and now even the custom game-state is instantiated and executed. Consequently the UI will appear and you can interact with it. Note that this still runs inside the editor process.

You can also export the scene (*Ctrl+E*) and run it externally (*Ctrl+R*) in the stand-alone *ezPlayer* application.


