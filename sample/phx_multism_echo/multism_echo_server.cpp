#include "multism_echo_server.h"
#include <assert.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>

using namespace phxpaxos;
using namespace std;

namespace phx_multism_echo
{
PhxMultismEchoServer :: PhxMultismEchoServer(const phxpaxos::NodeInfo & oMyNode, const phxpaxos::NodeInfoList & vecNodeList, int num_group, int num_sm_per_group) : m_oMyNode(oMyNode), m_vecNodeList(vecNodeList), m_poPaxosNode(nullptr), num_group(num_group), num_sm_per_group(num_sm_per_group){}

PhxMultismEchoServer :: ~PhxMultismEchoServer()
{
    delete m_poPaxosNode;
}


int PhxMultismEchoServer :: MakeLogStoragePath(std::string & sLogStoragePath)
{
    char sTmp[128] = {0};
    snprintf(sTmp, sizeof(sTmp), "./logpath_%s_%d", m_oMyNode.GetIP().c_str(), m_oMyNode.GetPort());

    sLogStoragePath = string(sTmp);

    if (access(sLogStoragePath.c_str(), F_OK) == -1)
    {
        if (mkdir(sLogStoragePath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
        {       
            printf("Create dir fail, path %s\n", sLogStoragePath.c_str());
            return -1;
        }       
    }

    return 0;
}

int PhxMultismEchoServer :: RunPaxos()
{
    Options oOptions;

    int ret = MakeLogStoragePath(oOptions.sLogStoragePath);
    if (ret !=0)
    {
        return ret;
    }
    oOptions.iGroupCount = num_group;

    oOptions.oMyNode = m_oMyNode;
    oOptions.vecNodeInfoList = m_vecNodeList;

    // multi group, each with multi sm
    for (int i = 0; i < num_group; i++) {
        GroupSMInfo oSMInfo;
        oSMInfo.iGroupIdx = i;

        for (int j = 0; j < num_sm_per_group; j++){
            PhxMultismEchoSM *poMultismEchoSM = new PhxMultismEchoSM(j + 1);
            oSMInfo.vecSMList.push_back(poMultismEchoSM);
        }
        oOptions.vecGroupSMInfoList.push_back(oSMInfo);
    }

    //use logger_google to print log
    LogFunc pLogFunc;
    ret = LoggerGoogle :: GetLogger("phxecho", "./log", 3, pLogFunc);
    if (ret != 0)
    {
        printf("get logger_google fail, ret %d\n", ret);
        return ret;
    }

    //set logger
    oOptions.pLogFunc = pLogFunc;

    ret = Node::RunNode(oOptions, m_poPaxosNode);
    if (ret != 0)
    {
        printf("run paxos fail, ret %d\n", ret);
        return ret;
    }

    printf("run paxos ok\n");
    return 0;
}

int PhxMultismEchoServer :: Echo(const int seq_no, const std::string & sEchoReqValue, std::string & sEchoRespValue)
{
    uint64_t llInstanceID = 0;
    SMCtx oCtx;
    //smid must same to PhxEchoSM.SMID().

    int temp_id  = seq_no % (num_group * num_sm_per_group);

    int group_id = temp_id / num_sm_per_group;
    int sm_id = temp_id % num_sm_per_group + 1;

    oCtx.m_iSMID = sm_id;

    // choose group by seq_no, TODO: do not know how to choose SM yet.
    int ret = m_poPaxosNode->Propose(group_id, sEchoReqValue, llInstanceID, &oCtx);

    if (ret != 0)
    {
        printf("paxos propose fail, ret %d\n", ret);
        return ret;
    }

    printf("propose message %d to (group %d, sm %d) successfully, instanceid %lu\n", seq_no, group_id, sm_id, llInstanceID);
    sEchoRespValue = to_string(sEchoReqValue.size());

    return 0;
}

}