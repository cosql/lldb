//===-- DynamicLoaderFreeBSDKernel.h ----------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_DynamicLoaderFreeBSDKernel_H_
#define liblldb_DynamicLoaderFreeBSDKernel_H_

// C Includes
// C++ Includes
// Other libraries and framework includes
#include "lldb/Breakpoint/StoppointCallbackContext.h"
#include "lldb/Target/DynamicLoader.h"
#include "Plugins/Process/FreeBSD-Kernel/ProcessFreeBSDKernel.h"

class DynamicLoaderFreeBSDKernel : public lldb_private::DynamicLoader
{
public:

    static void
    Initialize();

    static void
    Terminate();

    static lldb_private::ConstString
    GetPluginNameStatic();

    static const char *
    GetPluginDescriptionStatic();

    static lldb_private::DynamicLoader *
    CreateInstance(lldb_private::Process *process, bool force);

    DynamicLoaderFreeBSDKernel(lldb_private::Process *process);

    virtual
    ~DynamicLoaderFreeBSDKernel();

    //------------------------------------------------------------------
    // DynamicLoader protocol
    //------------------------------------------------------------------

    virtual void
    DidAttach();

    virtual void
    DidLaunch();

    virtual lldb::ThreadPlanSP
    GetStepThroughTrampolinePlan(lldb_private::Thread &thread,
                                 bool stop_others);

    virtual lldb_private::Error
    CanLoadImage();

    //------------------------------------------------------------------
    // PluginInterface protocol
    //------------------------------------------------------------------
    virtual lldb_private::ConstString
    GetPluginName();

    virtual uint32_t
    GetPluginVersion();

    virtual void
    GetPluginCommandHelp(const char *command, lldb_private::Stream *strm);

    virtual lldb_private::Error
    ExecutePluginCommand(lldb_private::Args &command, lldb_private::Stream *strm);

    virtual lldb_private::Log *
    EnablePluginLogging(lldb_private::Stream *strm, lldb_private::Args &command);

    void RefreshModules();

    void LoadAllCurrentModules();

    size_t ReadMemory(lldb::addr_t addr, void *buf, size_t size, lldb_private::Error &error);
protected:
    lldb::addr_t m_kernel_load_addr, m_linker_files_addr, m_kernel_file_addr;
    lldb::addr_t m_address_offset, m_filename_offset;
    lldb::addr_t m_next_offset, m_pathname_offset;
    std::vector<std::string> m_module_paths;
    ProcessFreeBSDKernel* m_kernel_process;
private:
    bool
    IsKLDOK (std::string path);

    bool
    CheckKLDPath (std::string path);

    bool FindKLDPath (std::string filename, std::string& path);

    bool FindKLDAddress(std::string kld_name, lldb::addr_t& kld_addr);

    void InitLoadSpecs();

    DISALLOW_COPY_AND_ASSIGN(DynamicLoaderFreeBSDKernel);
};

#endif  // liblldb_DynamicLoaderFreeBSDKernel_H_
