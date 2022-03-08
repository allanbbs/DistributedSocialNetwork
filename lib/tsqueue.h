//
// Created by allanbs on 29/12/21.
//

#ifndef PROJ2_TSQUEUE_H
#define PROJ2_TSQUEUE_H

#include <mutex>
#include <deque>
#include <thread>
#include <condition_variable>

template<class T>
class tsqueue {

public:
    tsqueue() = default;

    /*
    ~tsqueue(){
        tsdeque.clear();
    }
*/
    void front_to_back(tsqueue<T>&queue){
        while(!queue.empty())
            this->push_front(queue.pop_back());
    }

    void copy_to(tsqueue<T>&queue){
        for(auto it = tsdeque.begin();it != tsdeque.end();it++)
            queue.push_back(*it);
    }

    void push_front(T element){
        std::scoped_lock lock(mutex);
        tsdeque.push_front(element);
        notify();
    }

    void push_back(T element){
        std::scoped_lock lock(mutex);
        tsdeque.push_back(element);
        notify();
    }

    T pop_back(){
        T t = tsdeque.back();
        std::scoped_lock lock(mutex);
        tsdeque.pop_back();
        return t;
    }

    T pop_front(){
        T t = tsdeque.front();
        std::scoped_lock lock(mutex);
        tsdeque.pop_front();
        return t;
    }

    bool empty(){
        return tsdeque.empty();
    }

    void wait()
    {
        while (empty() && block)
        {
            std::unique_lock<std::mutex> ul(muxBlocking);
            cvBlocking.wait(ul);
        }
    }

    void notify(){
        std::unique_lock<std::mutex> ul(muxBlocking);
        cvBlocking.notify_one();
    }

    void set_block(bool b){
        block = b;
    }

private:
    bool block = true;
    std::mutex mutex;
    std::deque<T> tsdeque;
    std::condition_variable cvBlocking;
    std::mutex muxBlocking;

};


#endif //PROJ2_TSQUEUE_H
