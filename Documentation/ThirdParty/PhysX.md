PhysX {#PhysX}
=====================

ez has an integration for the PhysX engine. However, you need to acquire the SDK yourself, which may include building the libraries.

The latest supported version is **3.4**

Download the SDK from [Nvidia.com](https://developer.nvidia.com/physx-sdk)

1. Download and build the PhysX libraries
2. In CMake, enable **EZ\_BUILD\_PHYSX**
3. Run *Configure*
5. Fill out **EZ\_PHYSX\_DIR** to point to the directory where the SDK is located.
6. Run *Generate*

Now in the solution the projects **PhysXPlugin**, **EditorPluginPhysX** and **EnginePluginPhysX** should appear.