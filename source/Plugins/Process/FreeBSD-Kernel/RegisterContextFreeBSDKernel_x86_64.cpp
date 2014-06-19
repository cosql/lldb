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

namespace {
    typedef struct _GPR
    {
        uint64_t r15;
        uint64_t r14;
        uint64_t r13;
        uint64_t r12;
        uint64_t r11;
        uint64_t r10;
        uint64_t r9;
        uint64_t r8;
        uint64_t rdi;
        uint64_t rsi;
        uint64_t rbp;
        uint64_t rbx;
        uint64_t rdx;
        uint64_t rcx;
        uint64_t rax;
        uint32_t trapno;
        uint16_t fs;
        uint16_t gs;
        uint32_t err;
        uint16_t es;
        uint16_t ds;
        uint64_t rip;
        uint64_t cs;
        uint64_t rflags;
        uint64_t rsp;
        uint64_t ss;
    } GPR;
}

RegisterContextFreeBSDKernel_x86_64::RegisterContextFreeBSDKernel_x86_64(Thread &thread,
                                                                         RegisterInfoInterface *register_info)
    : RegisterContextPOSIX_x86 (thread, 0, register_info)
{
}

RegisterContextFreeBSDKernel_x86_64::~RegisterContextFreeBSDKernel_x86_64()
{
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
    ProcessSP process_sp (CalculateProcess());
    Error error;
    struct pcb pcb;
    if (process_sp)
    {
        ThreadFreeBSDKernel *kthread = static_cast<ThreadFreeBSDKernel*>(&m_thread);
        if (process_sp->ReadMemory(kthread->GetPCB(), &pcb, sizeof(pcb), error) != sizeof(pcb))
        {
            return false;
        }
        else
        {
            GPR gpr;
            gpr.rbx = (uint64_t)pcb.pcb_rbx;
            gpr.rbp = (uint64_t)pcb.pcb_rbp;
            gpr.rsp = (uint64_t)pcb.pcb_rsp;
            gpr.r12 = (uint64_t)pcb.pcb_r12;
            gpr.r13 = (uint64_t)pcb.pcb_r13;
            gpr.r14 = (uint64_t)pcb.pcb_r14;
            gpr.r15 = (uint64_t)pcb.pcb_r15;
            gpr.rip = (uint64_t)pcb.pcb_rip;

            value = *(uint64_t *)((char *)&gpr + reg_info->byte_offset);

            return true;
        }
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
