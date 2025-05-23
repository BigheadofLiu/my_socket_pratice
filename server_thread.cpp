#include<iostream>
#include<cstring>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<unistd.h> 
#include<thread>
#include<vector>
using namespace std;
#define MAX_CONNECT 512 //定义最大信息连接
//server 改为多线程
//服务端步骤
/*
----主线程----
1.创建用于监听的socket
2.绑定本地ip和端口号 bind
3.开始监听 listen
----子线程----
4.等待连接（阻塞式）accept返回一个用于通讯的socket
5.传输数据 （read，write）
6.关闭连接
*/

//新增一个用于存储主进程监听到的通讯socket和socketaddr_in地址(方便传递给子进程)
struct accept_info{
    struct sockaddr_in addr_client;  //客户端地址信息
    int cfd; //通信文件描述符
};
// accept_info info[MAX_CONNECT]{0}; //使用数组存储信息 cpp能否改为用vector?
vector<accept_info> v_info(MAX_CONNECT);
//子进程函数声明
void do_thread(void* args);
int main(){

    //1.创建监听socket（不变）
    int lfd=socket(AF_INET,SOCK_STREAM,0);
    if (lfd==-1)
    {
        /* code */
        perror("socket");
    }
    
    //指定本地ip和端口并绑定
    struct sockaddr_in addr_server;
    addr_server.sin_family=AF_INET;
    addr_server.sin_port=htons(10000);
    // inet_pton(lfd,"192.168.137.137",&addr_server.sin_addr.s_addr); //妈的写错了
    inet_pton(AF_INET,"192.168.137.137",&addr_server.sin_addr.s_addr);
    // addr_server.sin_addr.s_addr=INADDR_ANY;
    //服务器地址设置之后忘记bind！！！ are you pig？
    int bd=bind(lfd,(struct sockaddr*)&addr_server,sizeof(addr_server));
    if(bd==-1){
        perror("bind");
    }

    //3.开始监听
    int lst=listen(lfd,128);
    if(lst==-1){
        perror("listen");
    }
    // accept_info info[MAX_CONNECT]; //创建一个用于存储客户端信息的数组（通讯socket和addr）由于是子线程函数也需要调用，应定义在全局区

    //4.等待连接
    
    //循环检测是否有客户端发起
    // struct sockaddr_in addr_client={0};//用于接受来自监听的客户端地址信息(这里就用不到了)
    // socklen_t len=sizeof(addr_client);  //还能用一下

    //初始化info数组
    for(auto &i:v_info){  //error：范围for循环不会修改初始值 通过引用修改
        i.addr_client={};
        i.cfd=-1;  //-1为置空标志
    }
    while(1){
        struct accept_info* p_info=nullptr;
        // socklen_t len=sizeof(p_info->addr_client);
        socklen_t len=sizeof(sockaddr_in);
        for(auto& i:v_info){
            if(i.cfd==-1){
                p_info=&i;
                break; //break在这里
            }
            //如果未找到空闲 证明已超最大连接数
        }
        if(!p_info){
            cerr<<"已达最大连接数"<<endl;
            continue;
        }
        p_info->cfd=accept(lfd,(struct sockaddr*)&p_info->addr_client,&len);
        //存储cfd
        // p_info->cfd=cfd;
        if(p_info->cfd==-1){
            perror("accept");
            // exit(0);                                         
            continue;
        }
        //创建子线程
        thread t1(do_thread,p_info);
        t1.detach();  //子线程和主线程分离
    }
}

void do_thread(void* args){
    //打印监听到的ip和端口号
    accept_info* p_info=(accept_info*)args;

    char ip[32]{0};
    cout<<"监听到的ip："<<inet_ntop(AF_INET,&p_info->addr_client.sin_addr.s_addr,ip,sizeof(ip))<<" "<<"监听到的端口："<<ntohs(p_info->addr_client.sin_port)<<endl;
    //5.开始传输数据
    int number=0;
    while(1){
        char buf[1024]{0};
        int ret=read(p_info->cfd,buf,sizeof(buf));
        if(ret>0){
            cout<<"接收到客户端的信息："<<buf/* <<" "<<number++ */<<endl;
        }else if(ret=0){
            cout<<"连接已断开"<<endl;
            break;
        }else{
            perror("read");
            break;
        }
        fill(begin(buf),end(buf),0);
        auto str="服务器已确认收到客户端信息:"+to_string(number);
        strcpy(buf,str.c_str());
        write(p_info->cfd,buf,strlen(buf));
        sleep(1);
    }
    // close(lfd);
    // close(cfd);
    close(p_info->cfd);
    p_info->cfd=-1;
    
}

