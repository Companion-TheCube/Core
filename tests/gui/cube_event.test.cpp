#include <gtest/gtest.h>

#include "../../src/gui/eventHandler/cubeEvent.h"
#include "../../src/threadsafeQueue.h"

#include <GLFW/glfw3.h>

#include <chrono>
#include <future>
#include <thread>

using namespace std::chrono_literals;

TEST(CubeEvent, MapsGlfwKeysToCubeKeys)
{
    EXPECT_EQ(cubeKeyFromGlfwKey(GLFW_KEY_A), CubeKey::A);
    EXPECT_EQ(cubeKeyFromGlfwKey(GLFW_KEY_ESCAPE), CubeKey::Escape);
    EXPECT_EQ(cubeKeyFromGlfwKey(GLFW_KEY_KP_ADD), CubeKey::Add);
    EXPECT_EQ(cubeKeyFromGlfwKey(GLFW_KEY_UNKNOWN), CubeKey::Unknown);
}

TEST(CubeEvent, MapsButtonsToSpecificEventTypes)
{
    EXPECT_EQ(cubeMouseButtonFromGlfwButton(GLFW_MOUSE_BUTTON_RIGHT), CubeMouseButton::Right);
    EXPECT_EQ(
        specificEventTypeFromCubeMouseButton(CubeMouseButton::Left, CubeEventType::MouseButtonPressed),
        SpecificEventTypes::MOUSEPRESSED_LEFT);
    EXPECT_EQ(
        specificEventTypeFromCubeMouseButton(CubeMouseButton::Right, CubeEventType::MouseButtonReleased),
        SpecificEventTypes::MOUSERELEASED_RIGHT);
    EXPECT_EQ(
        specificEventTypeFromCubeKey(CubeKey::F5),
        SpecificEventTypes::KEYPRESS_F5);
}

TEST(CubeEvent, QueueCloseUnblocksBlockedEventPop)
{
    ThreadSafeQueue<CubeEvent> queue;
    std::promise<std::optional<CubeEvent>> resultPromise;
    auto resultFuture = resultPromise.get_future();

    std::thread consumer([&]() {
        resultPromise.set_value(queue.pop());
    });

    queue.close();

    ASSERT_EQ(resultFuture.wait_for(250ms), std::future_status::ready);
    EXPECT_FALSE(resultFuture.get().has_value());
    consumer.join();
}
