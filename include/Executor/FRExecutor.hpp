//
// Created by SlepiK on 09.06.24.
//

#ifndef SENSOR_EXECUTOR_FREXECUTOR_HPP
#define SENSOR_EXECUTOR_FREXECUTOR_HPP

#include <Core/Executor/IExecutor.hpp>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

namespace Sensor::Executor {
    class FRExecutor : public Energyleaf::Stream::V1::Core::Executor::IExecutor {
    public:
        FRExecutor() : FRExecutor(2) {
        }

        //Use with care, as this implementation needs a stack size already set. In this project the stl based implementation is used.
        explicit FRExecutor(std::size_t numThreads) : numThreads(numThreads), numTasksInProgress(0) {
            this->queueMutex = xSemaphoreCreateMutex();
            this->taskSemaphore = xSemaphoreCreateCounting(this->numThreads, 0);

            for (std::size_t i = 0; i < this->numThreads; ++i) {
                TaskHandle_t handle;
                xTaskCreate(&FRExecutor::taskRunner, "energyleaf_runner", 4096, this, 1, &handle);
                this->taskHandles.push_back(handle);
            }
        }

        ~FRExecutor() override {
            shutdown();
            for (auto handle : this->taskHandles) {
                vTaskDelete(handle);
            }
            vSemaphoreDelete(this->queueMutex);
            vSemaphoreDelete(this->taskSemaphore);
        }

        void addTask(std::function<void()> task) override {
            {
                std::unique_lock<std::mutex> lock(this->mutex);
                taskQueue.push(std::move(task));
                ++this->numTasksInProgress;
            }
            xSemaphoreGive(this->taskSemaphore);
        }

        void joinTasks() override {
            std::unique_lock<std::mutex> lock(this->mutex);
            cv.wait(lock, [this]() { return this->numTasksInProgress == 0; });
        }

        void shutdown() override {
            {
                std::unique_lock<std::mutex> lock(this->mutex);
                isShuttingDown = true;
            }
            for (size_t i = 0; i < this->numThreads; ++i) {
                xSemaphoreGive(taskSemaphore);
            }
            joinTasks();
        }
    private:
        std::size_t numThreads;
        std::vector<std::function<void()>> tasks;
        std::atomic<int> numTasksInProgress;
        std::mutex mutex;
        std::condition_variable cv;
        SemaphoreHandle_t queueMutex;
        SemaphoreHandle_t taskSemaphore;
        bool isShuttingDown;
        std::queue<std::function<void()>> taskQueue;
        std::vector<TaskHandle_t> taskHandles;

        static void taskRunner(void* param) {
            auto* executor = static_cast<FRExecutor*>(param);
            while (true) {
                xSemaphoreTake(executor->taskSemaphore, portMAX_DELAY);
                {
                    std::unique_lock<std::mutex> lock(executor->mutex);
                    if (executor->isShuttingDown && executor->taskQueue.empty()) {
                        break;
                    }
                }
                executor->runTask();
            }
        }

        void runTask() {
            std::function<void()> task;
            {
                xSemaphoreTake(queueMutex, portMAX_DELAY);
                if (!taskQueue.empty()) {
                    task = std::move(taskQueue.front());
                    taskQueue.pop();
                }
                xSemaphoreGive(queueMutex);
            }
            if (task) {
                task();
                {
                    std::unique_lock<std::mutex> lock(this->mutex);
                    if (--this->numTasksInProgress == 0) {
                        cv.notify_all();
                    }
                }
            }
        }
    };
}

#endif //SENSOR_EXECUTOR_FREXECUTOR_HPP
