//===-- DynamicLoaderFreeBSDKernel.cpp --------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// C Includes
#include <sys/types.h>
#include <sys/stat.h>
// C++ Includes
// Other libraries and framework includes
#include "lldb/Core/PluginManager.h"
#include "lldb/Core/Log.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/ModuleSpec.h"
#include "lldb/Core/Section.h"
#include "lldb/Symbol/ObjectFile.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/Thread.h"
#include "lldb/Target/ThreadPlanRunToAddress.h"
#include "lldb/Breakpoint/BreakpointLocation.h"

#include "DynamicLoaderFreeBSDKernel.h"

using namespace lldb;
using namespace lldb_private;

namespace {
    static const char *kld_suffixes[] = {
        ".debug",
        ".symbols",
        "",
        NULL
    };
};

void
DynamicLoaderFreeBSDKernel::Initialize()
{
    PluginManager::RegisterPlugin(GetPluginNameStatic(),
                                  GetPluginDescriptionStatic(),
                                  CreateInstance);
}

void
DynamicLoaderFreeBSDKernel::Terminate()
{
}

lldb_private::ConstString
DynamicLoaderFreeBSDKernel::GetPluginName()
{
    return GetPluginNameStatic();
}

lldb_private::ConstString
DynamicLoaderFreeBSDKernel::GetPluginNameStatic()
{
    static ConstString g_name("FreeBSDKernel-DYLD");
    return g_name;
}

const char *
DynamicLoaderFreeBSDKernel::GetPluginDescriptionStatic()
{
    return "Dynamic loader plug-in that watches for shared library "
           "loads/unloads in FreeBSD Kernel.";
}

void
DynamicLoaderFreeBSDKernel::GetPluginCommandHelp(const char *command, Stream *strm)
{
}

uint32_t
DynamicLoaderFreeBSDKernel::GetPluginVersion()
{
    return 1;
}

DynamicLoader *
DynamicLoaderFreeBSDKernel::CreateInstance(Process *process, bool force)
{
    bool create = force;
    if (!create)
    {
        const llvm::Triple &triple_ref = process->GetTarget().GetArchitecture().GetTriple();
        if (triple_ref.getOS() == llvm::Triple::FreeBSD)
            create = true;
    }
    
    if (create)
        return new DynamicLoaderFreeBSDKernel (process);
    return NULL;
}

DynamicLoaderFreeBSDKernel::DynamicLoaderFreeBSDKernel(Process *process)
    : DynamicLoader(process),
      m_kernel_load_address(LLDB_INVALID_ADDRESS)
{
}

DynamicLoaderFreeBSDKernel::~DynamicLoaderFreeBSDKernel()
{
}

void
DynamicLoaderFreeBSDKernel::DidAttach()
{

}

void
DynamicLoaderFreeBSDKernel::DidLaunch()
{
    ModuleSP executable;

    executable = GetTargetExecutable();

    if (executable.get())
    {
        ModuleList module_list;
        module_list.Append(executable);
        m_process->GetTarget().ModulesDidLoad(module_list);
    }
}

Error
DynamicLoaderFreeBSDKernel::ExecutePluginCommand(Args &command, Stream *strm)
{
    return Error();
}

Log *
DynamicLoaderFreeBSDKernel::EnablePluginLogging(Stream *strm, Args &command)
{
    return NULL;
}

Error
DynamicLoaderFreeBSDKernel::CanLoadImage()
{
    return Error();
}

void
DynamicLoaderFreeBSDKernel::RefreshModules()
{

}

ThreadPlanSP
DynamicLoaderFreeBSDKernel::GetStepThroughTrampolinePlan(Thread &thread, bool stop)
{
    ThreadPlanSP thread_plan_sp;

    return thread_plan_sp;
}

void
DynamicLoaderFreeBSDKernel::LoadAllCurrentModules()
{
}

addr_t
DynamicLoaderFreeBSDKernel::ComputeLoadOffset()
{
    return 0;
}

int
DynamicLoaderFreeBSDKernel::check_kld_path (std::string path)
{
    const char **suffix;

    suffix = kld_suffixes;
    while (*suffix != NULL) {
        std::string kld_path = path + *suffix;
        if (kld_ok(path))
            return (1);
        suffix++;
    }
    return (0);
}

int
DynamicLoaderFreeBSDKernel::kld_ok (std::string path)
{
    struct stat sb;

    if (stat(path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode))
        return (1);
    return (0);
}

int
DynamicLoaderFreeBSDKernel::find_kld_path (std::string filename, std::string path)
{
    char *module_path;
    char *kernel_dir, *module_dir, *cp;
    int error;

    // if (exec_bfd) {
    //     kernel_dir = dirname(bfd_get_filename(exec_bfd));
    //     if (kernel_dir != NULL) {
    //         snprintf(path, path_size, "%s/%s", kernel_dir,
    //                  filename);
    //         if (check_kld_path(path, path_size))
    //             return (1);
    //     }
    // }
    // if (module_path_addr != 0) {
    //     target_read_string(module_path_addr, &module_path, PATH_MAX,
    //                        &error);
    //     if (error == 0) {
    //         make_cleanup(xfree, module_path);
    //         cp = module_path;
    //         while ((module_dir = strsep(&cp, ";")) != NULL) {
    //             snprintf(path, path_size, "%s/%s", module_dir,
    //                      filename);
    //             if (check_kld_path(path, path_size))
    //                 return (1);
    //         }
    //     }
    // }
    return (0);
}
