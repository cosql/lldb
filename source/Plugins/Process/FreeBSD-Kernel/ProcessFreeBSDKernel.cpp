//===-- ProcessKDP.cpp ------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// C Includes
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
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
#include "lldb/Target/RegisterContext.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/Thread.h"

// Project includes
#include "ProcessFreeBSDKernel.h"
#include "ProcessPOSIXLog.h"
#include "Utility/StringExtractor.h"

using namespace lldb;
using namespace lldb_private;

namespace {
    static PropertyDefinition
    g_properties[] =
    {
        {  NULL            , OptionValue::eTypeInvalid, false, 0, NULL, NULL, NULL  }
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

static const lldb::tid_t g_kernel_tid = 1;

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
                 if ((llvm::Triple::OSType)triple_ref.getVendor() == llvm::Triple::FreeBSD)
                {
                    ObjectFile *exe_objfile = exe_module->GetObjectFile();
                    if (exe_objfile->GetType() == ObjectFile::eTypeExecutable &&
                        exe_objfile->GetStrata() == ObjectFile::eStrataKernel)
                        return true;
                }
                break;

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
    m_dyld_plugin_name (),
    m_core_file (crash_file_path),
    m_kernel_load_addr (LLDB_INVALID_ADDRESS),
    m_command_sp(),
    m_kernel_thread_wp()
{
    m_kernel_image_file_name =
        target.GetExecutableModule()->GetFileSpec().GetFilename();
    m_core_file_name = crash_file_path.GetFilename();
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
    Error error;
    char kvm_err[_POSIX2_LINE_MAX];
    if (m_core_file.Exists()) {
        m_kvm = kvm_openfiles(m_kernel_image_file_name.AsCString(),
                              m_core_file_name.AsCString() , nullptr,
                              O_RDONLY, kvm_err);
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
    if (m_dyld_ap.get() == NULL)
        m_dyld_ap.reset (DynamicLoader::FindPlugin(this, m_dyld_plugin_name.IsEmpty() ? NULL : m_dyld_plugin_name.AsCString()));
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

lldb::ThreadSP
ProcessFreeBSDKernel::GetKernelThread()
{
    // KDP only tells us about one thread/core. Any other threads will usually
    // be the ones that are read from memory by the OS plug-ins.

    ThreadSP thread_sp (m_kernel_thread_wp.lock());
    if (!thread_sp)
    {
    }
    return thread_sp;
}

bool
ProcessFreeBSDKernel::UpdateThreadList (ThreadList &old_thread_list, ThreadList &new_thread_list)
{
    // locker will keep a mutex locked until it goes out of scope
    Log *log (ProcessPOSIXLog::GetLogIfAllCategoriesSet (POSIX_LOG_THREAD));
    if (log && log->GetMask().Test(POSIX_LOG_VERBOSE))
        log->Printf ("ProcessFreeBSDKernel::%s (pid = %" PRIu64 ")", __FUNCTION__, GetID());

    // indivudually, there is really only one. Lets call this thread 1.
    ThreadSP thread_sp (old_thread_list.FindThreadByProtocolID(g_kernel_tid, false));
    if (!thread_sp)
        thread_sp = GetKernelThread ();
    new_thread_list.AddThread(thread_sp);

    return new_thread_list.GetSize(false) > 0;
}

void
ProcessFreeBSDKernel::RefreshStateAfterStop ()
{
    // Let all threads recover from stopping and do any clean up based
    // on the previous thread state (if any).
    m_thread_list.RefreshStateAfterStop();
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

    //KillDebugserverProcess ();
    return error;
}

Error
ProcessFreeBSDKernel::DoDestroy ()
{
    // For KDP there really is no difference between destroy and detach
    bool keep_stopped = false;
    return DoDetach(keep_stopped);
}

//------------------------------------------------------------------
// Process Queries
//------------------------------------------------------------------

bool
ProcessFreeBSDKernel::IsAlive ()
{
    return (m_kvm  != NULL);
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

class CommandObjectProcessFreeBSDKernelPacketSend : public CommandObjectParsed
{
private:

    OptionGroupOptions m_option_group;
    OptionGroupUInt64 m_command_byte;
    OptionGroupString m_packet_data;

    virtual Options *
    GetOptions ()
    {
        return &m_option_group;
    }


public:
    CommandObjectProcessFreeBSDKernelPacketSend(CommandInterpreter &interpreter) :
        CommandObjectParsed (interpreter,
                             "process plugin packet send",
                             "Send a custom packet through the KDP protocol by specifying the command byte and the packet payload data. A packet will be sent with a correct header and payload, and the raw result bytes will be displayed as a string value. ",
                             NULL),
        m_option_group (interpreter),
        m_command_byte(LLDB_OPT_SET_1, true , "command", 'c', 0, eArgTypeNone, "Specify the command byte to use when sending the KDP request packet.", 0),
        m_packet_data (LLDB_OPT_SET_1, false, "payload", 'p', 0, eArgTypeNone, "Specify packet payload bytes as a hex ASCII string with no spaces or hex prefixes.", NULL)
    {
        m_option_group.Append (&m_command_byte, LLDB_OPT_SET_ALL, LLDB_OPT_SET_1);
        m_option_group.Append (&m_packet_data , LLDB_OPT_SET_ALL, LLDB_OPT_SET_1);
        m_option_group.Finalize();
    }

    ~CommandObjectProcessFreeBSDKernelPacketSend ()
    {
    }

    bool
    DoExecute (Args& command, CommandReturnObject &result)
    {
        const size_t argc = command.GetArgumentCount();
        if (argc == 0)
        {
        }
        else
        {
            result.AppendErrorWithFormat ("'%s' takes no arguments, only options.", m_cmd_name.c_str());
            result.SetStatus (eReturnStatusFailed);
        }
        return false;
    }
};

class CommandObjectProcessFreeBSDKernelPacket : public CommandObjectMultiword
{
private:

public:
    CommandObjectProcessFreeBSDKernelPacket(CommandInterpreter &interpreter) :
    CommandObjectMultiword (interpreter,
                            "process plugin packet",
                            "Commands that deal with KDP remote packets.",
                            NULL)
    {
        LoadSubCommand ("send", CommandObjectSP (new CommandObjectProcessFreeBSDKernelPacketSend (interpreter)));
    }

    ~CommandObjectProcessFreeBSDKernelPacket ()
    {
    }
};

class CommandObjectMultiwordProcessFreeBSDKernel : public CommandObjectMultiword
{
public:
    CommandObjectMultiwordProcessFreeBSDKernel (CommandInterpreter &interpreter) :
    CommandObjectMultiword (interpreter,
                            "process plugin",
                            "A set of commands for operating on a ProcessFreeBSDKernel process.",
                            "process plugin <subcommand> [<subcommand-options>]")
    {
        LoadSubCommand ("packet", CommandObjectSP (new CommandObjectProcessFreeBSDKernelPacket    (interpreter)));
    }

    ~CommandObjectMultiwordProcessFreeBSDKernel ()
    {
    }
};

CommandObject *
ProcessFreeBSDKernel::GetPluginCommandObject()
{
    if (!m_command_sp)
        m_command_sp.reset (new CommandObjectMultiwordProcessFreeBSDKernel (GetTarget().GetDebugger().GetCommandInterpreter()));
    return m_command_sp.get();
}
