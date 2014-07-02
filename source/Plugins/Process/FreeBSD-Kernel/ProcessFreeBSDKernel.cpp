//===-- ProcessFreeBSDKernel.cpp --------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// C Includes
#include <fcntl.h>
#include <stdlib.h>

// C++ Includes
// Other libraries and framework includes
#include "lldb/Core/Debugger.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/ModuleSpec.h"
#include "lldb/Core/State.h"
#include "lldb/Core/UUID.h"
#include "lldb/Host/Host.h"
#include "lldb/Host/Symbols.h"
#include "lldb/Interpreter/CommandInterpreter.h"
#include "lldb/Interpreter/CommandObject.h"
#include "lldb/Interpreter/CommandObjectMultiword.h"
#include "lldb/Interpreter/CommandReturnObject.h"
#include "lldb/Interpreter/OptionGroupString.h"
#include "lldb/Interpreter/OptionGroupUInt64.h"
#include "lldb/Symbol/ObjectFile.h"
#include "lldb/Symbol/SymbolVendor.h"
#include "lldb/Target/RegisterContext.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/Thread.h"

// Project includes
#include "ProcessFreeBSDKernel.h"
#include "ThreadFreeBSDKernel.h"
#include "ProcessPOSIXLog.h"
#include "Utility/StringExtractor.h"

using namespace lldb;
using namespace lldb_private;

namespace {
    enum {
        TDS_INACTIVE = 0x0,
        TDS_INHIBITED,
        TDS_CAN_RUN,
        TDS_RUNQ,
        TDS_RUNNING
    };

    static int pcb_size = 320;

    static PropertyDefinition
    g_properties[] =
    {
        {NULL, OptionValue::eTypeInvalid, false, 0, NULL, NULL, NULL}
    };
    class PluginProperties : public Properties
    {
    public:

        static ConstString
        GetSettingName ()
        {
            return ProcessFreeBSDKernel::GetPluginNameStatic();
        }

        PluginProperties() :
            Properties ()
        {
            m_collection_sp.reset (new OptionValueProperties(GetSettingName()));
            m_collection_sp->Initialize(g_properties);
        }

        virtual
        ~PluginProperties()
        {
        }
    };

    typedef std::shared_ptr<PluginProperties> ProcessFreeBSDKernelPropertiesSP;

    static const ProcessFreeBSDKernelPropertiesSP &
    GetGlobalPluginProperties()
    {
        static ProcessFreeBSDKernelPropertiesSP g_settings_sp;
        if (!g_settings_sp)
            g_settings_sp.reset (new PluginProperties ());
        return g_settings_sp;
    }

} // anonymous namespace end

ConstString
ProcessFreeBSDKernel::GetPluginNameStatic()
{
    static ConstString g_name("FreeBSD-Kernel");
    return g_name;
}

const char *
ProcessFreeBSDKernel::GetPluginDescriptionStatic()
{
    return "Plug-in for FreeBSD kernel debugging.";
}

void
ProcessFreeBSDKernel::Terminate()
{
    PluginManager::UnregisterPlugin (ProcessFreeBSDKernel::CreateInstance);
}


lldb::ProcessSP
ProcessFreeBSDKernel::CreateInstance (Target &target,
                            Listener &listener,
                            const FileSpec *crash_file_path)
{
    lldb::ProcessSP process_sp;
    // Check if the target is potentially a FreeBSD kernel image
    lldb_private::SymbolVendor *sym_vendor =
        target.GetExecutableModule()->GetSymbolVendor ();
    if (sym_vendor)
    {
        lldb_private::Symtab *symtab = sym_vendor->GetSymtab();
        if (symtab)
        {
            std::vector<uint32_t> match_indexes;
            ConstString symbol_name ("kern_open");
            uint32_t num_matches = 0;

            num_matches = symtab->AppendSymbolIndexesWithName (symbol_name,
                                                               match_indexes);

            if (num_matches == 0)
            {
                return process_sp;
            }
        }
    }
    process_sp.reset(new ProcessFreeBSDKernel (target, listener, *crash_file_path));
    return process_sp;
}

bool
ProcessFreeBSDKernel::CanDebug(Target &target, bool plugin_specified_by_name)
{
    if (plugin_specified_by_name)
        return true;

    // For now we are just making sure the file exists for a given module
    Module *exe_module = target.GetExecutableModulePointer();
    if (exe_module)
    {
        const llvm::Triple &triple_ref = target.GetArchitecture().GetTriple();
        switch (triple_ref.getOS())
        {
        case llvm::Triple::FreeBSD:
            {
            ObjectFile *exe_objfile = exe_module->GetObjectFile();
            if (exe_objfile->GetType() == ObjectFile::eTypeExecutable) // &&
                // exe_objfile->GetStrata() == ObjectFile::eStrataKernel)
            {
                return true;
            }
            break;
            }
        default:
            break;
        }
    }
    return false;
}

//----------------------------------------------------------------------
// ProcessFreeBSDKernel constructor
//----------------------------------------------------------------------
ProcessFreeBSDKernel::ProcessFreeBSDKernel(Target& target, Listener &listener,
                                           const FileSpec &crash_file_path) :
    Process (target, listener),
    m_core_file_name (crash_file_path.GetPath().c_str()),
    m_dyld_plugin_name (),
    m_kernel_image_file_name (target.GetExecutableModule()->GetFileSpec().GetPath().c_str()),
    m_core_file (crash_file_path),
    m_kernel_load_addr (LLDB_INVALID_ADDRESS),
    m_kvm(nullptr)
{
}

//----------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------
ProcessFreeBSDKernel::~ProcessFreeBSDKernel()
{
    Clear();
    // We need to call finalize on the process before destroying ourselves
    // to make sure all of the broadcaster cleanup goes as planned. If we
    // destruct this class, then Process::~Process() might have problems
    // trying to fully destroy the broadcaster.
    Finalize();
}

//----------------------------------------------------------------------
// PluginInterface
//----------------------------------------------------------------------
lldb_private::ConstString
ProcessFreeBSDKernel::GetPluginName()
{
    return GetPluginNameStatic();
}

uint32_t
ProcessFreeBSDKernel::GetPluginVersion()
{
    return 1;
}

Error
ProcessFreeBSDKernel::WillLaunch (Module* module)
{
    Error error;
    error.SetErrorString ("launching not supported in freebsd-kernel plug-in");
    return error;
}

Error
ProcessFreeBSDKernel::WillAttachToProcessWithID (lldb::pid_t pid)
{
    Error error;
    error.SetErrorString ("attaching to a by process ID not supported in FreeBSD-Kernel plug-in");
    return error;
}

Error
ProcessFreeBSDKernel::WillAttachToProcessWithName (const char *process_name, bool wait_for_launch)
{
    Error error;
    error.SetErrorString ("attaching to a by process name not supported in FreeBSD-Kernel plug-in");
    return error;
}

//----------------------------------------------------------------------
// Process Control
//----------------------------------------------------------------------
Error
ProcessFreeBSDKernel::DoLoadCore ()
{
    Log *log (ProcessPOSIXLog::GetLogIfAllCategoriesSet (POSIX_LOG_PROCESS));
    Error error;
    char kvm_err[_POSIX2_LINE_MAX];
    if (m_core_file.Exists()) {
        m_kvm = kvm_openfiles(m_kernel_image_file_name.AsCString(),
                              m_core_file_name.AsCString(), nullptr,
                              O_RDONLY, kvm_err);
        if (m_kvm == nullptr && log)
        {
            log->Printf ("ProcessFreeBSDKernel::DoLoadCore() error %s", kvm_err);
            error.SetErrorString("Open core file failed in FreeBSD Kernel");
        } else {
            InitializeThreads();
        }
    }
    return error;
}

Error
ProcessFreeBSDKernel::DoLaunch (Module *exe_module,
                      ProcessLaunchInfo &launch_info)
{
    Error error;
    error.SetErrorString ("launching not supported in FreeBSD-kernel plug-in");
    return error;
}


Error
ProcessFreeBSDKernel::DoAttachToProcessWithID (lldb::pid_t attach_pid)
{
    Error error;
    error.SetErrorString ("attach to process by ID is not suppported in FreeBSD kernel debugging");
    return error;
}

Error
ProcessFreeBSDKernel::DoAttachToProcessWithID (lldb::pid_t attach_pid, const ProcessAttachInfo &attach_info)
{
    return DoAttachToProcessWithID(attach_pid);
}

Error
ProcessFreeBSDKernel::DoAttachToProcessWithName (const char *process_name, const ProcessAttachInfo &attach_info)
{
    Error error;
    error.SetErrorString ("attach to process by name is not suppported in freebsd kernel debugging");
    return error;
}


void
ProcessFreeBSDKernel::DidAttach ()
{
    Log *log (ProcessPOSIXLog::GetLogIfAllCategoriesSet (POSIX_LOG_PROCESS));
    if (log)
        log->Printf ("ProcessFreeBSDKernel::DidAttach()");
    if (GetID() != LLDB_INVALID_PROCESS_ID)
    {
        // TODO: figure out the register context that we will use
    }
}

addr_t
ProcessFreeBSDKernel::GetImageInfoAddress()
{
    Target *target = &GetTarget();
    ObjectFile *obj_file = target->GetExecutableModule()->GetObjectFile();
    Address addr = obj_file->GetImageInfoAddress(target);

    if (addr.IsValid())
        return addr.GetLoadAddress(target);
    return LLDB_INVALID_ADDRESS;
}

lldb_private::DynamicLoader *
ProcessFreeBSDKernel::GetDynamicLoader ()
{
    return m_dyld_ap.get();
}

Error
ProcessFreeBSDKernel::WillResume ()
{
    return Error();
}

Error
ProcessFreeBSDKernel::DoResume ()
{
    Error error;
    // Log *log (ProcessPOSIXLog::GetLogIfAllCategoriesSet (POSIX_LOG_PROCESS));
    // Only start the async thread if we try to do any process control

    return error;
}

ThreadFreeBSDKernel *
ProcessFreeBSDKernel::CreateNewThreadFreeBSDKernel (lldb_private::Process &process,
                                                   lldb::tid_t tid)
{
    return new ThreadFreeBSDKernel(process, tid);
}

bool
ProcessFreeBSDKernel::UpdateThreadList (ThreadList &old_thread_list,
                                        ThreadList &new_thread_list)
{
    Log *log (ProcessPOSIXLog::GetLogIfAllCategoriesSet (POSIX_LOG_THREAD));
    if (log && log->GetMask().Test(POSIX_LOG_VERBOSE))
        log->Printf ("ProcessFreeBSDKernel::%s (pid = %" PRIu64 ")", __FUNCTION__, GetID());


    for (size_t i = 0; i < m_kthreads.size(); i++)
    {
        new_thread_list.AddThread(m_kthreads[i]);
    }
    return new_thread_list.GetSize(false) > 0;
}

void
ProcessFreeBSDKernel::RefreshStateAfterStop ()
{
    // Let all threads recover from stopping and do any clean up based
    // on the previous thread state (if any).
    //  m_thread_list.RefreshStateAfterStop();
}

Error
ProcessFreeBSDKernel::DoHalt (bool &caused_stop)
{
    Error error;
    return error;
}

Error
ProcessFreeBSDKernel::DoDetach(bool keep_stopped)
{
    Error error;
    Log *log (ProcessPOSIXLog::GetLogIfAllCategoriesSet(POSIX_LOG_PROCESS));
    if (log)
        log->Printf ("ProcessFreeBSDKernel::DoDetach(keep_stopped = %i)", keep_stopped);

    return error;
}

Error
ProcessFreeBSDKernel::DoDestroy ()
{
    bool keep_stopped = false;
    return DoDetach(keep_stopped);
}

//------------------------------------------------------------------
// Process Queries
//------------------------------------------------------------------

bool
ProcessFreeBSDKernel::IsAlive ()
{
    return (m_kvm  != nullptr);
}

//------------------------------------------------------------------
// Process Memory
//------------------------------------------------------------------
size_t
ProcessFreeBSDKernel::ReadMemory (lldb::addr_t addr, void *buf, size_t size, Error &error)
{
    return DoReadMemory (addr, buf, size, error);
}

size_t
ProcessFreeBSDKernel::DoReadMemory (addr_t addr, void *buf, size_t size, Error &error)
{
    if (!m_kvm) {
        return 0;
    } else {
        return kvm_read(m_kvm, addr, buf, size);
    }
}

size_t
ProcessFreeBSDKernel::WriteMemory (addr_t addr, const void *buf, size_t size, Error &error)
{
    return DoWriteMemory(addr, buf, size, error);
}

size_t
ProcessFreeBSDKernel::DoWriteMemory (addr_t addr, const void *buf, size_t size, Error &error)
{
    if (!m_kvm) {
        return 0;
    } else {
        return kvm_write(m_kvm, addr, buf, size);
    }
}

lldb::addr_t
ProcessFreeBSDKernel::DoAllocateMemory (size_t size, uint32_t permissions, Error &error)
{
    error.SetErrorString ("memory allocation not suppported in freebsd kernel debugging");
    return LLDB_INVALID_ADDRESS;
}

Error
ProcessFreeBSDKernel::DoDeallocateMemory (lldb::addr_t addr)
{
    Error error;
    error.SetErrorString ("memory deallocation not suppported in freebsd kernel debugging");
    return error;
}

Error
ProcessFreeBSDKernel::EnableBreakpointSite (BreakpointSite *bp_site)
{
    return EnableSoftwareBreakpoint (bp_site);
}

Error
ProcessFreeBSDKernel::DisableBreakpointSite (BreakpointSite *bp_site)
{
    return DisableSoftwareBreakpoint (bp_site);
}

Error
ProcessFreeBSDKernel::EnableWatchpoint (Watchpoint *wp, bool notify)
{
    Error error;
    error.SetErrorString ("watchpoints are not suppported in freebsd kernel debugging");
    return error;
}

Error
ProcessFreeBSDKernel::DisableWatchpoint (Watchpoint *wp, bool notify)
{
    Error error;
    error.SetErrorString ("watchpoints are not suppported in freebsd kernel debugging");
    return error;
}

void
ProcessFreeBSDKernel::Clear()
{
    if (m_kvm) {
        kvm_close(m_kvm);
        m_kvm = nullptr;
    }
    m_thread_list.Clear();
}

Error
ProcessFreeBSDKernel::DoSignal (int signo)
{
    Error error;
    error.SetErrorString ("sending signals is not suppported in kdp remote debugging");
    return error;
}

void
ProcessFreeBSDKernel::Initialize()
{
    static bool g_initialized = false;

    if (g_initialized == false)
    {
        g_initialized = true;
        PluginManager::RegisterPlugin (GetPluginNameStatic(),
                                       GetPluginDescriptionStatic(),
                                       CreateInstance,
                                       DebuggerInitialize);

        Log::Callbacks log_callbacks = {
            ProcessPOSIXLog::DisableLog,
            ProcessPOSIXLog::EnableLog,
            ProcessPOSIXLog::ListLogCategories
        };

        Log::RegisterLogChannel (ProcessFreeBSDKernel::GetPluginNameStatic(), log_callbacks);
    }
}

void
ProcessFreeBSDKernel::DebuggerInitialize (lldb_private::Debugger &debugger)
{
    if (!PluginManager::GetSettingForProcessPlugin(debugger, PluginProperties::GetSettingName()))
    {
        const bool is_global_setting = true;
        PluginManager::CreateSettingForProcessPlugin (debugger,
                                                      GetGlobalPluginProperties()->GetValueProperties(),
                                                      ConstString ("Properties for the FreeBSD kernel process plug-in."),
                                                      is_global_setting);
    }
}

lldb::addr_t ProcessFreeBSDKernel::LookUpSymbolAddressInModule(lldb::ModuleSP module,
                                                               const char *name)
{
    lldb_private::SymbolVendor *sym_vendor = module->GetSymbolVendor ();
    if (sym_vendor)
    {
        lldb_private::Symtab *symtab = sym_vendor->GetSymtab();
        if (symtab)
        {
            std::vector<uint32_t> match_indexes;
            ConstString symbol_name (name);
            uint32_t num_matches = 0;

            num_matches = symtab->AppendSymbolIndexesWithName (symbol_name,
                                                               match_indexes);

            if (num_matches > 0)
            {

                Symbol *symbol = symtab->SymbolAtIndex(match_indexes[0]);
                return symbol->GetAddress().GetFileAddress();
            }
        }
    }
    return 0;
}

bool ProcessFreeBSDKernel::InitializeThreads()
{
    ModuleSP module = GetTarget().GetExecutableModule();
    lldb::addr_t addr, paddr;

    addr = LookUpSymbolAddressInModule(module, "allproc");
    if (addr == 0)
        return false;
    kvm_read(m_kvm, addr, &paddr, sizeof(paddr));

    m_dumppcb = LookUpSymbolAddressInModule(module, "dumppcb");
    if (m_dumppcb == 0)
        return false;

    addr = LookUpSymbolAddressInModule(module, "dumptid");
    if (addr == 0)
        m_dumptid = -1;
    else
        kvm_read(m_kvm, addr, &m_dumptid, sizeof(m_dumptid));

    addr = LookUpSymbolAddressInModule(module, "stopped_cpus");
    CPU_ZERO(&m_stopped_cpus);
    m_cpusetsize = sysconf(_SC_CPUSET_SIZE);
    if (m_cpusetsize != -1 && (unsigned long)m_cpusetsize <= sizeof(cpuset_t) &&
        addr != 0)
        kvm_read(m_kvm, addr, &m_stopped_cpus, m_cpusetsize);

    AddProcs(paddr);
    addr = LookUpSymbolAddressInModule(module, "zombproc");
    if (addr != 0)
    {
        kvm_read(m_kvm, addr, &paddr, sizeof(paddr));
        AddProcs(paddr);
    }
    // curkthr = kgdb_thr_lookup_tid(dumptid);
    // if (curkthr == NULL)
    //     curkthr = first;
    return true;
}

void
ProcessFreeBSDKernel::AddProcs(uintptr_t paddr)
{
    Log *log (ProcessPOSIXLog::GetLogIfAllCategoriesSet(POSIX_LOG_PROCESS));
    struct proc p;
    struct thread td;
    addr_t addr;

    while (paddr != 0) {
        if (kvm_read(m_kvm, paddr, &p, sizeof(p)) != sizeof(p)) {
            if (log)
                log->Printf("kvm_read: %s", kvm_geterr(m_kvm));
            break;
        }
        addr = (addr_t)TAILQ_FIRST(&p.p_threads);
        while (addr != 0) {
            if (kvm_read(m_kvm, addr, &td, sizeof(td)) !=
                sizeof(td)) {
                if (log)
                    log->Printf("kvm_read: %s", kvm_geterr(m_kvm));
                break;
            }

            ThreadSP thread_sp;
            ThreadFreeBSDKernel * kthread =
                CreateNewThreadFreeBSDKernel(*this, td.td_tid);

            kthread->m_kaddr = addr;
            if ((lldb::tid_t)td.td_tid == m_dumptid)
                kthread->m_pcb = m_dumppcb;
            else if (TD_IS_RUNNING(&td) &&
                     CPU_ISSET(td.td_oncpu, &m_stopped_cpus))
                kthread->m_pcb = FindCorePCB(td.td_oncpu);
            else
                kthread->m_pcb = (addr_t)td.td_pcb;
            kthread->m_kstack = td.td_kstack;
            kthread->m_pid = p.p_pid;
            kthread->m_tid = td.td_tid;
            kthread->m_paddr = paddr;
            kthread->m_cpu = td.td_oncpu;
            thread_sp.reset(kthread);
            m_kthreads.push_back(thread_sp);
            addr = (addr_t)TAILQ_NEXT(&td, td_plist);
        }
        paddr = (addr_t)LIST_NEXT(&p, p_list);
    }
}

addr_t ProcessFreeBSDKernel::FindCorePCB(uint32_t cpuid)
{
    ModuleSP module = GetTarget().GetExecutableModule();
    if (m_stoppcbs == 0) {
        m_stoppcbs = LookUpSymbolAddressInModule(module, "stoppcbs");
    }

    if (m_stoppcbs == 0)
        return 0;

    return m_stoppcbs + cpuid * pcb_size;
}
