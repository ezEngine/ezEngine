#if defined(__clang__) || defined(__GNUC__)
	#if defined(__clang__)
		#pragma message("COMPILER:'clang'")
	#else
		#pragma message("COMPILER:'gcc'")
	#endif
	
	#if defined(__x86_64__) 
		#pragma message("ARCH:'x64'")
	#elif defined(__i386__)
		#pragma message("ARCH:'x86'")
	#elif defined(__aarch64__)
		#pragma message("ARCH:'arm64'")
	#elif defined(__arm__)
		#pragma message("ARCH:'arm32'")
	#elif defined(__EMSCRIPTEN__)
		#pragma message("ARCH:'emscripten'")
	#else
		#error unhandled clang/gcc architecture
	#endif
#elif defined(_MSC_VER)
	#define STRINGIFY(s) STRINGFY_HELPER(s)
	#define STRINGFY_HELPER(s) #s

	#pragma message ("COMPILER:'msvc'")
	#pragma message ("MSC_VER:'" STRINGIFY(_MSC_VER) "'")
	#ifdef _M_AMD64
		#pragma message ("ARCH:'x64'")
	#elif defined(_M_IX86)
		#pragma message ("ARCH:'x86'")
	#elif defined(_M_ARM64)
		#pragma message ("ARCH:'arm64'")
	#elif defined(_M_ARM)
		#pragma message ("ARCH:'arm32'")
	#else
		#error unhandled msvc architecture
	#endif
#else
	#error unhandled compiler
#endif