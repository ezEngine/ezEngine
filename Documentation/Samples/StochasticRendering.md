Stochastic Rendering Plugin {#StochasticRendering}
======================

The Stochastic Rendering Plugin is a sample runtime plugin, that extends the existing rendering pipeline. It adds a new render pass to do stochastic sampling over multiple frames. 
The result is accumulated while the camera is not moving. Every camera movement results in a reset of the stochastic data. 
The editor sample "Brdf Explorer" uses stochastic sampling to evaluate BRDFs without approximations.

The Editor Sample Brdf Explorer
-------------------------------------------

This sample uses stochastic sampling to compute compute the amount of reflected light off a surface without any aproximations.
The underlying BRDFs are computed as is and are evaluated using the monte carlo estimator resulting in a ground truth result comparable to offline raytracers.

To open the sample:
 * Start the Editor and open the Editor project in Data\EditorSamples\BrdfExplorer.
 * Open the "Main" scene
 * Switch the 3d view from "Editor View" to "Main View"
 
The view port will converge to the ground truth result as the scene is opened. Whenever the camera is moved the result of the stochastic rendering will be cleared and restarted.
The two rows initially show of the difference between importance sampling and uniform sampling. The upper row will use uniform sampling and the lower row will use importance sampling.
By modifying the two materials "BrdfA" and "BrdfB" you can play around with various options. The upper sphere row uses the "BrdfA" material and the lower row uses "BrdfB".
Each sphere uses a different roughness value.

The Stochastic Rendering Plugin
-------------------------------------------

The stochastic rendering plugin can be used as an example of how to extend the existing rendering functionality with plugins.
The newly added pass is used in a custom render pipeline which can be found inside the "Brdf Explorer" editor sample.






