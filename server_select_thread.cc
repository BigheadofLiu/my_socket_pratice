/**
 *  select模式的 socket server客户端
 *  改自 server.cpp
 *  v1.0 单线程版本
 *  v2.0 多线程并发（存在的问题：数据共享保护、lfd 的处理、） accept 放主线程即可
 *  v3.0 accept放主线程
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
#include<mutex> 
#include<atomic>

// std::mutex maxfd_metux;
std::mutex g_mutex;

struct fd_info{
    int fd;     //只剩一个 fd 其实没必要使用结构体了。。
    // int& max_fd;  //需要修改（传入传出-用指针） 指针太麻烦 改引用了
    fd_set& red_set; //需要修改（传入传出-用指针）
};

// void accept_thread(/*fd_info* info*/const std::shared_ptr<fd_info> info){
//     std::cout<<" accept子线程开始执行，线程 id："<<std::this_thread::get_id()<<std::endl;
//     int cfd=accept(info->fd,nullptr,nullptr);
//     if(cfd==-1){
//         perror("accept");
//         return;
//     }
//     retset_mutex.lock();
//     // std::lock_guard<std::mutex> locker(retset_mutex);
//     FD_SET(cfd,&info->red_set);
//     retset_mutex.unlock();

//     maxfd_metux.lock();
//     info->max_fd=cfd>info->max_fd?cfd:info->max_fd;   //更新 maxfd
//     maxfd_metux.unlock();
// }

void comm_thread(/*fd_info* info*/const std::shared_ptr<fd_info> info){
    std::cout<<" 通信子线程开始执行，线程 id："<<std::this_thread::get_id()<<std::endl;
    // 开始收发数据(拷贝阻塞模式的读写)
    char buffer[1024] {0};  //读写缓冲区
    bool need_close {false}; // 判断cfd是否需要关闭

    //读数据
    // int ret_read=read(i,buffer,sizeof(buffer));
    int ret_read=recv(info->fd,buffer,sizeof(buffer),0);  //改用 recv
    if(ret_read>0){
        //有数据可读
        std::cout<<"收到客户端消息："<<buffer<<std::endl;

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
        //断开连接需要删除集合中的文件描述符
        // retset_mutex.lock();
        // FD_CLR(info->fd,&info->red_set);
        // retset_mutex.unlock();
        // close(info->fd);
        // break;  不应该使用跳出循环（多客户端情况下） 应该进行下次检测
        // continue;
        // return; //退出子线程
    }else{  /* -1 */
        //error
        perror("recv");
        need_close=true;
        // exit(1);
    }
    if(need_close){
        //需要关闭的情况
        std::lock_guard<std::mutex> locker(g_mutex);
        // retset_mutex.lock();
        FD_CLR(info->fd,&info->red_set);
        // retset_mutex.unlock();
        close(info->fd);
    }else{
        //继续保留
         std::lock_guard<std::mutex> locker(g_mutex);
         FD_SET(info->fd,&info->red_set);
    }
    //发数据
    // std::fill(std::begin(buffer),std::end(buffer),0);  //清空缓冲区
    // std::string msg="服务器确认收到消息:"/*+std::to_string(number++)8*/;  
    // strcpy(buffer,msg.c_str());  //拷贝消息到缓冲区
    // int ret_write=write(i,buffer,strlen(buffer));
    // int ret_write=send(info->fd,buffer,strlen(buffer),0);  //非阻塞改用 send
    // if(ret_write==-1){
    //     perror("write");
        // break;
    }
    // sleep(1); //避免发送频繁看不到 阻塞模式 可以用 非阻塞不要用
    // return;
// }

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
            // retset_mutex.lock();
            std::lock_guard<std::mutex> locker(g_mutex);
            fd_set temp_set=red_set;  //拷贝一份
            // retset_mutex.unlock();
            
            int ret_select=select(maxfd+1,&temp_set,nullptr,nullptr,nullptr);
            if(ret_select==-1){
                perror("select");
                break;
            }
            if(FD_ISSET(lfd,&temp_set)){
             
                int cfd=accept(lfd,nullptr,nullptr);
                if(cfd==-1){
                    perror("accept");
                    // continue;
                }
                std::lock_guard<std::mutex> locker(g_mutex);
                FD_SET(cfd,&red_set);
                maxfd=cfd>maxfd?cfd:maxfd;   //更新 maxfd
                
                // 放进子线程处理
                // fd_info info={lfd,maxfd,red_set};  //注意变量生存周期 线程间调用必须使用堆区内存
                // fd_info* info=new fd_info{lfd,maxfd,red_set}; //不想手动管理内存？ 
                // std::shared_ptr<fd_info> info(new fd_info{lfd,maxfd,red_set});
                // std::thread t1(accept_thread,info);
                // t1.detach();
            }
            for(auto i=0;i<=maxfd;++i){ //遍历 fd_set
                if(i!=lfd && FD_ISSET(i,&temp_set)){ //不是监听 lfd 剩下的就是 cfd
                    bool should_handle=false;
                    std::lock_guard<std::mutex> locker(g_mutex);
                    if(FD_ISSET(i,&temp_set)){
                        FD_CLR(i,&temp_set);   //先移除 交给子线程处理
                        should_handle=true;
                    }
                    if(should_handle){
                        std::shared_ptr<fd_info> info(new fd_info{i,red_set});   //maxfd可以不传 但是定义的引用
                        std::thread t2(comm_thread,info);
                        t2.detach();
                    }
                //     // 收发数据放入子线程
                //     // 开始收发数据(拷贝阻塞模式的读写)
                //     char buffer[1024] {};

                //     //读数据
                //     // int ret_read=read(i,buffer,sizeof(buffer));
                //     int ret_read=recv(i,buffer,sizeof(buffer),0);  //改用 recv
                //     if(ret_read>0){
                //         //有数据可读
                //         std::cout<<"收到客户端消息："<<buffer<<std::endl;
                //     }else if(ret_read==0){
                //         //无数据可读
                //         std::cout<<"已断开连接！"<<std::endl;
                //         //断开连接需要删除集合中的文件描述符
                //         FD_CLR(i,&red_set);
                //         close(i);
                //         // break;  不应该使用跳出循环（多客户端情况下） 应该进行下次检测
                //         continue;
                //     }else{  /* -1 */
                //         //error
                //         perror("read");
                //         exit(1);
                //     }
                //     //发数据
                //     std::fill(std::begin(buffer),std::end(buffer),0);  //清空缓冲区
                //     std::string msg="服务器确认收到消息:"/*+std::to_string(number++)8*/;  
                //     strcpy(buffer,msg.c_str());  //拷贝消息到缓冲区
                //     // int ret_write=write(i,buffer,strlen(buffer));
                //     int ret_write=send(i,buffer,strlen(buffer),0);  //非阻塞改用 send
                //     if(ret_write==-1){
                //         perror("write");
                //         break;
                //     }
                //     // sleep(1); //避免发送频繁看不到 阻塞模式 可以用 非阻塞不要用
                // }
            }

        // sockaddr_in client_socket_addr; //用于存储监听到客户端的地址信息
        // socklen_t client_len=sizeof(client_socket_addr);  //socket 地址长度信息 用于参数传入
        // int cfd=accept(lfd,(sockaddr*)&client_socket_addr,&client_len);  //accept用于获取用于通信的套接字
        // if(cfd==-1){
        //     perror("accept");
        // }
        // char ip[32]{0};
        // std::cout<<"监听到的客户端";
        // std::cout<<"打印监听到的客户端信息："<<std::endl;
        // std::cout<<"端口："<<ntohs(client_socket_addr.sin_port)<<std::endl;
        // std::cout<<"IP:"<<inet_ntop(AF_INET,&client_socket_addr.sin_addr.s_addr,ip,sizeof(ip));

        // //5.（阻塞模式此步骤开始传输数据）
        // int number {0};
        // while (1)//循环收发收据
        // {   char buffer[1024] {};

        //     //读数据
        //     int ret_read=read(cfd,buffer,sizeof(buffer));
        //         if(ret_read>0){
        //             //有数据可读
        //             std::cout<<"收到客户端消息："<<buffer<<std::endl;
        //         }else if(ret_read==0){
        //             //无数据可读
        //             std::cout<<"已断开连接！"<<std::endl;
        //             break;
        //         }else{
        //             //error
        //             perror("read");
        //             break;
        //         }
        //     //发数据
        //     std::fill(std::begin(buffer),std::end(buffer),0);  //清空缓冲区
        //     auto msg="服务器确认收到消息:"+std::to_string(number++);  
        //     strcpy(buffer,msg.c_str());  //拷贝消息到缓冲区
        //     int ret_write=write(cfd,buffer,strlen(buffer));
        //     if(ret_write==-1){
        //         perror("write");
        //         break;
        //     }
        //     sleep(1); //避免发送频繁看不到
        // }
        // close(cfd);
        }

    }
    close(lfd);
    return 0;
}


