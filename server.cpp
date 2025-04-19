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

    //4.等待连接
    struct sockaddr_in addr_client;//用于接受来自监听的客户端地址信息
    socklen_t len=sizeof(addr_client);
    int cfd=accept(lfd,(struct sockaddr*)&addr_client,&len);
    if(cfd==-1){
        perror("accept");
        exit(0);
    }
    //打印监听到的ip和端口号
    char ip[32]{0};
    cout<<"监听到的ip："<<inet_ntop(AF_INET,&addr_client.sin_addr.s_addr,ip,sizeof(ip))<<" "<<"监听到的端口："<<ntohs(addr_client.sin_port)<<endl;
    //5.开始传输数据
    int number{0};
    while(1){
        char buf[1024]{0};
        int ret=read(cfd,buf,sizeof(buf));
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
        auto str="服务器已确认收到客户端信息"+to_string(number);
        strcpy(buf,str.c_str());
        write(cfd,buf,strlen(buf));
        sleep(1);
    }
    close(lfd);
    close(cfd);
}
