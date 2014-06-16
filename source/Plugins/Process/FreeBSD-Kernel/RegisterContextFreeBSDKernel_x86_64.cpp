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
        if (process_sp->ReadMemory(kthread->GetPCB(), &pcb, sizeof(pcb), error) == 0)
        {
            return false;
        }
        else
        {
            size_t size = GetGPRSize();
            uint8_t* data = new uint8_t[size];

//          *(uint64_t*)(data + sizeof(uint64_t)*gpr_rbx_x86_64) = (uint64_t)pcb.pcb_rbx;
            *(uint64_t*)(data + 80) = (uint64_t)pcb.pcb_rbp;
//          *(uint64_t*)(data + sizeof(uint64_t)*gpr_rsp_x86_64) = (uint64_t)pcb.pcb_rsp;
//          *(uint64_t*)(data + sizeof(uint64_t)*gpr_r12_x86_64) = (uint64_t)pcb.pcb_r12;
//          *(uint64_t*)(data + sizeof(uint64_t)*gpr_r13_x86_64) = (uint64_t)pcb.pcb_r13;
//          *(uint64_t*)(data + sizeof(uint64_t)*gpr_r14_x86_64) = (uint64_t)pcb.pcb_r14;
//          *(uint64_t*)(data + sizeof(uint64_t)*gpr_r15_x86_64) = (uint64_t)pcb.pcb_r15;
            *(uint64_t*)(data + 136) = (uint64_t)pcb.pcb_rip;
            value = *(uint64_t *)(data + reg_info->byte_offset);
            printf("register name %s offset %d\n", reg_info->name, reg_info->byte_offset);
            delete [] data;
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
