//===-- ThreadFreeBSD.cpp ---------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/Core/ArchSpec.h"
#include "lldb/Core/DataExtractor.h"
#include "lldb/Core/StreamString.h"
#include "lldb/Core/State.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/RegisterContext.h"
#include "lldb/Target/StopInfo.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/Unwind.h"
#include "lldb/Breakpoint/Watchpoint.h"

#include "ProcessFreeBSDKernel.h"
#include "ProcessPOSIXLog.h"
#include "RegisterContextFreeBSD_x86_64.h"
#include "RegisterContextFreeBSDKernel_x86_64.h"
#include "ThreadFreeBSDKernel.h"

using namespace lldb;
using namespace lldb_private;

//----------------------------------------------------------------------
// Thread Registers
//----------------------------------------------------------------------

ThreadFreeBSDKernel::ThreadFreeBSDKernel (Process &process,
                                          tid_t tid)
    : Thread(process, tid),
      m_thread_name ()
{
}

ThreadFreeBSDKernel::~ThreadFreeBSDKernel ()
{
    DestroyThread();
}

void
ThreadFreeBSDKernel::RefreshStateAfterStop()
{
    // Invalidate all registers in our register context. We don't set "force" to
    // true because the stop reply packet might have had some register values
    // that were expedited and these will already be copied into the register
    // context by the time this function gets called. The KDPRegisterContext
    // class has been made smart enough to detect when it needs to invalidate
    // which registers are valid by putting hooks in the register read and
    // register supply functions where they check the process stop ID and do
    // the right thing.
    const bool force = false;
    lldb::RegisterContextSP reg_ctx_sp (GetRegisterContext());
    if (reg_ctx_sp)
        reg_ctx_sp->InvalidateIfNeeded (force);
}

void
ThreadFreeBSDKernel::Dump(Log *log, uint32_t index)
{
}


bool
ThreadFreeBSDKernel::ShouldStop (bool &step_more)
{
    return true;
}

lldb::RegisterContextSP
ThreadFreeBSDKernel::GetRegisterContext ()
{
    if (!m_reg_context_sp)
    {
        m_reg_context_sp = CreateRegisterContextForFrame (NULL);
    }
    return m_reg_context_sp;
}

lldb::RegisterContextSP
ThreadFreeBSDKernel::CreateRegisterContextForFrame (StackFrame *frame)
{
    lldb::RegisterContextSP reg_ctx_sp;
    RegisterInfoInterface *reg_interface = NULL;
    uint32_t concrete_frame_idx = 0;

    if (frame) {
        concrete_frame_idx = frame->GetConcreteFrameIndex ();
    }
    const ArchSpec &target_arch = GetProcess()->GetTarget().GetArchitecture();

    if (concrete_frame_idx == 0)
    {
        ProcessSP process_sp (CalculateProcess());
        ProcessFreeBSDKernel * process =
                static_cast<ProcessFreeBSDKernel *>(process_sp.get());
        if (process)
        {
            switch (process->GetTarget().GetArchitecture().GetMachine())
            {
            case llvm::Triple::x86_64:
                {
                    reg_interface =  new RegisterContextFreeBSD_x86_64 (target_arch);
                    break;
                }
            default:
                assert (!"Add CPU type support in FreeBSD Kernel");
                break;
            }
        }
    }
    assert(reg_interface && "OS or CPU not supported!");
    switch (target_arch.GetMachine())
    {
        case llvm::Triple::x86:
        case llvm::Triple::x86_64:
            {
                RegisterContextFreeBSDKernel_x86_64 *reg_ctx =
                    new RegisterContextFreeBSDKernel_x86_64(*this, reg_interface);
                m_reg_context_sp.reset(reg_ctx);
                break;
            }
        default:
            break;
    }
    return m_reg_context_sp;
}

bool
ThreadFreeBSDKernel::CalculateStopInfo ()
{
    ProcessSP process_sp (GetProcess());
    if (process_sp)
    {
        if (m_cached_stop_info_sp)
        {
            SetStopInfo (m_cached_stop_info_sp);
        }
        else
        {
            SetStopInfo(StopInfo::CreateStopReasonWithSignal (*this, SIGSTOP));
        }
        return true;
    }
    return false;
}
