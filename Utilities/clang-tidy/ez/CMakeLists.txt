set(LLVM_LINK_COMPONENTS
  FrontendOpenMP
  Support
  )

add_clang_library(clangTidyEzModule
  EzTidyModule.cpp
  
  NameCheck.cpp

  LINK_LIBS
  clangTidy
  clangTidyReadabilityModule
  clangTidyUtils

  DEPENDS
  omp_gen
  )

clang_target_link_libraries(clangTidyEzModule
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  )
