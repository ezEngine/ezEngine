# rasterizer

This project is a state-of-the-art software occlusion culling system.

It's similar in spirit to Intel's https://github.com/GameTechDev/OcclusionCulling, but uses completely different techniques and is 2-3 times faster in single-threaded AVX mode when rendering the full set of occluders (no minimum size).

Checkout http://threadlocalmutex.com/?p=144 and http://threadlocalmutex.com/?p=163 for some implementation details.

Sample Data
===========

The folder Sponza contains a copy of Crytek's public domain Sponza model.

The folder Castle contains a copy of Intel's Occlusion Culling sample scene, redistributed here under the terms of the Intel Code Samples License.

Controls are WASD and cursor keys for the camera.

Requirements
============
- An AVX2 and FMA3 capable CPU (Haswell, Excavator or later)
- Visual Studio 2017 or higher

License
============

[!["Creative Commons Licence"](https://i.creativecommons.org/p/zero/1.0/88x31.png)](https://creativecommons.org/publicdomain/zero/1.0/)

The code in this repository is licensed under [CC0 1.0 Universal (CC0 1.0) Public Domain Dedication](https://creativecommons.org/publicdomain/zero/1.0/).
