#pragma once

#include "phxpaxos/node.h"
#include "multism_echo_sm.h"
#include <string>
#include <vector>
#include "phxpaxos_plugin/logger_google.h"

namespace phx_multism_echo
{

class PhxMultismEchoServer
{
public:
    PhxMultismEchoServer(const phxpaxos::NodeInfo & oMyNode, const phxpaxos::NodeInfoList & vecNodeList, int num_group, int num_sm_per_group, int num_io_thread);
    ~PhxMultismEchoServer();

    int RunPaxos();

    int Echo(const int seq_no, const std::string & sEchoReqValue, std::string & sEchoRespValue);

private:
    int MakeLogStoragePath(std::string & sLogStoragePath);

    phxpaxos::NodeInfo m_oMyNode;
    phxpaxos::NodeInfoList m_vecNodeList;

    phxpaxos::Node * m_poPaxosNode;   

    int num_group;
    int num_sm_per_group;
    int num_io_thread;

    bool bUseBatch;

};

}