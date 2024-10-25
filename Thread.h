#ifndef THREAD_H
#define THREAD_H
#include <thread>

/**
 * @brief The Thread class 封装线程类
 */
class Thread
{
public:
    Thread() {}
    virtual ~Thread()  //析构函数,释放线程
    {
        if (thread_)
        {
            Thread::Stop();
        }
    }
    int Start() {}
    int Stop()
    {
        abort_ = 1;
        if (thread_)
        {
            thread_->join();    //加入式,子线程会阻塞主线程 另外一种方式是分离式(detach)
            //thread_会阻塞主线程，直到线程结束，
            delete thread_;     //然后释放该线程
            thread_ = nullptr;
        }
        return 0;
    }

    int Stop2()
    {
        abort_ = 1;
        if (thread_)
        {
            thread_->detach();    //分离式进程
            delete thread_;     //然后释放该线程
            thread_ = nullptr;
        }
        return 0;
    }

    virtual void Run() = 0;
protected:
    int abort_ = 0;
    std::thread* thread_ = nullptr;

};

#endif // THREAD_H
