//客户端通步骤
/*
1.创建用于通信的socket
2.绑定服务器ip和端口
3.连接服务器
4.通讯
5.关闭连接
*/
#include<cstring>
#include<iostream>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<unistd.h>
#include<stdlib.h>
using namespace std;
//sb main函数都忘记写了！
int main(){
    //1.创建用于通讯的socket
int cfd=socket(AF_INET,SOCK_STREAM,0);
if(cfd==-1){
    perror("socket");
    exit(0); //退出忘记写
}
// struct sockaddr_in addr_client; //应为server地址
//2.指定服务器IP和端口
struct sockaddr_in addr_server;
addr_server.sin_family=AF_INET;
addr_server.sin_port=htons(10000); //端口：主机序->网络序
inet_pton(AF_INET,"192.168.137.137",&addr_server.sin_addr.s_addr); //ip:主机序->网络序

//3.连接
int ret=connect(cfd,(struct sockaddr*)&addr_server,sizeof(addr_server));
if(ret==-1){
    perror("connect");
    exit(0);
}
//4.通讯
int number{0};
while(1){
    char buf[1024]{0};//用作读写缓存区
    auto str="hello server"+to_string(number++);
    strcpy(buf,str.c_str());
    //发送数据
    write(cfd,buf,strlen(buf)+1);

    //接收数据
    fill(begin(buf),end(buf),0); //cpp替代memset
    int len=read(cfd,buf,sizeof(buf));
    if(len>0){
        cout<<buf<<endl;
    }else if(len=0){
        cout<<"连接已断开"<<endl;
        exit(0);
    }else{
        perror("read");
        break;
    }
    sleep(1);//控制发送和接收时延
}
}