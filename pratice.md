#### socket练习 
###### 服务器
 1.创建用于监听的socket lfd  
 2.创建用于存储server地址的sockaddr_in地址，并赋值  
 3.将lfd与server_addr绑定 bind(lfd,(struct sockaddr*)&server_addr,sizeof(serveraddr))  
 4.开始监听 listen(lfd,128);  
 5.创建用于存储监听到的客户端地址的sockaddr_in地址  
 6.accept(lfd,)  
 ~~7.开始连接 connect（）~~ 服务器不需要connect  
 7.通讯 read write  
 8.关闭sokcet  
 
 ###### 代码
 ```cpp
 #include<所有需要的头文件>
 using namespace std；
 int main(){
    int lfd=socket(网络协议（IPv4&ipv6），传输模式（流&块），0)；
    struct sockaddr_in server_addr;
    server_addr.通讯协议=();
    server_addr.端口=(主机序->网络序);
    inet_pton(网络协议，"ip",&server_addr.ip.ip);(主机序->网络序)
    bind(lfd,(struct sockaddr*)&server_addr,sizeof(server_addr));

    listen(lfd,128);
    struct sockr_in client_addr
    int cfd=accept(lfd,(struct sockaddr*)&client_addr,sizeof(client_addr));

    cout<<inet_ntop(client_addr.ip)<<(client_addr.port) //注意网络序->主机序

    while(1){
        //接收数据
        char buf[1024]{0};
        int ret=read(cfd,buf,strlen(buf)+1);
        if(ret>0){
            cout<<buf<<endl;
        }else if(...){

        }else{
            ...
        }
        //发送数据 
        fill(begin(buf),end(buf),0);
        buf="...";
        write(cfd,buf,sizeof(buf));
    }
    close(lfd);
    close(cfd);
 }
 ```
###### 客户端步骤
1.创建用于通讯的socket  
2.指定服务器地址  
3.连接服务器connect  
4.传输数据  
5.关闭连接  
```cpp
#include<需要用到的头文件>
using namespace std;
int main(){
    cfd=socket(AF_INIT,传输方式，0)；
    struct sockaddr_in server_addr;
    server.addr.通讯协议=...;
    server.addr.port=ntohs();
    inet_ntop(通讯协议，"ip",server_addr.ip.ip);
    connect(cfd,(struct sockaddr*)&server_addr,sizeof(addr));

    while(1){
        buf[1024]{0};
        buf="";
        //发送数据
        write(cfd,buf,sizeof(buf));

        fill(begin(buf),endbuf(),0);
        //接收数据
        read(cfd,buf,strlen(buf));
        ...
    }
    close(cfd);
}
```