#include "multism_echo_sm.h"

namespace phx_multism_echo
{
PhxMultismEchoSM :: PhxMultismEchoSM(int sm_id) : sm_id(sm_id) {}

bool PhxMultismEchoSM ::Execute(const int iGroupIdx, const uint64_t llInstanceID, 
            const std::string & sPaxosValue, phxpaxos::SMCtx * poSMCtx)
{
    // printf("[SM Execute] ok, smid %d instanceid %lu value length %lu\n", 
    //         SMID(), llInstanceID, sPaxosValue.size());
    
    return true;
}
} // namespace phx_multism_echo