#include "multism_echo_server.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <string.h>
#include "phxpaxos/options.h"

using namespace phx_multism_echo;
using namespace phxpaxos;
using namespace std;

inline uint64_t get_time_us()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

int parse_ipport(const char * pcStr, NodeInfo & oNodeInfo)
{
    char sIP[32] = {0};
    int iPort = -1;

    int count = sscanf(pcStr, "%[^':']:%d", sIP, &iPort);
    printf("%s\n", pcStr);
    printf("IP: %s, port: %d\n", sIP, iPort);
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

int run_single_server(int argc, char** argv)
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
    int num_io_thread = atoi(argv[5]);

    PhxMultismEchoServer oMultismEchoServer(oMyNode, vecNodeInfoList, num_group, num_sm_per_group, num_io_thread);
    int ret = oMultismEchoServer.RunPaxos();
    if (ret != 0)
    {
        return -1;
    }
    printf("multism echo server start, ip %s port %d, %d groups with %d sms each and %d io threads\n", oMyNode.GetIP().c_str(), oMyNode.GetPort(), num_group, num_sm_per_group, num_io_thread);

    string sEchoReqValue;
    string sEchoRespValue;
    int seq_no = 0;
    while (true)
    {
        int msg_size = 1000;
        getline(cin, sEchoReqValue);

        if(sEchoReqValue == string("quit")) {
            break;
        }

        char *payload = static_cast<char *>(malloc(msg_size));
        for (int i = 0; i < msg_size; i++)
        {
            payload[i] = '0' + (i % 10);
        }
        uint64_t start_time = get_time_us();
        for (int i = 0; i < std::atoi(sEchoReqValue.c_str()); i++)
        {
            ret = oMultismEchoServer.Echo(seq_no++, payload, sEchoRespValue);
            if (ret != 0)
            {
                printf("Echo fail, ret %d\n", ret);
            }
            else
            {
                printf("echo resp value %lu\n", sEchoRespValue.size());
            }
        }
        printf("%d messages used %lu ms\n", std::atoi(sEchoReqValue.c_str()), (get_time_us() - start_time) / 1000);
    }
    return 0;
}

class IpPort
{
public:
    string sIP;
    int iPort;
    IpPort(string ip, int port):sIP(ip), iPort(port){
    }
};


IpPort multi_parse_ipport(const char * pcStr)
{
    char sIP[32] = {0};
    int iPort = -1;
    sscanf(pcStr, "%[^':']:%d", sIP, &iPort);

    return IpPort(sIP, iPort);
}

void print_ip_port(IpPort o)
{
    cout << "IP: " << o.sIP << ", port: " << o.iPort << "\n";
}

vector<IpPort> multi_parse_ipport_list(const char * pcStr)
{
    string sTmpStr;
    int iStrLen = strlen(pcStr);
    vector<IpPort> vecRtn;

    for (int i = 0; i < iStrLen; i++)
    {
        if (pcStr[i] == ',' || i == iStrLen - 1)
        {
            if (i == iStrLen - 1 && pcStr[i] != ',')
            {
                sTmpStr += pcStr[i];
            }
            
            IpPort oRet = multi_parse_ipport(sTmpStr.c_str());

            vecRtn.push_back(oRet);

            sTmpStr = "";
        }
        else
        {
            sTmpStr += pcStr[i];
        }
    }
    return vecRtn;
}

void echoWorker(PhxMultismEchoServer *poMultismEchoServer, int start_seq, int end_seq)
{
    string sEchoRespValue;
    
    int msg_size = 1000;
    char *payload = static_cast<char *>(malloc(msg_size));
    for (int i = 0; i < msg_size; i++)
    {
        payload[i] = '0' + (i % 10);
    }
    for(int i = start_seq; i < end_seq; i++)
    {
        poMultismEchoServer->Echo(i, payload, sEchoRespValue);
    }

}

int run_multi_server(int argc, char** argv)
{
    // IpPort local_ip_starter = multi_parse_ipport(argv[1]);
    
    // print_ip_port(local_ip_starter);
    
    // vector<IpPort> all_ip_starter = multi_parse_ipport_list(argv[2]);
    // for (auto o : all_ip_starter)
    // {
    //     print_ip_port(o);
    // }

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
    int num_sender = atoi(argv[3]);
    int num_group = atoi(argv[4]);
    int num_sm_per_group = atoi(argv[5]);
    int num_io_thread = atoi(argv[6]);

    PhxMultismEchoServer *poMultismEchoServer = new PhxMultismEchoServer(oMyNode, vecNodeInfoList, num_group, num_sm_per_group, num_io_thread);
    int ret = poMultismEchoServer->RunPaxos();
    if (ret != 0)
    {
        return -1;
    }
    printf("multism echo server start, ip %s port %d, %d groups with %d sms each and %d io threads\n", oMyNode.GetIP().c_str(), oMyNode.GetPort(), num_group, num_sm_per_group, num_io_thread);

    string sEchoReqValue;
    string sEchoRespValue;
    while (true)
    {
        getline(cin, sEchoReqValue);

        int msg_count = std::atoi(sEchoReqValue.c_str());
        int msg_per_thread = msg_count / num_sender;
        vector<thread> vecThreads;

        uint64_t start_time = get_time_us();
        for (int i = 0; i < num_sender; i++) {
            vecThreads.push_back(thread(echoWorker, poMultismEchoServer, i * msg_per_thread, (i + 1) * msg_per_thread));
        }
        for (int i = 0; i < num_sender; i++) {
            vecThreads.at(i).join();
        }
        printf("%d messages used %lu ms\n", msg_count, (get_time_us() - start_time) / 1000);


    }
    return 0;
}

int main(int argc, char** argv)
{
    if (argc == 6)
        return run_single_server(argc, argv);
    else if (argc == 7)
        return run_multi_server(argc, argv);
}