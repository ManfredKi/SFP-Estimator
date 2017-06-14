#ifndef THREADSAVEQUEUE_H
#define THREADSAVEQUEUE_H

#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>

//#define DEBUG

#ifdef DEBUG
#include <iostream>
#endif
template <typename Type>
class ThreadSaveQueue : public std::queue<Type *>
{
  public:

    using std::queue<Type *>::queue;

    ThreadSaveQueue() : std::queue<Type *>(),
                        m_pushCnt(0),
                        m_popCnt(0),
                        m_done(false),
                        m_finish(false),
                        m_lockedUsers(0),
                        m_threashold(0)
    {}

    virtual ~ThreadSaveQueue()    {
#ifdef DEBUG
      std::cout << "++++ ThreadSaveQueue deleted" << std::endl;
#endif
      m_done = true;
      m_finish = true;
      m_waitCondition.notify_all();
    }

    void push_back(Type * dataPtr)
    {
#ifdef DEBUG
      if(m_finish)
        std::cout << "OOOOOOOOOOOOHHHHHNOOOOOO" << std::endl;
#endif

      {
        std::unique_lock<std::mutex> lock(m_mt);
        this->push(dataPtr);
        m_pushCnt++;
      }

      m_waitCondition.notify_one();
    }

    bool pop_front(Type* & dataPtr)
    {
      if(m_done){
        dataPtr = nullptr;
        m_waitCondition.notify_all();
        return false;
      }
      // lock mutex
      std::unique_lock<std::mutex> lock(m_mt);

      while(this->size()<=m_thld){
        auto now = std::chrono::system_clock::now();
        // wait for notify as long as the PacketQueue is empty
        m_waitCondition.wait_until(lock,now+std::chrono::milliseconds(10), [&](){return (m_done || (this->size() > m_thld)); } );
      }

#ifdef DEBUG
      if(m_finish)
        std::cout << "QUEUE pop after finish still " << this->size()-1  << " to go" << std::endl;

      if(m_done)
        std::cout << "QUEUE pop after done will exit" << std::endl;

#endif

      m_thld = 0;

      if(!m_done){
        m_popCnt++;
        dataPtr = this->front();
        this->pop();
      }else{
        dataPtr = nullptr;
        return false;
      }

      if(this->size() == 0){
        m_thld = m_threashold;
        if(m_finish){
          m_done = true;
#ifdef DEBUG
          std::cout << "QUEUE is done after finish" << std::endl;
#endif
          m_waitCondition.notify_all();
        }
      }
      return true;
    }

    inline void setThreashold(uint32_t threashold){
      m_threashold = threashold;
      m_thld = m_threashold;
    }

    inline void reset() {
      close();
      m_done = false;
      m_finish = false;
      m_pushCnt = 0;
      m_popCnt = 0;
    }

    inline uint32_t getPushes() const { return m_pushCnt; }
    inline uint32_t getPops()   const { return m_popCnt;  }

    bool isDone(){return m_done;}

    void signUp(){
      m_lockedUsers++;
    }

    void finish(){
      m_lockedUsers--;
      setThreashold(0);

      if(m_lockedUsers<=0){
        m_finish = true;
#ifdef DEBUG
        std::cout << "QUEUE is set finished" << std::endl;
#endif
      }      
      m_waitCondition.notify_all();
    }

    void wake(){m_waitCondition.notify_all();}

    void close(){
      m_lockedUsers = 0;
      m_done = true;
      setThreashold(0);
      m_waitCondition.notify_all();
    }

  private:
    std::condition_variable m_waitCondition;
    std::atomic<uint32_t> m_pushCnt;
    std::atomic<uint32_t> m_popCnt;
    std::mutex m_mt;
    std::atomic<bool> m_done;
    std::atomic<bool> m_finish;
    std::atomic<int> m_lockedUsers;
    std::atomic<uint32_t> m_threashold;
    uint32_t m_thld;
};

#endif // THREADSAVEQUEUE_H
