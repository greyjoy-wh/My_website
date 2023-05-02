#include<pthread.h>
#include<list>
#include<semaphore.h>

template <class T>
class m_threadpool {
private:
    int max_thread_num;
    int max_request_num;
    std::list<T*> m_requests;
    sem_t sem;
    pthread_mutex_t m_mutex;
    pthread_t* m_phreads;
    void run();


public:
    m_threadpool(int max_thread, int max_request);
    ~m_threadpool();
    bool append(T* request);
    static void* work(void* arg);

};

template <class T>
m_threadpool<T>::m_threadpool(int max_thread, int max_request) {
    max_thread_num = max_thread;
    max_request_num = max_request;
    m_phreads = new pthread_t[max_thread_num];
    for(int i = 0; i < max_thread; i++) {
        pthread_create(&m_phreads[i], NULL, &work, this);
        pthread_detach(&m_phreads[i]);
    }
}

template<class T>
m_threadpool<T>::~m_threadpool() {
    delete[] m_phreads;
    sem_destroy(sem);
    pthread_mutex_destroy(&m_mutex);
}

template<class T> 
void* m_threadpool<T>::work(void* arg){
    m_threadpool* m_pool = (m_threadpool*)arg;
    m_pool-run();
    return NULL;
}

template<class T> 
bool m_threadpool<T>::append(T* request) {
    pthread_mutex_lock(&m_mutex);
    if(m_requests.size() == max_request_num) {
        pthread_mutex_unlock(&m_mutex);
        return false;
    }
    m_requests.push_back(request);
    sem_post(&sem);
    pthread_mutex_unlock(&m_mutex);
    return true;                      
}

template<class T> 
void m_threadpool<T>::run(){
    while(true){
        sem_wait(&sem);
        pthread_mutex_lock(&m_mutex);
        if(m_requests.empty()) {
            pthread_mutex_unlock(&m_mutex);
            continue;;
        }
        T* request = m_requests.front();
        m_requests.pop_front();
        pthread_mutex_unlock(&m_mutex);
        request->process();
    }
}
