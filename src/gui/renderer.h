#pragma once
#ifndef __GLEW_H__
#include "GL/glew.h"
#endif
#include <thread>
#include "SFML/Graphics.hpp"
#include <iostream>
#include "../logger/logger.h"
#include <cmath>
#include <vector>
#include "objects.h"
#include <mutex>
#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>
#include "shader.h"
#include "characterManager.h"
#include <atomic>
#include <queue>
#include <functional>
#include <condition_variable>
#include <latch>

class TaskQueue {
public:
    void push(std::function<void()> task) {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push(std::move(task));
        condition_.notify_one();
    }

    std::function<void()> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !tasks_.empty(); });
        auto task = std::move(tasks_.front());
        tasks_.pop();
        return task;
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mutex_);
        return tasks_.size();
    }

    std::queue<std::function<void()>> getTasks() {
        return tasks_;
    }

private:
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable condition_;
};

class Renderer{
    private:
        int thread();
        sf::Font font;
        std::thread t;
        CubeLog *logger;
        bool running = true;
        sf::RenderWindow window;
        std::vector<sf::Event> events;
        std::vector<Object*> objects;
        Shader* shader;
        Shader* textShader;
        TaskQueue setupQueue;
        TaskQueue loopQueue;
        std::atomic<bool> ready = false;
        std::latch* latch;
    public:
        Renderer(CubeLog *logger, std::latch& latch);
        ~Renderer();
        void stop();
        std::vector<sf::Event> getEvents();
        bool getIsRunning();
        void addObject(Object *object);
        Shader* getShader();
        Shader* getTextShader();
        void addSetupTask(std::function<void()> task);
        void addLoopTask(std::function<void()> task);
        void setupTasksRun();
        void loopTasksRun();
        bool isReady();
};