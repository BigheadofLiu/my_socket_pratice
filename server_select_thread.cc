/**
 *  select模式的 socket server客户端
 *  改自 server.cpp
 *  v1.0 单线程版本
 *  v2.0 多线程并发（存在的问题：数据共享保护、lfd 的处理、accept 放主线程即可
 *  v3.0 accept放主线程
 *  注意加锁的范围 用{}来限制
 */
#include<iostream>
#include<cstring>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<unistd.h> 
#include<sys/select.h> //using select
#include<thread> //using thread
#include<memory> //using make_share
#include<mutex>  //using mutex

std::mutex g_mutex;

struct fd_info{
    int fd;    
    fd_set& red_set; 
};


void comm_thread(const std::shared_ptr<fd_info> info){
    std::cout<<" 通信子线程开始执行，线程 id："<<std::this_thread::get_id()<<std::endl;
    // 开始收发数据(拷贝阻塞模式的读写)
    char buffer[1024] {0};  //读写缓冲区
    bool need_close {false}; // 判断cfd是否需要关闭

    //读数据
    int ret_read=recv(info->fd,buffer,sizeof(buffer),0);  //改用 recv
    if(ret_read>0){
        //有数据可读
        std::cout<<"收到客户端消息："<<buffer<<std::endl;
        //同时发送消息
        std::string msg="服务器确认收到消息:";
        msg+=buffer;
        int ret_send=send(info->fd,msg.c_str(),msg.length(),0);
        if(ret_send==-1){
            perror("send");
            need_close=true;
        }
    }else if(ret_read==0){
        //无数据可读
        std::cout<<"已断开连接！"<<std::endl;
        need_close=true;
    }else{  /* -1 */
        //error
        perror("recv");
        need_close=true;
    }
    if(need_close){
        //需要关闭的情况
        std::lock_guard<std::mutex> locker(g_mutex);
        FD_CLR(info->fd,&info->red_set);
        close(info->fd);
    }else{
         std::lock_guard<std::mutex> locker(g_mutex);
        //放回集合
         FD_SET(info->fd,&info->red_set);
    }
    }


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
        int list=listen(lfd,128);  
        if(list==-1){
            perror("listen");
        }
        
        fd_set red_set;   //创建文件描述符集合
        FD_ZERO(&red_set);  //初始化
        FD_SET(lfd,&red_set); //把用于监听的套接字放入集合中
        int maxfd=lfd;      //指定 select 存储的最大的套接字下标（上限为 1024）
        
        while(1){
            // std::lock_guard<std::mutex> locker(g_mutex)   锁的范围太大 导致死锁
            fd_set temp_set;
            {
                std::lock_guard<std::mutex> locker(g_mutex);
                temp_set=red_set;
            }
            int ret_select=select(maxfd+1,&temp_set,nullptr,nullptr,nullptr);
            if(ret_select==-1){
                perror("select");
                break;
            }
            if(FD_ISSET(lfd,&temp_set)){
                int cfd=accept(lfd,nullptr,nullptr);
                if(cfd==-1){
                    perror("accept");
                }else{
                std::lock_guard<std::mutex> locker(g_mutex);
                FD_SET(cfd,&red_set);
                maxfd=cfd>maxfd?cfd:maxfd;   //更新 maxfd
                }
            }
            for(auto i=0;i<=maxfd;++i){ //遍历 fd_set
                if(i!=lfd && FD_ISSET(i,&temp_set)){ //不是监听 lfd 剩下的就是 cfd
                    bool should_handle=false;
                    {   //锁的生效范围
                        std::lock_guard<std::mutex> locker(g_mutex);
                        if(FD_ISSET(i,&temp_set)){
                            FD_CLR(i,&temp_set);   //先移除 交给子线程处理
                            should_handle=true;
                        }
                    }
                    if(should_handle){
                        std::shared_ptr<fd_info> info(new fd_info{i,red_set}); 
                        std::thread t2(comm_thread,info);
                        t2.detach();
                    }
            }
        }
    }
    close(lfd);
    return 0;
}


