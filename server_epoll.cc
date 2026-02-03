#include<iostream>
#include<cstring>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<unistd.h> 
#include<poll.h> //using poll
#include<vector> //using vector

//epoll模式 改自 poll

int main(){
        //1.创建用于监听的套接字
        int lfd=socket(AF_INET,SOCK_STREAM,0);
        if(lfd==-1){
            perror("socket");
        }

        //2.创建用于socket通信的地址
        sockaddr_in server_socket_addr{}; 
        server_socket_addr.sin_family=AF_INET;  //协议
        server_socket_addr.sin_port=htons(10000); //端口
        // inet_pton(AF_INET,"172.16.172.129",&server_socket_addr.sin_addr.s_addr); //ip
        server_socket_addr.sin_addr.s_addr=INADDR_ANY;
        
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

        std::vector<pollfd> fds;  //使用 vector替换fd_set  数据不支持动态扩展：pollfd fds[1024];

        pollfd pfd;
        pfd.fd=lfd;
        pfd.events=POLLIN;   //关注什么事件
        pfd.revents=0;       //实际发生了什么（内核修改）
        fds.push_back(pfd);  //放入vector

        while (1)
        {
           int ret=poll(fds.data(),fds.size(),-1);  

           if(ret==-1){
            perror("poll");
            break;
           }
           for (size_t i = 0; i < fds.size(); ++i)     //开始遍历 vector
           {
            if (fds[i].revents & POLLIN)
            {
                if(fds[i].fd==lfd){
                    int cfd=accept(lfd,nullptr,nullptr);
                    if(cfd== -1){
                        perror("accept");
                    }
                    pollfd new_pfd;
                    new_pfd.fd=cfd;
                    new_pfd.events=POLLIN;
                    new_pfd.revents=0;
                    fds.push_back(new_pfd);

                }else{
                    char buffer[1024] {0};
                    int recv_ret=recv(fds[i].fd,buffer,sizeof(buffer),0);
                    if(recv_ret>0){
                        std::cout<<"收到客户端（fd="<<fds[i].fd<<")发来的消息："<<buffer<<std::endl;

                        std::string msg="服务器确认收到消息:";
                        msg+=buffer;
                        int send_ret=send(fds[i].fd,msg.c_str(),msg.length(),0);
                        if(send_ret==-1){
                            perror("send");
                        }
                    }else if(recv_ret==0){
                        std::cout<<"客户端（fd="<<fds[i].fd<<")已断开连接"<<std::endl;
                        close(fds[i].fd);  //cfd清除之前需要先关闭
                        fds[i]=fds.back();
                        fds.pop_back();
                        i--;
                    }else{
                        /*-1*/
                        perror("recv");
                        close(fds[i].fd);
                        fds[i]=fds.back();
                        fds.pop_back();
                    }
                }
            }
           }
        }
    }