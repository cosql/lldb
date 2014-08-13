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
#include <kvm.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
// C++ Includes
// Other libraries and framework includes
#include "lldb/Core/PluginManager.h"
#include "lldb/Core/Log.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/ModuleSpec.h"
#include "lldb/Core/Section.h"
#include "lldb/Symbol/ObjectFile.h"
#include "lldb/Symbol/SymbolVendor.h"
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
    m_kernel_process = static_cast<ProcessFreeBSDKernel*>(process);
    InitLoadSpecs();
}

DynamicLoaderFreeBSDKernel::~DynamicLoaderFreeBSDKernel()
{
}

void
DynamicLoaderFreeBSDKernel::DidAttach()
{
     LoadAllCurrentModules();
    // printf ("load %s\n", FindKLDAddress("dick.ko", addr) ? "succeeded" : "failed");
}

void
DynamicLoaderFreeBSDKernel::DidLaunch()
{
    printf ("function called %s %s\n", __FILE__, __FUNCTION__);
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

ThreadPlanSP
DynamicLoaderFreeBSDKernel::GetStepThroughTrampolinePlan(Thread &thread, bool stop)
{
    ThreadPlanSP thread_plan_sp;

    return thread_plan_sp;
}

void
DynamicLoaderFreeBSDKernel::LoadAllCurrentModules()
{
    ModuleList &module_list = m_process->GetTarget().GetImages();
    const ArchSpec &arch_spec = m_process->GetTarget().GetArchitecture();

    char kld_filename[PATH_MAX];
    Error error;

    if (m_linker_files_addr == 0 || m_address_offset == 0 ||
        m_filename_offset == 0 || m_next_offset == 0)
        return;

    for (addr_t kaddr = ReadPointer (m_linker_files_addr); kaddr != 0;
         kaddr = ReadPointer (kaddr + m_next_offset)) {
        if (kaddr == m_kernel_load_addr) {
            continue;
        }
        ReadMemory (ReadPointer(kaddr + m_filename_offset),
                    kld_filename, PATH_MAX, error);

        std::string path;
        if (FindKLDPath(kld_filename, path)) {
            addr_t kld_addr = ReadPointer (kaddr + m_address_offset);
            FileSpec file_spec (path.c_str(), true);
            ModuleSP module_sp(new Module(file_spec, arch_spec, NULL, kld_addr));
            module_list.AppendIfNeeded(module_sp);
        }
    }
    m_process->GetTarget().ModulesDidLoad(module_list);
}

void
DynamicLoaderFreeBSDKernel::RefreshModules()
{
}

void
DynamicLoaderFreeBSDKernel::InitLoadSpecs()
{
    addr_t addr;
    char buf[PATH_MAX];
    char *cp, *module_dir;
    int word_bytes;

    const ArchSpec &arch = m_process->GetTarget().GetArchitecture();
    if (arch.GetMachine() == llvm::Triple::mips64 ||
        arch.GetMachine() == llvm::Triple::x86_64)
        word_bytes = 8;
    else
        word_bytes = 4;

    ModuleSP module = m_process->GetTarget().GetExecutableModule();

    addr = m_kernel_process->LookUpSymbolAddressInModule(module, "linker_path");
    kvm_t * kd = static_cast<ProcessFreeBSDKernel *>(m_process)->GetKVM();
    kvm_read(kd, addr, buf, PATH_MAX);
    cp = buf;
    while ((module_dir = strsep(&cp, ";")) != NULL) {
        m_module_paths.push_back(module_dir);
    }

    m_kernel_file_addr =
        m_kernel_process->LookUpSymbolAddressInModule(module, "linker_kernel_file");

    m_kernel_load_addr = ReadPointer(m_kernel_file_addr);

    m_linker_files_addr =
        m_kernel_process->LookUpSymbolAddressInModule(module, "linker_files");

    m_next_offset = word_bytes + sizeof(int) * 4;
    m_filename_offset = m_next_offset + word_bytes * 2;
    m_pathname_offset = m_filename_offset + word_bytes;
    /// FIXME: padding issues remains, using 64 for amd64
    m_address_offset = 64;
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
    for (std::vector<std::string>::iterator it = m_module_paths.begin();
         it != m_module_paths.end(); it++) {
        path = *it + "/" + filename;
        if (CheckKLDPath(path))
            return true;
    }
    return false;
}

bool
DynamicLoaderFreeBSDKernel::FindKLDAddress(std::string kld_name,
                                           addr_t& kld_addr)
{
    char kld_filename[PATH_MAX];
    char *filename;
    Error error;

    if (m_linker_files_addr == 0 || m_address_offset == 0 ||
        m_filename_offset == 0 || m_next_offset == 0)
        return false;

    filename = basename(kld_name.c_str());

    for (addr_t kaddr = ReadPointer (m_linker_files_addr); kaddr != 0;
         kaddr = ReadPointer (kaddr + m_next_offset)) {

        ReadMemory (ReadPointer(kaddr + m_filename_offset),
                    kld_filename, PATH_MAX, error);

        if (strcmp (kld_filename, filename)) {
            continue;
        }
        kld_addr = ReadPointer (kaddr + m_address_offset);
        if (kld_addr == 0) {
            return false;
        } else {
            return true;
        }
    }
    return false;
}

size_t DynamicLoaderFreeBSDKernel::ReadMemory(addr_t addr, void *buf,
                                              size_t size, Error &error)
{
    return m_kernel_process->ReadMemory(addr, buf, size, error);
}
