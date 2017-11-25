FBX {#FBX}
=====================

By default ez uses Assimp for asset import. This has some support for importing FBX files. For more extensive support, you can enable the use of the official FBX SDK.

The last tested FBX version is **2018.1.1**

Download the SDK from [Autodesk.com](https://www.autodesk.com/products/fbx/overview)

1. Download and install the FBX SDK.
2. In CMake, enable **EZ\_BUILD\_OFFICIAL\_FBX\_SDK**.
3. Run *Configure*
4. Fill out **EZ\_FBXSDK\_DIR** to point to the directory where you installed the FBX SDK.
5. Run *Generate*

Now in the solution the macro **BUILDSYSTEM\_BUILD\_WITH\_OFFICIAL\_FBX\_SDK** should be defined and the *ModelImporter* library should make use of the official FBX SDK during asset import.