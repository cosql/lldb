//===-- RegisterContextFreeBSDKernel_x86_64.cpp -----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include <sys/types.h>
#include <machine/pcb.h>

#include "lldb/Core/DataExtractor.h"
#include "lldb/Core/RegisterValue.h"
#include "lldb/Target/Thread.h"
#include "ProcessFreeBSDKernel.h"
#include "RegisterContextPOSIX.h"
#include "RegisterContextFreeBSDKernel_x86_64.h"
#include "ThreadFreeBSDKernel.h"

using namespace lldb;
using namespace lldb_private;

RegisterContextFreeBSDKernel_x86_64::RegisterContextFreeBSDKernel_x86_64(Thread &thread,
                                                                         RegisterInfoInterface *register_info)
    : RegisterContextPOSIX_x86 (thread, 0, register_info),
      m_gpr(nullptr)
{
    ProcessSP process_sp (CalculateProcess());
    Error error;
    struct pcb pcb;
    if (process_sp)
    {
        ThreadFreeBSDKernel *kthread = static_cast<ThreadFreeBSDKernel*>(&m_thread);
        if (process_sp->ReadMemory(kthread->GetPCB(), &pcb, sizeof(pcb), error) != sizeof(pcb))
        {
            return;
        }
        else
        {
            m_gpr  = new GPR;
            memset(m_gpr, 0, sizeof(GPR));
            m_gpr->rbx = (uint64_t)pcb.pcb_rbx;
            m_gpr->rbp = (uint64_t)pcb.pcb_rbp;
            m_gpr->rsp = (uint64_t)pcb.pcb_rsp;
            m_gpr->r12 = (uint64_t)pcb.pcb_r12;
            m_gpr->r13 = (uint64_t)pcb.pcb_r13;
            m_gpr->r14 = (uint64_t)pcb.pcb_r14;
            m_gpr->r15 = (uint64_t)pcb.pcb_r15;
            m_gpr->rip = (uint64_t)pcb.pcb_rip;

        }
    }
}

RegisterContextFreeBSDKernel_x86_64::~RegisterContextFreeBSDKernel_x86_64()
{
    delete m_gpr;
}

bool
RegisterContextFreeBSDKernel_x86_64::ReadGPR()
{
    return false;
}

bool
RegisterContextFreeBSDKernel_x86_64::ReadFPR()
{
    return false;
}

bool
RegisterContextFreeBSDKernel_x86_64::WriteGPR()
{
    assert(0);
    return false;
}

bool
RegisterContextFreeBSDKernel_x86_64::WriteFPR()
{
    assert(0);
    return false;
}

bool
RegisterContextFreeBSDKernel_x86_64::ReadRegister(const RegisterInfo *reg_info, RegisterValue &value)
{
    if (m_gpr)
    {
        value = *(uint64_t *)((char *)m_gpr + reg_info->byte_offset);

        return true;
    }
    return false;
}

bool
RegisterContextFreeBSDKernel_x86_64::ReadAllRegisterValues(lldb::DataBufferSP &data_sp)
{
    return false;
}

bool
RegisterContextFreeBSDKernel_x86_64::WriteRegister(const RegisterInfo *reg_info, const RegisterValue &value)
{
    return false;
}

bool
RegisterContextFreeBSDKernel_x86_64::WriteAllRegisterValues(const lldb::DataBufferSP &data_sp)
{
    return false;
}

bool
RegisterContextFreeBSDKernel_x86_64::HardwareSingleStep(bool enable)
{
    return false;
}

size_t
RegisterContextFreeBSDKernel_x86_64::GetGPRSize()
{
    return sizeof(GPR);
}
