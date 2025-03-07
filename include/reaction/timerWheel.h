#include <iostream>
#include <vector>
#include <functional>
#include <chrono>
#include <thread>
#include <list>
#include <atomic>
#include <unordered_map>
#include <mutex>

class HierarchicalTimerWheel
{
public:
    using Task = std::function<void()>;
    using TimePoint = std::chrono::steady_clock::time_point;
    using TaskID = uint64_t;

    static HierarchicalTimerWheel &getInstance()
    {
        static HierarchicalTimerWheel instance;
        return instance;
    }

    ~HierarchicalTimerWheel()
    {
        stop();
    }

    void start()
    {
        running_ = true;
        worker_ = std::thread(&HierarchicalTimerWheel::run, this);
    }

    void stop()
    {
        running_ = false;
        if (worker_.joinable())
        {
            worker_.join();
        }
    }

    TaskID addTask(std::chrono::milliseconds interval, Task task)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto taskID = nextTaskID_++;
        auto now = std::chrono::steady_clock::now();
        auto triggerTime = now + interval;
        size_t slot = (currentMs_ + interval.count()) % 1000;
        msWheel_[slot].emplace_back(taskID, triggerTime, interval, task);
        taskMap_[taskID] = &msWheel_[slot].back(); // 保存任务指针
        return taskID;
    }

    void removeTask(TaskID taskID)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = taskMap_.find(taskID);
        if (it != taskMap_.end())
        {
            it->second->valid = false; // 标记任务为无效
            taskMap_.erase(it);        // 从任务映射中移除
        }
    }

private:
    HierarchicalTimerWheel()
        : running_(false),
          currentMs_(0),
          currentSec_(0),
          currentMin_(0),
          currentHour_(0),
          nextTaskID_(1)
    { // 任务 ID 从 1 开始
        // 初始化时间轮
        msWheel_.resize(1000); // 毫秒级：1000 个槽位，每槽 1ms
        secWheel_.resize(60);  // 秒级：60 个槽位，每槽 1s
        minWheel_.resize(60);  // 分钟级：60 个槽位，每槽 1min
        hourWheel_.resize(24); // 小时级：24 个槽位，每槽 1h
    }

    struct TimerTask
    {
        TaskID id;
        TimePoint triggerTime;
        std::chrono::milliseconds interval;
        Task task;
        bool valid = true; // 任务是否有效
    };

    void run()
    {
        while (running_)
        {
            auto now = std::chrono::steady_clock::now();
            processLevel(msWheel_, currentMs_, std::chrono::milliseconds(1));
            if (currentMs_ == 0)
            {
                processLevel(secWheel_, currentSec_, std::chrono::seconds(1));
                if (currentSec_ == 0)
                {
                    processLevel(minWheel_, currentMin_, std::chrono::minutes(1));
                    if (currentMin_ == 0)
                    {
                        processLevel(hourWheel_, currentHour_, std::chrono::hours(1));
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            currentMs_ = (currentMs_ + 1) % 1000;
        }
    }

    void processLevel(std::vector<std::list<TimerTask>> &wheel, size_t &currentSlot, auto interval)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto &tasks = wheel[currentSlot];
        for (auto it = tasks.begin(); it != tasks.end();)
        {
            if (!it->valid)
            {
                // 如果任务无效，直接删除
                it = tasks.erase(it);
            }
            else if (std::chrono::steady_clock::now() >= it->triggerTime)
            {
                it->task(); // 触发任务

                // 重新插入任务
                auto nextTriggerTime = it->triggerTime + it->interval;
                size_t slot = (currentMs_ + it->interval.count()) % 1000;
                msWheel_[slot].emplace_back(it->id, nextTriggerTime, it->interval, it->task);
                taskMap_[it->id] = &msWheel_[slot].back(); // 更新任务指针

                it = tasks.erase(it); // 删除已触发的任务
            }
            else
            {
                ++it;
            }
        }
        currentSlot = (currentSlot + 1) % wheel.size();
    }

    std::vector<std::list<TimerTask>> msWheel_;   // 毫秒级时间轮
    std::vector<std::list<TimerTask>> secWheel_;  // 秒级时间轮
    std::vector<std::list<TimerTask>> minWheel_;  // 分钟级时间轮
    std::vector<std::list<TimerTask>> hourWheel_; // 小时级时间轮

    size_t currentMs_;
    size_t currentSec_;
    size_t currentMin_;
    size_t currentHour_;

    std::unordered_map<TaskID, TimerTask *> taskMap_; // 任务 ID 到任务指针的映射
    std::atomic<TaskID> nextTaskID_;                  // 下一个任务 ID
    std::atomic<bool> running_;
    std::thread worker_;
    std::mutex mutex_; // 用于线程安全
};