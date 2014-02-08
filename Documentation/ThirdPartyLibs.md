Third Party Libraries {#ThirdPartyLibs}
=====================

The following libraries are used in ezEngine:

  * [zlib](http://www.zlib.net/): Provides algorithms for zip compression and decompression. It is used by ezCompressedStreamReader and ezCompressedStreamWriter.
  * [enet](http://enet.bespin.org/): An efficient and easy to use networking library, built on top of the UDP protocoll. It is used by ezTelemetry.
  * [SFML](http://www.sfml-dev.org/): This library provides a simple and portable interface for window creation, input handling and more. Used by ezWindow and ezStandardInputDevice on non-Windows platforms (Mac, Linux).
  * [GPA](http://software.intel.com/en-us/vcsource/tools/intel-gpa): When EZ_USE_PROFILING_GPA is enabled, ezEngine allows to use the Intel GPA tool for performance profiling.
  * [Lua](http://www.lua.org/): The Lua scripting language. Can be used directly or through ezLuaWrapper for easier access to common functionality.
  * [Silk Icons](http://www.famfamfam.com/lab/icons/silk): Icons from this set are used by some of our tools.