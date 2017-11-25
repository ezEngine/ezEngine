Fmod {#FMOD}
=====================

ez has an integration for the Fmod sound system. However, you need to download the SDK yourself.

The latest supported version is **1.10.01**

Download the SDK from [Fmod.com](https://www.fmod.com/download)

1. Download and install the Fmod Studio API
2. For editing sounds you also need to download and install the Fmod Studio Tool
3. In CMake, enable **EZ\_BUILD\_FMOD**
4. Run *Configure*
5. Fill out **EZ\_FMOD\_DIR** to point to the directory where you installed the Fmod API.
6. Run *Generate*

Now in the solution the projects **FmodPlugin** and **EditorPluginFmod** should appear.