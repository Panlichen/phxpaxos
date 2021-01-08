#include "multism_echo_server.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <string.h>
#include "phxpaxos/options.h"

using namespace phx_multism_echo;
using namespace phxpaxos;
using namespace std;

int parse_ipport(const char * pcStr, NodeInfo & oNodeInfo)
{
    char sIP[32] = {0};
    int iPort = -1;

    int count = sscanf(pcStr, "%[^':']:%d", sIP, &iPort);
    if (count != 2)
    {
        return -1;
    }

    oNodeInfo.SetIPPort(sIP, iPort);

    return 0;
}

int parse_ipport_list(const char * pcStr, NodeInfoList & vecNodeInfoList)
{
    string sTmpStr;
    int iStrLen = strlen(pcStr);

    for (int i = 0; i < iStrLen; i++)
    {
        if (pcStr[i] == ',' || i == iStrLen - 1)
        {
            if (i == iStrLen - 1 && pcStr[i] != ',')
            {
                sTmpStr += pcStr[i];
            }
            
            NodeInfo oNodeInfo;
            int ret = parse_ipport(sTmpStr.c_str(), oNodeInfo);
            if (ret != 0)
            {
                return ret;
            }

            vecNodeInfoList.push_back(oNodeInfo);

            sTmpStr = "";
        }
        else
        {
            sTmpStr += pcStr[i];
        }
    }

    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        printf("%s <myip:myport> <node0_ip:node_0port,node1_ip:node_1_port,node2_ip:node2_port,...>\n", argv[0]);
        return -1;
    }

    NodeInfo oMyNode;
    if (parse_ipport(argv[1], oMyNode) != 0)
    {
        printf("parse myip:myport fail\n");
        return -1;
    }

    NodeInfoList vecNodeInfoList;
    if (parse_ipport_list(argv[2], vecNodeInfoList) != 0)
    {
        printf("parse ip/port list fail\n");
        return -1;
    }

    int num_group = atoi(argv[3]);
    int num_sm_per_group = atoi(argv[4]);

    PhxMultismEchoServer oMultismEchoServer(oMyNode, vecNodeInfoList, num_group, num_sm_per_group);
    int ret = oMultismEchoServer.RunPaxos();
    if (ret != 0)
    {
        return -1;
    }
    printf("multism echo server start, ip %s port %d, %d groups with %d sms each\n", oMyNode.GetIP().c_str(), oMyNode.GetPort(), num_group, num_sm_per_group);

    string sEchoReqValue;
    int seq_no = 0;
    while (true)
    {
        printf("\nplease input: <echo req value>\n");
        getline(cin, sEchoReqValue);
        string sEchoRespValue;
        ret = oMultismEchoServer.Echo(seq_no++, sEchoReqValue, sEchoRespValue);
        if (ret != 0)
        {
            printf("Echo fail, ret %d\n", ret);
        }
        else
        {
            printf("echo resp value %s\n", sEchoRespValue.c_str());
        }
    }
}