#include<iostream>
#include<cstring>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<unistd.h> 
using namespace std;

//服务端步骤
/*
1.创建用于监听的socket
2.绑定本地ip和端口号 bind
3.开始监听 listen
4.等待连接（阻塞式）accept返回一个用于通讯的socket
5.传输数据 （read，write）
6.关闭连接
*/
int main(){
    //1.创建监听socket
    int lfd=socket(AF_INET,SOCK_STREAM,0); //ipv4 tcp
    if (lfd==-1)   //error
    {
        /* code */
        perror("socket");
    }
    
    //指定本地ip和端口并绑定
    struct sockaddr_in addr_server;   //适用于sock的结构体地址（协议，端口，ip）<- 端口和 ip 均需要网络序列保存
    addr_server.sin_family=AF_INET;  //ipv4
    addr_server.sin_port=htons(10000);  //port  host to network short（字节序(小端)->网络序(大端)）
    // inet_pton(lfd,"192.168.137.137",&addr_server.sin_addr.s_addr); //妈的写错了
    // inet_pton(AF_INET,"192.168.137.137",&addr_server.sin_addr.s_addr);
    inet_pton(AF_INET,"172.16.172.129",&addr_server.sin_addr.s_addr);  //使用 ipv4 本地字符串 ip -> 网络字节序 &存储在传入的地址中

    // addr_server.sin_addr.s_addr=INADDR_ANY;
    //服务器地址设置之后忘记bind！！！ are you pig？
    int bd=bind(lfd,(struct sockaddr*)&addr_server,sizeof(addr_server));   //把用于监听的 socket 和 地址绑定
    if(bd==-1){
        perror("bind");
    }

    //3.开始监听
    int lst=listen(lfd,128);
    if(lst==-1){
        perror("listen");
    }

    //4.等待连接
    struct sockaddr_in addr_client;//用于接受来自监听的客户端地址信息
    socklen_t len=sizeof(addr_client);  //指定长度
    int cfd=accept(lfd,(struct sockaddr*)&addr_client,&len);   //cfd 用于接收 accept 监听到的客户端 socket
    if(cfd==-1){
        perror("accept");
        exit(0);
    }
    //打印监听到的ip和端口号
    char ip[32]{0};
    cout<<"监听到的ip："<<inet_ntop(AF_INET,&addr_client.sin_addr.s_addr,ip,sizeof(ip))<<" "<<"监听到的端口："<<ntohs(addr_client.sin_port)<<endl;
    //5.开始传输数据
    //阻塞式使用 read/write没有问题 
    //非阻塞是记得用 recv/send
    int number{0};
    while(1){
        char buf[1024]{0};  // 读写数据 buffer
        int ret=read(cfd,buf,sizeof(buf));//读取 cfd 中发送来的数据
        if(ret>0){
            cout<<"接收到客户端的信息："<<buf/* <<" "<< number++ */ <<endl;
        }else if(ret=0){
            cout<<"连接已断开"<<endl;
            break;
        }else{
            perror("read");
            break;
        }

        //发送数据
        fill(begin(buf),end(buf),0);  //清空 buffer
        auto str="服务器已确认收到客户端信息"+to_string(number++);
        strcpy(buf,str.c_str());
        write(cfd,buf,strlen(buf));   //往 cfd 中发数据
        sleep(1);
    }
    close(lfd);
    close(cfd);
}
