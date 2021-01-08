#pragma once

#include "phxpaxos/sm.h"
#include "phxpaxos/options.h"
#include <stdio.h>
#include <unistd.h>


namespace phx_multism_echo
{
// TODO: I think this is now useless
// class PhxMultismEchoCtx 

class PhxMultismEchoSM : public phxpaxos::StateMachine
{
public:
    PhxMultismEchoSM(int sm_id);

    bool Execute(const int iGroupIdx, const uint64_t llInstanceID, 
            const std::string & sPaxosValue, phxpaxos::SMCtx * poSMCtx);

    const int SMID() const {return sm_id;}

private:
    int sm_id;
};

}