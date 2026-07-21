#pragma once
#include "CashMath.h"
#include "CashDebug.h"

#include "Tracy.hpp"

#include <atomic>
#include <mutex>
#include <semaphore>
#include <thread>
#include <vector>

using Mutex = std::mutex;
template<std::ptrdiff_t N>
using Semaphore = std::counting_semaphore<N>;
template<class N>
using Atomic = std::atomic<N>;
template<class N>
using Lock = std::lock_guard<N>;
//creates a lock_guard named "lock"
#define TRACY_LOCK(var) Lock<LockableBase(Mutex)> lock(var)
#define TRACY_MUTEX(var) TracyLockable(Mutex, var)

enum AsyncStatus : u32 {
    AsyncStatus_Empty           = 0,
    AsyncStatus_Fetching        = BIT(0),
    AsyncStatus_FetchedSuccess  = BIT(1),
    AsyncStatus_FetchedFailed   = BIT(2),
    AsyncStatus_Completed       = AsyncStatus_FetchedSuccess | AsyncStatus_FetchedFailed,
};
ENUMOPS_PURE(AsyncStatus);

template<typename T>
struct AsyncData {
    T data;
    TRACY_MUTEX(lock);
    Atomic<AsyncStatus> state = {};
};

struct Job
{
    Atomic<AsyncStatus>* status = nullptr;
    virtual void RunJob() = 0;
};

struct Threading {
private:
    TRACY_MUTEX(m_jobVectorMutex);
    Semaphore<PTRDIFF_MAX>      m_semaphore;
    Atomic<i32>                 m_jobsInFlight = {};
    Atomic<bool>                m_running;
    std::vector<Job*>           m_jobs;
    std::vector<std::thread>    m_threads;

    struct ThreadData {
        std::string name;
        u32 index;
    };

    static i32 ThreadFunction(ThreadData data);
    Threading();
    ~Threading();
    Threading(Threading&) = delete;
    Threading& operator=(Threading&) = delete;
    Job* AcquireJob();

public:
    void ForceQuit();
    static Threading& GetInstance()
    {
        static Threading instance;
        return instance;
    }
    i32 GetJobsInFlight() const
    {
        return m_jobsInFlight;
    }
    void RunAndClearJobs()
    {
        while (true)
        {
            Job* job = AcquireJob();
            if (job == nullptr)
                return;

            m_semaphore.acquire();
            delete job;
            m_jobsInFlight--;
        }
    }
	void SubmitJob(Job* job);
};

bool OnMainThread();

template<class T> inline void operator^=(std::atomic<T>& a, T b)
{
    T at = a.load();
    at = T(+at ^ b);
    a.store(at);
}
template<class T> inline void operator|=(std::atomic<T>& a, T b)
{
    T at = a.load();
    at = T(+at | b);
    a.store(at);
}
template<class T> inline void operator&=(std::atomic<T>& a, T b)
{
    T at = a.load();
    at = T(+at & b);
    a.store(at);
}
void GetNameAndTextForJob(std::string& text, std::string& name, const std::wstring& app, const std::wstring& args);
