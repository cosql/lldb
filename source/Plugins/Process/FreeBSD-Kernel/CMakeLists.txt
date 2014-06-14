set(LLVM_NO_RTTI 1)

include_directories(.)
include_directories(../POSIX)
include_directories(../Utility)

add_lldb_library(lldbPluginProcessFreeBSDKernel
  ProcessFreeBSDKernel.cpp
  RegisterContextFreeBSDKernel_x86_64.cpp
  ThreadFreeBSDKernel.cpp
  )
