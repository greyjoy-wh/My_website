#include<list>
#include"lock/locker.h"
#include<mysql/mysql.h>
#include<list>
#include<string>
using namespace std;

class connection_pool {
public:
    static connection_pool* getInstence() {
        static connection_pool m_con_pool;
        return &m_con_pool;
    }
    void init(int max_con, string url, int port, string user, string password, string database);
    MYSQL* getConnection();
    bool   relConnection(MYSQL* con);
	std::string m_url;			 //主机地址
	std::string m_Port;		 //数据库端口号
	std::string m_User;		 //登陆数据库用户名
	std::string m_PassWord;	 //登陆数据库密码
	std::string m_DatabaseName; //使用数据库名

private:
    std::list<MYSQL*> con_list;
    sem m_sem;
    locker m_locker;
    int free_connection_num;
    int max_connection_num;
    connection_pool();
    connection_pool(const connection_pool&) ;
    connection_pool& operator=(const connection_pool&) = delete;
    ~connection_pool();

};

void connection_pool::init(int max_con, string url, int port, string user, string password, string database) {
    m_url = url;
    m_User = user;
    m_PassWord = password;
    m_DatabaseName = database;
    max_connection_num = max_con;
    m_sem = sem(max_con);
    for(int i = 0; i<max_con; i++) {
        //创建对应的MYSQL 加入到list中
        free_connection_num++;
    }
}

MYSQL* connection_pool::getConnection() {

    m_sem.wait();
    m_locker.lock();
    MYSQL* con;
    con = con_list.front();
    con_list.pop_front();
    free_connection_num--;
    m_locker.unlock();
    return con;
}

bool connection_pool::relConnection(MYSQL* con) {
    m_locker.lock();
    con_list.push_back(con);
    m_sem.post();
    free_connection_num++;
    return true;
}

connection_pool::~connection_pool() {
    m_locker.lock();
    for(int i = 0; i<con_list.size(); i++) {
        //断开对应的连接
    }
    m_locker.unlock();
}


class connectionRALL {
public:
    connectionRALL(MYSQL** con, connection_pool* con_pool);
    ~connectionRALL();
private:
    MYSQL* m_con;
    connection_pool* m_con_pool;
};

connectionRALL::connectionRALL(MYSQL** con, connection_pool* con_pool) {
    *con = con_pool->getConnection();
    m_con = *con;
    con_pool = con_pool;
}

connectionRALL::~connectionRALL() {
    m_con_pool->relConnection(m_con);
}

