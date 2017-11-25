Third Party Libraries {#ThirdPartyLibs}
=====================

The following libraries are used in ezEngine:

  * [zlib](http://www.zlib.net/): Provides algorithms for zip compression and decompression. It is used by ezCompressedStreamReader and ezCompressedStreamWriter.
  * [enet](http://enet.bespin.org/): An efficient and easy to use networking library, built on top of the UDP protocoll. It is used by ezTelemetry.
  * [SFML](http://www.sfml-dev.org/): This library provides a simple and portable interface for window creation, input handling and more. Used by ezWindow and ezStandardInputDevice on non-Windows platforms (Mac, Linux).
  * [Lua](http://www.lua.org/): The Lua scripting language. Can be used directly or through ezLuaWrapper for easier access to common functionality.
  * [mikktspace](http://mmikkelsen3d.blogspot.ie/): Tangent space generation code by Morten S. Mikkelsen. See https://wiki.blender.org/index.php/Dev:Shading/Tangent_Space_Normal_Maps for more information. It is used by ezGeometry.
  * [Silk Icons](http://www.famfamfam.com/lab/icons/silk): Icons from this set are used by the tools.
  * [Microsoft DirectXTex](https://github.com/Microsoft/DirectXTex): Used by the ezTexConv tool for block compression.
  * [stb](https://github.com/nothings/stb): Public domain licensed code by Sean Barrett. Used by ezTexConv tool to import some of the supported formats.
  * [dear imgui](https://github.com/ocornut/imgui): Library for easy debug GUIs. MIT license. Also uses several files from [stb](https://github.com/nothings/stb)
  * [Assimp](http://www.assimp.org/): Open Asset Import Library, a portable Open Source library to import various well-known 3D model formats in a uniform manner.