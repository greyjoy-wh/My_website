#ifndef THREADPOOL_H
#define THREADPOOL_H
#include<pthread.h>
#include<list>
#include"../lock/locker.h"
#include"../CGImysql/sql_connection_pool.h"

template <typename T>
class threadpool{
private:
    int m_max_thread_number;    //最大线程数目
    int m_max_request_number;   //最大请求数目
    pthread_t *m_pthreads;       //线程数组指针
    std::list<T*> m_request;     //请求队列
    locker m_queuelock;         //请求队列的互斥锁
    sem m_queuestat;            //信号量 指代请求队列的长度
    int m_actor_model;          //模型切换
    connection_pool *m_connPool;//数据库连接池

    static void* worker(void* arg);
    void run();

public:
    threadpool(int model, connection_pool* conn, int thread_number = 8, int request_number = 10000);
    ~threadpool();
    bool append(T* request);
};

template<typename T>
threadpool<T>::threadpool(int model, connection_pool* conn, int thread_number, int request_number):m_actor_model(model),m_max_thread_number(thread_number), m_max_request_number(request_number), m_pthreads(NULL),m_connPool(conn)
 {
    //创建指定数目的线程，并且都执行worker工作。
    //实现线程分离。
    m_pthreads = new pthread_t[thread_number];
    for(int i = 0; i < thread_number; i++){
        pthread_create(&m_pthreads[i], NULL, &worker, this);
        pthread_detach(m_pthreads[i]);
    }
}

template<typename T>
threadpool<T>::~threadpool() {
    //线程池析构只需要将new出来的线程全部delete就行，连接池不需要删除。
    delete[] m_pthreads;
}

template<typename T> 
bool threadpool<T>::append(T* request) {
    //将某个请求加入到请求队列中，由于请求队列是共享的资源？所以要注意线程同步
    m_queuelock.lock();
    if(m_request.size() >= m_max_request_number){
        m_queuelock.unlock();
        return false;
    }
    m_request.push_back(request);    
    m_queuelock.unlock();
    m_queuestat.post();//信号量 + 1 唤醒一个工作线程来处理请求
    return true;
}

template<typename T> 
void* threadpool<T>::worker(void* arg) {
    //每个线程都是去执行run
    threadpool* pool = (threadpool*)arg;
    pool->run();
    return pool;
}

template <typename T>
void threadpool<T>::run(){
    while(true) {
        m_queuestat.wait();
        m_queuelock.lock();
        //从请求队列中取数据，要用锁
        if (m_request.empty())
        {
            m_queuelock.unlock();
            continue;
        }
        T* request = m_request.front();
        m_request.pop_front();
        m_queuelock.unlock();
        if (!request){
            continue;
        }
            connectionRAII mysqlcon(&request->mysql, m_connPool);
            request->process();
    }
}




#endif