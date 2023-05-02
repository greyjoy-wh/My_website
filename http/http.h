#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>

#include "../CGImysql/sql_connection_pool.h"
#include "../lock/locker.h"
#include "../log/log.h"



class http_conn {
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    //请求行方法
    enum METHOD {
        GET = 0,
        POST
    };
    //主状态机状态
    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    //报文解析结果
    enum HTTP_CODE {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    //从状态机状态
    enum LINE_STATUS {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };
    static int m_epollfd;   //静态，所有对象共用一个epollfd
    static int m_user_count;
    MYSQL *mysql;
    int m_state; //读写状态

private:
    int m_sockfd; //本次连接产生的的fd
    sockaddr_in m_address;
    char m_read_buf[READ_BUFFER_SIZE];
    long m_read_idx;    //64位操作系统 long代表的是8个字节 读取到的字节数
    long m_checked_idx; //解析到的字节数
    int m_start_line;
    char m_write_buf[WRITE_BUFFER_SIZE];
    long m_write_idx;
    CHECK_STATE  m_check_state;
    METHOD m_method;
    //下面六个是请求报文的6个变量储存点
    char m_real_file[FILENAME_LEN];
    char *m_url;
    char *m_version;
    char *m_host;
    long m_content_length;
    bool m_linger;

    char *m_file_address;       //读取服务器上的文件地址
    struct stat m_file_stat;    //io向量机制iovec
    struct iovec m_iv[2];
    int m_iv_count;
    int cgi;                    //是否启用的POST
    char *m_string;             //存储请求头数据
    int bytes_to_send;          //剩余发送字节数
    int bytes_have_send;        //已发送字节数
    char *doc_root; 

    map<string, string> m_users;
    int m_TRIGMode;
    int m_close_log;

    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];

public:
    http_conn() {}
    ~http_conn() {}
    void init(int sockfd, const sockaddr_in &addr, char*, int, int, string user, string passwd, string sqlname);
    void close_conn(bool real_close = true);
    void process();
    bool read_once();
    bool write();
    sockaddr_in *get_addr(){
        return &m_address;
    }

    void initmysql_result(connection_pool *pool);
    int timer_flag;         //开启定时器？
    int improv;             //??

private:
    void init();
    HTTP_CODE process_read();  //读取并处理请求报文，返回报文解析结果
    bool process_write(HTTP_CODE ret);  //写入响应报文数据
    HTTP_CODE parse_request_line(char *text);   //解析请求行
    HTTP_CODE parse_headers(char *text);        //解析请求头
    HTTP_CODE parse_content(char *text);        //解析请求内容
    HTTP_CODE do_request();                     //生成响应报文
    
    char *get_line() {return m_read_buf + m_start_line;}
    LINE_STATUS parse_line();
    void unmap();
    //根据响应报文格式，生成对应的8个部分，以下函数均由do_request调用
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

};

#endif
