//===-- RegisterContextFreeBSDKernel_x86_64.cpp -----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/Core/DataExtractor.h"
#include "lldb/Core/RegisterValue.h"
#include "lldb/Target/Thread.h"
#include "RegisterContextPOSIX.h"
#include "RegisterContextFreeBSDKernel_x86_64.h"
#include "ThreadFreeBSDKernel.h"

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
