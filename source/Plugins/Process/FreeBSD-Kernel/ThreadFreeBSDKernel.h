//===-- ThreadFreeBSD.h ----------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_ThreadFreeBSDKernel_h_
#define liblldb_ThreadFreeBSDKernel_h_

#include <string>

#include "lldb/Target/Thread.h"

class ThreadFreeBSDKernel : public lldb_private::Thread
{
public:
	ThreadFreeBSDKernel (lldb_private::Process &process, lldb::tid_t tid);

    virtual
	~ThreadFreeBSDKernel ();

    virtual void
	RefreshStateAfterStop();

    virtual lldb::RegisterContextSP
	GetRegisterContext ();

    virtual lldb::RegisterContextSP
	CreateRegisterContextForFrame (lldb_private::StackFrame *frame);

    static bool
	ThreadIDIsValid (lldb::tid_t thread)
    {
	    return thread != 0;
    }

    virtual const char *
	GetName ()
    {
	    if (m_thread_name.empty())
		    return NULL;
	    return m_thread_name.c_str();
    }

    void
	SetName (const char *name)
    {
	    if (name && name[0])
		    m_thread_name.assign (name);
	    else
		    m_thread_name.clear();
    }

    const char *
    GetQueueName ()
    {
        return NULL;
    }

    void
    Dump (lldb_private::Log *log, uint32_t index);

    bool
    ShouldStop (bool &step_more);

    lldb::addr_t GetPCB()
    {
        return m_pcb;
    }
protected:
    friend class ProcessFreeBSDKernel;
    //------------------------------------------------------------------
    // Member variables.
    //------------------------------------------------------------------
    std::string m_thread_name;
    lldb::RegisterContextSP m_thread_reg_ctx_sp;
    lldb::addr_t    m_paddr;
    lldb::addr_t    m_kaddr;
    lldb::addr_t    m_kstack;
    lldb::addr_t    m_pcb;
    lldb::tid_t     m_tid;
    lldb::pid_t     m_pid;
    unsigned char   m_cpu;
    int m_signo;
    lldb::StopInfoSP m_cached_stop_info_sp;
    
    virtual bool CalculateStopInfo();

};

#endif // liblldb_ThreadFreeBSDKernel_h_
