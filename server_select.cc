/**
 *  select模式的 socket server客户端
 *  改自 server.cpp
 *  v1.0 单线程版本
 */
#include<iostream>
#include<cstring>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<unistd.h> 
#include<sys/select.h> //using select


int main(){
        //1.创建用于监听的套接字
        int lfd=socket(AF_INET,SOCK_STREAM,0);
        if(lfd==-1){
            perror("socket");
        }

        //2.创建用于socket通信的地址
        sockaddr_in server_socket_addr{}; 

        //这样写对吗？看起来更直接一点 c++17不支持 老实分开写吧
        // sockaddr_in server_socket_addr{
        //    .sin_addr= {.s_addr=inet_addr("172.16.172.129")},
        //    .sin_family=AF_INET,
        //    .sin_port=htons(1000),
        // };

        server_socket_addr.sin_family=AF_INET;  //协议
        server_socket_addr.sin_port=htons(10000); //端口
        inet_pton(AF_INET,"172.16.172.129",&server_socket_addr.sin_addr.s_addr); //ip
        
        //3.地址与套接字绑定  
        socklen_t server_len;
        server_len=sizeof(server_socket_addr);

        int opt = 1;
        setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));  //预防端口一直占用
        int bd=bind(lfd,(sockaddr*)&server_socket_addr,server_len);
        if(bd==-1){
            perror("bind");
        }

        //4.开始监听
        int list=listen(lfd,128);  //128指的是最大连接数？
        if(list==-1){
            perror("listen");
        }
        
        // （修改）select模式开始（非阻塞模式）
        fd_set red_set;   //创建文件描述符集合
        FD_ZERO(&red_set);  //初始化
        FD_SET(lfd,&red_set); //把用于监听的套接字放入集合中
        int maxfd=lfd;      //指定 select 存储的最大的套接字下标（上限为 1024）
        
        while(1){
 
            fd_set temp_set=red_set;  //拷贝一份
     
            
            int ret_select=select(maxfd+1,&temp_set,nullptr,nullptr,nullptr);
            if(ret_select==-1){
                perror("select");
                break;
            }
            if(FD_ISSET(lfd,&temp_set)){
             
                int cfd=accept(lfd,nullptr,nullptr);
                if(cfd==-1){
                    perror("accept");
                }

                FD_SET(cfd,&red_set);
                maxfd=cfd>maxfd?cfd:maxfd;   //更新 maxfd
                
      
            }
            for(auto i=0;i<=maxfd;++i){ //遍历 fd_set
                if(i!=lfd && FD_ISSET(i,&temp_set)){ //不是监听 lfd 剩下的就是 cfd
                     // 开始收发数据(拷贝阻塞模式的读写)
                    char buffer[1024] {};
                    //读数据
                    int ret_read=recv(i,buffer,sizeof(buffer),0);  //改用 recv
                    if(ret_read>0){
                        //有数据可读
                        std::cout<<"收到客户端消息："<<buffer<<std::endl;
                    }else if(ret_read==0){
                        //无数据可读
                        std::cout<<"已断开连接！"<<std::endl;
                        //断开连接需要删除集合中的文件描述符
                        FD_CLR(i,&red_set);
                        close(i);
                    }else{  /* -1 */
                        //error
                        perror("read");
                        exit(1);
                    }
                    //发数据
                    std::fill(std::begin(buffer),std::end(buffer),0);  //清空缓冲区
                    std::string msg="服务器确认收到消息:"/*+std::to_string(number++)8*/;  
                    strcpy(buffer,msg.c_str());  //拷贝消息到缓冲区
                    int ret_write=send(i,buffer,strlen(buffer),0);  //非阻塞改用 send
                    if(ret_write==-1){
                        perror("write");
                        break;
                    }
                }
            }
        }
    close(lfd);
    return 0;
    }
    



