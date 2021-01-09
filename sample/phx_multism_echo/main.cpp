#include "multism_echo_server.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <string.h>
#include "phxpaxos/options.h"
#include <condition_variable>
#include <mutex>
#include <list>
#include <fstream>
#include <sstream>
#include <atomic>
using namespace phx_multism_echo;
using namespace phxpaxos;
using namespace std;

std::atomic_int64_t msg_cnt(0);

struct echo_message
{
    int seq;
    int file_size;
    echo_message(){};
    echo_message(int seq, int file_size):seq(seq),file_size(file_size){};
};


inline uint64_t get_time_us()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

int parse_ipport(const char *pcStr, NodeInfo &oNodeInfo)
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

int parse_ipport_list(const char *pcStr, NodeInfoList &vecNodeInfoList)
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

int run_single_server(int argc, char **argv)
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

        if (sEchoReqValue == string("quit"))
        {
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

// below class and function is now useless
class IpPort
{
public:
    string sIP;
    int iPort;
    IpPort(string ip, int port) : sIP(ip), iPort(port)
    {
    }
};

// below class and function is now useless
IpPort multi_parse_ipport(const char *pcStr)
{
    char sIP[32] = {0};
    int iPort = -1;
    sscanf(pcStr, "%[^':']:%d", sIP, &iPort);

    return IpPort(sIP, iPort);
}

// below class and function is now useless
void print_ip_port(IpPort o)
{
    cout << "IP: " << o.sIP << ", port: " << o.iPort << "\n";
}

// below class and function is now useless
vector<IpPort> multi_parse_ipport_list(const char *pcStr)
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
    for (int i = start_seq; i < end_seq; i++)
    {
        poMultismEchoServer->Echo(i, payload, sEchoRespValue);
    }
}

void echoTraceWorker(PhxMultismEchoServer *poMultismEchoServer, std::list<echo_message> &list, std::mutex &mutex, std::condition_variable &cv)
{
    string sEchoRespValue;
    while (true)
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&list]() { return !list.empty(); });
        echo_message ems = list.front();
        int msg_size = ems.file_size;
        list.pop_front();
        char *payload = static_cast<char *>(malloc(msg_size));
        for (int i = 0; i < msg_size; i++)
        {
            payload[i] = '0' + (i % 10);
        }
        poMultismEchoServer->Echo(ems.seq, payload, sEchoRespValue);
        msg_cnt += 1;
        printf("%ld done\n", msg_cnt.load());
    }
}


int run_multi_server(int argc, char **argv)
{
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

    if (num_sender > 200)
    {
        printf("currently we only support at most 200 threads\n");
        return -1;
    }

    PhxMultismEchoServer *poMultismEchoServer = new PhxMultismEchoServer(oMyNode, vecNodeInfoList, num_group, num_sm_per_group, num_io_thread);
    int ret = poMultismEchoServer->RunPaxos();
    if (ret != 0)
    {
        return -1;
    }
    printf("multism echo server start, ip %s port %d, %d groups with %d sms each and %d io threads\n", oMyNode.GetIP().c_str(), oMyNode.GetPort(), num_group, num_sm_per_group, num_io_thread);

    string sEchoReqValue;
    string sEchoRespValue;
    //fixed # of message
    // while (true)
    // {
    //     getline(cin, sEchoReqValue);

    //     int msg_count = std::atoi(sEchoReqValue.c_str());
    //     int msg_per_thread = msg_count / num_sender;
    //     vector<thread> vecThreads;

    //     uint64_t start_time = get_time_us();
    //     for (int i = 0; i < num_sender; i++)
    //     {
    //         vecThreads.push_back(thread(echoWorker, poMultismEchoServer, i * msg_per_thread, (i + 1) * msg_per_thread));
    //     }
    //     for (int i = 0; i < num_sender; i++)
    //     {
    //         vecThreads.at(i).join();
    //     }
    //     printf("%d messages used %lu ms\n", msg_count, (get_time_us() - start_time) / 1000);
    // }

    // trace
    getline(cin, sEchoReqValue);
    std::mutex list_mutex[200];
    std::condition_variable list_cv[200];
    std::list<echo_message> lists[200];

    std::ifstream inFile("/users/weijia/phxpaxos/sample/phx_multism_echo/trace_09_20.csv", std::ios::in);
    std::string lineStr;
    int msg_size = 8192 - 128;
    int message_index = 0;
    int total_files_num = 517294;
    // int total_files_num = 687;
    getline(inFile, lineStr);
    vector<thread> vecThreads;

    uint64_t start_time = get_time_us();
    for (int i = 0; i < num_sender; i++)
    {
        // vecThreads.push_back(thread(echoWorker, poMultismEchoServer, i * msg_per_thread, (i + 1) * msg_per_thread));
        vecThreads.push_back(thread(echoTraceWorker, poMultismEchoServer, std::ref(lists[i]), std::ref(list_mutex[i]), std::ref(list_cv[i])));
        // vecThreads.push_back(thread(tracetest, 1, 2, 3));
    }

    while (getline(inFile, lineStr))
    {
        std::vector<std::string> fields;
        std::stringstream ss(lineStr);
        std::string str;
        while (getline(ss, str, ','))
        {
            fields.push_back(str);
        }
        // int timestamp = atoi(fields[1].c_str());
        // // cout << "time: " << timestamp << endl;
        // sleep(timestamp);
        int file_size = atoi(fields[0].c_str());

        if (file_size < msg_size)
        {
            // printf("%d thread send:", message_index % num_sender);
            lists[message_index % num_sender].push_back(echo_message(message_index, file_size));
            list_cv[message_index % num_sender].notify_one();
            message_index++;
        }
        else
        {
            while (file_size > msg_size)
            {
                // printf("%d thread send:", message_index % num_sender);
                lists[message_index % num_sender].push_back(echo_message(message_index, msg_size));
                list_cv[message_index % num_sender].notify_one();
                file_size -= msg_size;
                message_index++;
            }
            if (file_size > 0)
            {
                // printf("%d thread send:", message_index % num_sender);
                lists[message_index % num_sender].push_back(echo_message(message_index, file_size));
                list_cv[message_index % num_sender].notify_one();
                message_index++;
            }
        }
    }
    printf("observe %d messages\n\n", message_index);
    while (msg_cnt != total_files_num)
    {
        sleep(0.01);
    }
    for (int i = 0; i < num_sender; i++)
    {
        vecThreads.at(i).detach();
    }
    printf("%d messages used %lu ms\n", total_files_num, (get_time_us() - start_time) / 1000);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 6)
        return run_single_server(argc, argv);
    else if (argc == 7)
        return run_multi_server(argc, argv);
}