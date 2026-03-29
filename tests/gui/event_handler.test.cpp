#include <gtest/gtest.h>

#include "../../src/gui/eventHandler/eventHandler.h"

class TestClickable final : public Clickable {
public:
    explicit TestClickable(long xMin, long xMax, long yMin, long yMax)
    {
        area_.clickableObject = this;
        area_.xMin = xMin;
        area_.xMax = xMax;
        area_.yMin = yMin;
        area_.yMax = yMax;
    }

    void onClick(const CubeEvent&) override { ++clickCount; }
    void onRelease(const CubeEvent&) override { ++releaseCount; }
    void onMouseDown(const CubeEvent&) override { ++mouseDownCount; }
    void onRightClick(const CubeEvent&) override { ++rightClickCount; }
    std::vector<MeshObject*> getObjects() override { return {}; }
    void setOnClick(std::function<unsigned int(void*)>) override { }
    void setOnRightClick(std::function<unsigned int(void*)>) override { }
    ClickableArea* getClickableArea() override { return &area_; }
    void setVisibleWidth(float) override { }
    void setClickAreaSize(unsigned int xMin, unsigned int xMax, unsigned int yMin, unsigned int yMax) override
    {
        area_.xMin = xMin;
        area_.xMax = xMax;
        area_.yMin = yMin;
        area_.yMax = yMax;
    }
    void capturePosition() override { }
    void restorePosition() override { }
    void resetScroll() override { }
    bool getIsClickable() override { return clickable_; }
    bool setIsClickable(bool isClickable) override
    {
        clickable_ = isClickable;
        return clickable_;
    }
    void draw() override { }
    bool setVisible(bool visible) override
    {
        visible_ = visible;
        return visible_;
    }
    bool getVisible() override { return visible_; }

    int clickCount = 0;
    int releaseCount = 0;
    int mouseDownCount = 0;
    int rightClickCount = 0;

private:
    ClickableArea area_;
    bool visible_ = true;
    bool clickable_ = true;
};

TEST(EventManager, ClickWithinThresholdTriggersClickableCallbacks)
{
    EventManager manager;
    TestClickable clickable(0, 100, 0, 100);
    manager.addClickableArea(clickable.getClickableArea());

    CubeEvent press;
    press.type = CubeEventType::MouseButtonPressed;
    press.mouseButton = CubeMouseButton::Left;
    press.x = 10;
    press.y = 10;

    CubeEvent release = press;
    release.type = CubeEventType::MouseButtonReleased;
    release.x = 12;
    release.y = 13;

    EXPECT_TRUE(manager.triggerEvent(press));
    EXPECT_TRUE(manager.triggerEvent(release));
    EXPECT_EQ(clickable.mouseDownCount, 1);
    EXPECT_EQ(clickable.releaseCount, 1);
    EXPECT_EQ(clickable.clickCount, 1);
    EXPECT_EQ(clickable.rightClickCount, 0);
}

TEST(EventManager, DragBeyondThresholdEmitsDragEventWithoutClick)
{
    EventManager manager;
    TestClickable clickable(0, 100, 0, 100);
    manager.addClickableArea(clickable.getClickableArea());

    int dragCount = 0;
    int lastDeltaY = 0;
    const int dragIndex = manager.createEvent("DragY");
    EventHandler* dragHandler = manager.getEvent(dragIndex);
    dragHandler->setEventType(CubeEventType::MouseMoved);
    dragHandler->setSpecificEventType(SpecificEventTypes::DRAG_Y);
    dragHandler->setAction([&](const CubeEvent& event) {
        ++dragCount;
        lastDeltaY = event.deltaY;
    });

    CubeEvent press;
    press.type = CubeEventType::MouseButtonPressed;
    press.mouseButton = CubeMouseButton::Left;
    press.x = 20;
    press.y = 20;

    CubeEvent move;
    move.type = CubeEventType::MouseMoved;
    move.x = 22;
    move.y = 48;

    CubeEvent release;
    release.type = CubeEventType::MouseButtonReleased;
    release.mouseButton = CubeMouseButton::Left;
    release.x = 22;
    release.y = 48;

    EXPECT_TRUE(manager.triggerEvent(press));
    EXPECT_FALSE(manager.triggerEvent(move));
    EXPECT_FALSE(manager.triggerEvent(release));
    EXPECT_EQ(dragCount, 1);
    EXPECT_EQ(lastDeltaY, 28);
    EXPECT_EQ(clickable.clickCount, 0);
    EXPECT_EQ(clickable.mouseDownCount, 1);
    EXPECT_EQ(clickable.releaseCount, 1);
}
