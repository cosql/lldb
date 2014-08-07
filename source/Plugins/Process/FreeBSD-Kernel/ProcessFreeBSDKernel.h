//===-- ProcessFreeBSDKernel.h ----------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_ProcessFreeBSDKernel_H_
#define liblldb_ProcessFreeBSDKernel_H_

// C Includes
#include <kvm.h>
#include <sys/proc.h>
#include <sys/cpuset.h>

// C++ Includes

// Other libraries and framework includes
#include "ThreadFreeBSDKernel.h"

#include "lldb/Core/ArchSpec.h"
#include "lldb/Core/Broadcaster.h"
#include "lldb/Core/ConstString.h"
#include "lldb/Core/Error.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/StreamString.h"
#include "lldb/Core/StringList.h"
#include "lldb/Core/ThreadSafeValue.h"
#include "lldb/Target/DynamicLoader.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/Thread.h"

class ProcessMonitor;
class ThreadFreeBSDKernel;

class ProcessFreeBSDKernel :
    public lldb_private::Process
{
public:
    //------------------------------------------------------------------
    // Constructors and Destructors
    //------------------------------------------------------------------
    static lldb::ProcessSP
    CreateInstance (lldb_private::Target& target,
                    lldb_private::Listener &listener,
                    const lldb_private::FileSpec *crash_file_path);

    static void
    Initialize();

    static void
    DebuggerInitialize (lldb_private::Debugger &debugger);

    static void
    Terminate();

    static lldb_private::ConstString
    GetPluginNameStatic();

    static const char *
    GetPluginDescriptionStatic();

    //------------------------------------------------------------------
    // Constructors and Destructors
    //------------------------------------------------------------------
    ProcessFreeBSDKernel(lldb_private::Target& target,
                         lldb_private::Listener &listener,
                         const lldb_private::FileSpec &crash_file_path);

    virtual
    ~ProcessFreeBSDKernel();

    //------------------------------------------------------------------
    // Check if a given Process
    //------------------------------------------------------------------
    virtual bool
    CanDebug (lldb_private::Target &target,
              bool plugin_specified_by_name);

    //------------------------------------------------------------------
    // Creating a new process, or attaching to an existing one
    //------------------------------------------------------------------
    virtual lldb_private::Error
    DoLoadCore ();

    virtual lldb_private::DynamicLoader *
    GetDynamicLoader ();

    virtual lldb_private::Error
    WillLaunch (lldb_private::Module* module);

    virtual lldb_private::Error
    DoLaunch (lldb_private::Module *exe_module,
              lldb_private::ProcessLaunchInfo &launch_info);

    virtual lldb_private::Error
    WillAttachToProcessWithID (lldb::pid_t pid);

    virtual lldb_private::Error
    WillAttachToProcessWithName (const char *process_name, bool wait_for_launch);

    virtual lldb_private::Error
    DoAttachToProcessWithID (lldb::pid_t pid);

    virtual lldb_private::Error
    DoAttachToProcessWithID (lldb::pid_t pid, const lldb_private::ProcessAttachInfo &attach_info);

    virtual lldb_private::Error
    DoAttachToProcessWithName (const char *process_name, const lldb_private::ProcessAttachInfo &attach_info);

    virtual void
    DidAttach (lldb_private::ArchSpec &process_arch);

    lldb::addr_t
    GetImageInfoAddress();

    //------------------------------------------------------------------
    // PluginInterface protocol
    //------------------------------------------------------------------
    virtual lldb_private::ConstString
    GetPluginName();

    virtual uint32_t
    GetPluginVersion();

    //------------------------------------------------------------------
    // Process Control
    //------------------------------------------------------------------
    virtual lldb_private::Error
    WillResume ();

    virtual lldb_private::Error
    DoResume ();

    virtual lldb_private::Error
    DoHalt (bool &caused_stop);

    virtual lldb_private::Error
    DoDetach (bool keep_stopped);

    virtual lldb_private::Error
    DoSignal (int signal);

    virtual lldb_private::Error
    DoDestroy ();

    virtual void
    RefreshStateAfterStop();

    //------------------------------------------------------------------
    // Process Queries
    //------------------------------------------------------------------
    virtual bool
    IsAlive ();

    //------------------------------------------------------------------
    // Process Memory
    //------------------------------------------------------------------
    virtual size_t
    ReadMemory (lldb::addr_t addr, void *buf, size_t size, lldb_private::Error &error);

    virtual size_t
    DoReadMemory (lldb::addr_t addr, void *buf, size_t size, lldb_private::Error &error);

    virtual size_t
    WriteMemory (lldb::addr_t addr, const void *buf, size_t size, lldb_private::Error &error);

    virtual size_t
    DoWriteMemory (lldb::addr_t addr, const void *buf, size_t size, lldb_private::Error &error);

    virtual lldb::addr_t
    DoAllocateMemory (size_t size, uint32_t permissions, lldb_private::Error &error);

    virtual lldb_private::Error
    DoDeallocateMemory (lldb::addr_t ptr);

    //----------------------------------------------------------------------
    // Process Breakpoints
    //----------------------------------------------------------------------
    virtual lldb_private::Error
    EnableBreakpointSite (lldb_private::BreakpointSite *bp_site);

    virtual lldb_private::Error
    DisableBreakpointSite (lldb_private::BreakpointSite *bp_site);

    //----------------------------------------------------------------------
    // Process Watchpoints
    //----------------------------------------------------------------------
    virtual lldb_private::Error
    EnableWatchpoint (lldb_private::Watchpoint *wp, bool notify = true);

    virtual lldb_private::Error
    DisableWatchpoint (lldb_private::Watchpoint *wp, bool notify = true);

    lldb::addr_t
    LookUpSymbolAddressInModule(lldb::ModuleSP  module,
                                const char *sym_name);

    kvm_t * 
    GetKVM() {return m_kvm;}
protected:

    //----------------------------------------------------------------------
    // Accessors
    //----------------------------------------------------------------------
    bool
    IsRunning ( lldb::StateType state )
    {
        return    state == lldb::eStateRunning || IsStepping(state);
    }

    bool
    IsStepping ( lldb::StateType state)
    {
        return    state == lldb::eStateStepping;
    }

    bool
    CanResume ( lldb::StateType state)
    {
        return state == lldb::eStateStopped;
    }

    bool
    HasExited (lldb::StateType state)
    {
        return state == lldb::eStateExited;
    }

    bool
    ProcessIDIsValid ( ) const;

    void
    Clear ( );

    virtual bool
    UpdateThreadList (lldb_private::ThreadList &old_thread_list,
                      lldb_private::ThreadList &new_thread_list);

    ThreadFreeBSDKernel *
    CreateNewThreadFreeBSDKernel(lldb_private::Process &process,
                                 lldb::tid_t tid);

    private:
    //------------------------------------------------------------------
    // For ProcessFreeBSDKernel only
    //------------------------------------------------------------------
    lldb_private::ConstString m_core_file_name;
    lldb_private::ConstString m_dyld_plugin_name;
    lldb_private::ConstString m_kernel_image_file_name;
    lldb_private::FileSpec m_core_file;
    uint32_t m_dumptid;
    long m_cpusetsize;
    cpuset_t m_stopped_cpus;
    lldb::addr_t m_kernel_load_addr, m_dumppcb, m_stoppcbs;
    lldb::CommandObjectSP m_command_sp;
    lldb::ThreadWP m_kernel_thread_wp;
    kvm_t *m_kvm;
    std::vector<lldb::ThreadSP> m_kthreads;


    void AddProcs(uintptr_t paddr);

    lldb::addr_t FindCorePCB(uint32_t cpuid);

    bool InitializeThreads();

    DISALLOW_COPY_AND_ASSIGN (ProcessFreeBSDKernel);
};

#endif // liblldb_ProcessFreeBSDKernel_H_
