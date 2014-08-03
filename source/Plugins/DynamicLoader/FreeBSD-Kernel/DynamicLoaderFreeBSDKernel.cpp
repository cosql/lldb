//===-- DynamicLoaderFreeBSDKernel.cpp --------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// C Includes
#include <libgen.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/linker.h>
#include <sys/stat.h>
// C++ Includes
#include <iostream>
#include <sstream>
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
    std::vector<std::string> kld_suffixes = { ".debug", ".symbols", ""};
    // http://svnweb.freebsd.org/base/head/sys/sys/linker.h
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
      m_kernel_load_addr(LLDB_INVALID_ADDRESS)
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

bool
DynamicLoaderFreeBSDKernel::CheckKLDPath (std::string path)
{
    for (std::vector<std::string>::iterator suffix = kld_suffixes.begin();
         suffix != kld_suffixes.end(); suffix++) {
        std::string kld_path = path + *suffix;
        if (IsKLDOK(kld_path))
            return true;
        suffix++;
    }
    return false;
}

bool
DynamicLoaderFreeBSDKernel::IsKLDOK (std::string path)
{
    struct stat sb;

    if (stat(path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode))
        return true;
    return false;
}

bool
DynamicLoaderFreeBSDKernel::FindKLDPath (std::string filename, std::string& path)
{
    char *kernel_dir;
    ModuleSP executable = GetTargetExecutable();
    if (executable) {
        kernel_dir = dirname(executable->GetSpecificationDescription().c_str());
           if (kernel_dir != NULL) {
               path = std::string(kernel_dir) + "/" + filename;
            if (CheckKLDPath(path))
                return true;
        }
    }
    if (!m_module_path.empty()) {
        std::stringstream ss(m_module_path);
        std::string module_dir;
        while (std::getline(ss, module_dir, ';')) {
             path = module_dir + "/" + filename;
             if (CheckKLDPath(path))
                 return true;
        }
    }
    return false;
}

// int
// DynamicLoaderFreeBSDKernel::FindKLDAddress(std::string kld_name,
//                                              addr_t& kld_addr)
// {
//     addr_t kld;
//     char *kld_filename;
//     char *filename;
//     int error;

//     if (m_linker_files_addr == 0 || address_offset == 0 ||
//         filename_offset == 0 || next_offset == 0)
//         return 0;

//     filename = basename(kld_name.c_str());
//     for (kld = read_pointer(m_linker_files_addr); kld != 0;
//          kld = read_pointer(kld + off_next)) {
//         /* Try to read this linker file's filename. */
//         target_read_string(read_pointer(kld + off_filename),
//                            &kld_filename, PATH_MAX, &error);
//         if (error)
//             continue;

//         /* Compare this kld's filename against our passed in name. */
//         if (strcmp(kld_filename, filename) != 0) {
//             xfree(kld_filename);
//             continue;
//         }
//         xfree(kld_filename);

//         /*
//          * We found a match, use its address as the base
//          * address if we can read it.
//          */
//         *address = read_pointer(kld + off_address);
//         if (*address == 0)
//             return 0;
//         return 1;
//     }
//     return 0;
// }
