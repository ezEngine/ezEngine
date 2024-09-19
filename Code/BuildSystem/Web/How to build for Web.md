# Emscripten Web + WebGPU Build

## Compiling for the Web

Use `Code/BuildSystem/Web/build-web.ps1` to compile:
* It downloads the Emscripten SDK for you.
* Installs and activates it.
* Generates the Solution for you with CMake, with the necessary build flags.
* Compiles the solution for you using *ninja*.
* Doesn't repeat unnecessary steps the next time.
	
The solution is generated in "Workspace/Web".
You can also execute "ninja" there directly, but build-web.ps1 is fast enough to just run it every time.

## Launching the app

1. Launch the local webserver:
	Data/Tools/Precompiled/webserver/start-server.ps1
2. In Chrome or Edge (Firefox doesn't work)
	http://localhost:1337/Web.html (page name depends on the app that you want to run)


## For Desktop WebGPU

1. Launch the CMake GUI (e.g. from `Data\Tools\Precompiled\cmake\bin\cmake-gui.exe`).
2. Enable `EZ_BUILD_EXPERIMENTAL_VULKAN` and `EZ_BUILD_EXPERIMENTAL_WEBGPU`.
3. Configure + Generate (this adds A LOT of thirdparty projects).
4. Build the Visual Studio solution.
5. Some samples now run with the WebGPU renderer.
