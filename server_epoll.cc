#include<iostream>
#include<cstring>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<unistd.h> 
#include<sys/epoll.h> //using epoll
#include<vector>      //using vector
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
        //创建 epoll实例
        int epoll_fd=epoll_create(1);
        if(epoll_fd==-1){
            perror("epoll_create");
        }
        //创建epoll_event 存储 lfd
        epoll_event ev;
        ev.events=EPOLLIN;
        ev.data.fd=lfd;
        //放入epoll 树
        auto ctl_ret=epoll_ctl(epoll_fd,EPOLL_CTL_ADD,lfd,&ev);
        if(ctl_ret==-1){
            perror("epoll_ctl");
        }
        //创建 vector 容器 用于接收就绪事件
        std::vector<epoll_event> events(1024); //epoll 需要提前分配空间
        while (1)
        {  //阻塞检测 如果检测到就绪事件放入容器中
           int ret=epoll_wait(epoll_fd,events.data(),events.size(),-1);
           //error
           if(ret==-1){
            perror("epoll_wait");
            break;
           }
           //ret>0 返回的值为就绪的事件个数  遍历 events
           for (auto i = 0; i < ret; ++i)     //开始遍历 vector
           {    
                auto temp_fd=events[i].data.fd;  //取出 fd 判断是 lfd 还是 cfd 
                if(temp_fd==lfd){
                    //如果是 lfd 就做 accept
                    //accept
                    int cfd=accept(lfd,nullptr,nullptr);
                    if(cfd==-1){
                        perror("accept");
                        continue;
                    }
                    //cfd上树
                    epoll_event temp_ev;
                    temp_ev.data.fd=cfd;
                    temp_ev.events=EPOLLIN;
                    int temp_ctl_ret=epoll_ctl(epoll_fd,EPOLL_CTL_ADD,cfd,&temp_ev);
                    if(temp_ctl_ret==-1){
                        perror("epoll_ctl");
                    }
                }else{
                    //cfd处理
                    //判断是不是读事件
                    if(events[i].events & EPOLLIN){
                        char buffer[1024] {0};
                        int recv_ret=recv(temp_fd,buffer,sizeof(buffer),0);
                        if(recv_ret>0){
                            //缓冲区有数据
                            std::cout<<"收到客户端（fd="<<temp_fd<<")发来的消息："<<buffer<<std::endl;
                            std::string msg="服务器确认收到消息:";
                            msg+=buffer;
                            int send_ret=send(temp_fd,msg.c_str(),msg.length(),0);
                            if(send_ret==-1){
                                perror("send");
                            }
                        }else if(recv_ret==0){
                            //缓冲区无数据
                            std::cout<<"客户端（fd="<<temp_fd<<")已断开连接"<<std::endl;
                            // close(events[i].data.fd);  //cfd清除之前需要先关闭
                            // //容器中删除
                            // events[i]=events.back();
                            // events.pop_back();
                            // i--;
                            //这里不需要手动管理 交给epoll自己处理
                            epoll_ctl(epoll_fd,EPOLL_CTL_DEL,temp_fd,nullptr);
                            close(temp_fd);
                        }else{
                            /*-1*/
                            //返回值-1 出错
                            perror("recv");
                            // close(events[i].data.fd);  //cfd清除之前需要先关闭
                            // //容器中删除
                            // events[i]=events.back();
                            // events.pop_back();
                            epoll_ctl(epoll_fd,EPOLL_CTL_DEL,temp_fd,nullptr);
                            close(temp_fd);
                        }
                    }                  
                }
           }
        }
    }