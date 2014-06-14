//===-- RegisterContextFreBSDKernel_x86_64.h --------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//

#ifndef liblldb_RegisterContextFreeBSDKernel_x86_64_H_
#define liblldb_RegisterContextFreeBSDKernel_x86_64_H_

#include "Plugins/Process/Utility/RegisterContextPOSIX_x86.h"

class RegisterContextFreeBSDKernel_x86_64 :
    public RegisterContextPOSIX_x86
{
public:
    RegisterContextFreeBSDKernel_x86_64 (lldb_private::Thread &thread,
                                     lldb_private::RegisterInfoInterface *register_info);

    ~RegisterContextFreeBSDKernel_x86_64();

    virtual bool
    ReadRegister(const lldb_private::RegisterInfo *reg_info, lldb_private::RegisterValue &value);

    virtual bool
    WriteRegister(const lldb_private::RegisterInfo *reg_info, const lldb_private::RegisterValue &value);

    bool
    ReadAllRegisterValues(lldb::DataBufferSP &data_sp);

    bool
    WriteAllRegisterValues(const lldb::DataBufferSP &data_sp);

    bool
    HardwareSingleStep(bool enable);

protected:
    bool
    ReadGPR();

    bool
    ReadFPR();

    bool
    WriteGPR();

    bool
    WriteFPR();

private:
};

#endif // #ifndef liblldb_RegisterContextFreeBSDKernel_x86_64_H_
