/** I/O 复用 select 模式
    改自 client
    传统 client 采用阻塞模式效率较低
*/
#include<cstring>
#include<iostream>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<unistd.h>
#include<stdlib.h>
using namespace std;

    int main(){
        //1.创建用于通信的套接字
        int cfd = socket(AF_INET,SOCK_STREAM,0);
        if(cfd==-1){
            perror("socket");
            exit(0);
        }

        //2.创建服务器 socket 地址并赋值
        sockaddr_in server_sock_addr {};
        server_sock_addr.sin_family=AF_INET;
        server_sock_addr.sin_port=htons(10000); 
        inet_pton(AF_INET,"172.16.172.129",&server_sock_addr.sin_addr.s_addr);
        
        //3.与服务器建立连接
        int con=connect(cfd,(sockaddr*)&server_sock_addr,sizeof(server_sock_addr));
        if(con==-1){
            perror("connect");
        }
        //4.收发数据
        int number {0};
        while(1){
            char buffer[1024] {0};  //读写缓冲区
            //发数据
            auto msg="客户端发送消息"+to_string(number++);
            strcpy(buffer,msg.c_str());        //msg存入缓冲区
            int ret_write = write(cfd,buffer,strlen(buffer)+1);  //写入数据
            if(ret_write==-1){
                // cout<<"连接已断开"<<endl;
                // return 0;
                perror("write");
                break;
            }

            //收数据
            fill(begin(buffer),end(buffer),0); //重置缓冲区
            int ret_read = read(cfd,buffer,sizeof(buffer));  //读取数据
            if(ret_read<=0){
                cout<<"服务器已断开"<<endl;
                // return 0;
                exit(1);
            }
            cout<<buffer<<endl;
            sleep(1);
        }
        close(cfd);
        return 0;
    }

