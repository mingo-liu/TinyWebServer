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
#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
class http_conn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,    // 分析请求行
        CHECK_STATE_HEADER,             // 分析请求头部
        CHECK_STATE_CONTENT             // 分析请求体 
    };
    enum HTTP_CODE
    {
        NO_REQUEST,           // 请求不完整, 需要继续读取客户数据
        GET_REQUEST,          // 获得了一个完整的客户请求
        BAD_REQUEST,          // 用户请求有语法错误
        NO_RESOURCE,          // 服务器没有请求的资源？ 
        FORBIDDEN_REQUEST,    // 用户对资源没有足够的访问权限
        FILE_REQUEST,         // 请求文件 
        INTERNAL_ERROR,       // 服务器内部错误
        CLOSED_CONNECTION     // 客户端已经关闭了连接
    };
    enum LINE_STATUS
    {
        LINE_OK = 0,    // 读取一个完整的行
        LINE_BAD,       // 行出错
        LINE_OPEN       // 行数据尚不完整
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in &addr);   // 初始化连接
    void close_conn(bool real_close = true);          // 关闭连接
    void process();
    bool read_once();     // 读取用户数据
    bool write();
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    void initmysql_result(connection_pool *connPool);

private:
    void init();
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    HTTP_CODE parse_request_line(char *text);   // 解析请求行
    HTTP_CODE parse_headers(char *text);        // 解析请求头
    HTTP_CODE parse_content(char *text);        // 解析请求体
    HTTP_CODE do_request();
    char *get_line() { return m_read_buf + m_start_line; };   // 获取HTTP请求下一行的起始地址
    LINE_STATUS parse_line();
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
    static int m_epollfd;         // 内核事件表对应的文件描述符
    static int m_user_count;      // 当前用户的个数
    MYSQL *mysql;

private:
    int m_sockfd;                     // 连接socket
    sockaddr_in m_address;            // 客户端socket地址
    char m_read_buf[READ_BUFFER_SIZE];  // 读缓冲
    int m_read_idx;     // 读缓冲数据部分最后一个字节的下一个位置                   
    int m_checked_idx;  //  指向 m_read_buf 中正在分析的字节
    int m_start_line;   // 当前所解析的行的起始位置
    char m_write_buf[WRITE_BUFFER_SIZE];    // 写缓冲 
    int m_write_idx;     
    CHECK_STATE m_check_state;      // 主状态机当前所处的状态
    METHOD m_method;                // HTTP请求方法
    char m_real_file[FILENAME_LEN];   // 客户请求的目标文件的完整路径
    char *m_url;
    char *m_version;
    char *m_host;
    int m_content_length;
    bool m_linger;
    char *m_file_address;       // 客户请求的文件被mmap到内存中的起始位置
    struct stat m_file_stat;    // 请求文件的状态
    struct iovec m_iv[2];
    int m_iv_count;
    int cgi;        //是否启用的POST
    char *m_string; //存储请求头数据
    int bytes_to_send;
    int bytes_have_send;
};

#endif
