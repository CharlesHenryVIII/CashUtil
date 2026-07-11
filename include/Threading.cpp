#include "Threading.h"
#include "Math.h"
#include "System.h"

#include "Tracy.hpp"

#include <Chrono>

#define ASYNC

Threading::Threading()
    : m_semaphore(0)
{
#ifdef ASYNC
    u32 usableCores = (u32)Max<i32>(1, g_sysinfo.threads - 1);
#else
    u32 usableCores = 1;
#endif
    for (u32 i = 0; i < usableCores; ++i)
    {
        ThreadData data = {
            .name = ToString("Thread: %i", i),
            .index = i
        };
        m_threads.push_back(std::thread(&Threading::ThreadFunction, data));
    }
    m_running = true;
}
Threading::~Threading()
{
    m_running = false;
    m_semaphore.release(m_threads.size());

    for (std::thread& thread : m_threads)
        if (thread.joinable())
            thread.join();
}

[[nodiscard]] Job* Threading::AcquireJob()
{
    ZoneScopedN("Acquire Job");
    TRACY_LOCK(m_jobVectorMutex);
    if (m_jobs.empty())
        return nullptr;
    m_jobsInFlight++;

    Job* job = m_jobs[0];
    m_jobs.erase(m_jobs.begin());
    return job;
}

void Threading::SubmitJob(Job* job)
{
    ZoneScopedN("Submit Job");
    TRACY_LOCK(m_jobVectorMutex);
    m_jobs.push_back(job);
    m_semaphore.release();
}

i32 Threading::ThreadFunction(ThreadData data)
{
    Threading& MT = GetInstance();
    tracy::SetThreadName(data.name.c_str());

    while (true)
    {
        MT.m_semaphore.acquire();
        if (!MT.m_running)
            break;

        Job* job = MT.AcquireJob();
        ASSERT(job);
        if (job == nullptr)
            continue;

        if (job->status)
        {
            ASSERT(*job->status == AsyncStatus_Empty);
            *job->status = AsyncStatus_Fetching;
        }
        job->RunJob();
        if (job->status)
        {
            ASSERT(*job->status == AsyncStatus_Fetching);
            *job->status = AsyncStatus_FetchedSuccess;
        }

        MT.m_jobsInFlight--;
        delete job;
    }
    return 0;
}

void Threading::ForceQuit()
{
    for (auto& t : m_threads)
    {
        t.detach();
        t.~thread();
    }
}

std::thread::id mainThreadID = std::this_thread::get_id();
bool OnMainThread()
{
    return mainThreadID == std::this_thread::get_id();
}

void GetNameAndTextForJob(std::string& text, std::string& name, const std::wstring& app, const std::wstring& args)
{
    std::wstring namew;
    std::wstring textw;
    if (!app.size())
    {
       size_t p = args.find_first_of(L' ', 1);
       namew = args.substr(0, p);
       textw = args;
    }
    else
    {
        namew = app;
        if (args.size())
            textw = app + L" " + args;
    }
    if (namew.size())
        SysConvertWideCharToMultiByte(name, namew);
    else
        name.clear();
    if (textw.size())
        SysConvertWideCharToMultiByte(text, textw);
    else
        text.clear();
}
