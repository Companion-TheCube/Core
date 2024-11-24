#ifndef PERSONALITYMANAGER_H
#define PERSONALITYMANAGER_H

#include <string>
#include <thread>
#include <mutex>
#include <stop_token>
#include <chrono>
#include <unordered_map>
#include <logger.h>
#include "utils.h"
#include <numbers>

#define EXPONENTIAL_RAMP_EXPONENT 2.0

namespace Personality {

using TimePoint = std::chrono::system_clock::time_point;

class Emotion{
public:
    enum class EmotionType{
        EMOTION_NOT_ASSIGNED,
        CURIOSITY,
        PLAYFULNESS,
        EMPATHY,
        ASSERTIVENESS,
        ATTENTIVENESS,
        CAUTION,
        EMOTION_COUNT
    };
    enum class TimeTargetType {
        TARGET_TYPE_NOT_DEFINED,
        TARGET_RAMP_EXPONENTIAL, // Exponential ramp to target value
        TARGET_RAMP_INVERSE_EXPONENTIAL, // Inverse exponential ramp to target value
        TARGET_RAMP_LINEAR, // Linear ramp to target value
        TARGET_RAMP_SINUSOIDAL, // Sinusoidal ramp to target value and back to default
        TARGET_RAMP_STEP, // Step to target value at targetValueTime
        TARGET_RAMP_COUNT
    };
    enum class RampStage{
        RAMP_NOT_STARTED,
        RAMP_TO_TARGET,
        RAMP_TO_DEFAULT,
        RAMP_COMPLETE
    };
    Emotion(int value = 0, EmotionType name);
    int operator=(int value);
    int operator+(int value);
    int operator-(int value);
    int operator*(int value);
    int operator/(int value);
    int operator+=(int value);
    int operator-=(int value);
    int operator*=(int value);
    int operator/=(int value);
    int operator++();
    int operator--();
    int operator++(int);
    int operator--(int);
    TimePoint getLastUpdate();
    TimeTargetType timeTargetType = TimeTargetType::TARGET_TYPE_NOT_DEFINED;
    TimeTargetType expirationType = TimeTargetType::TARGET_TYPE_NOT_DEFINED;
    TimeTargetType rampType = TimeTargetType::TARGET_TYPE_NOT_DEFINED;
    RampStage rampStage = RampStage::RAMP_COMPLETE;
    int currentValue; // Current value for the emotion
    int defaultValue; // Default value for the emotion
    int targetValue; // Expected value at targetValueTime
    int rampStartValue; // Value at the start of the ramp
    int rampEndValue; // Value at the end of the ramp
    TimePoint expirationTime;
    TimePoint targetValueTime;
    TimePoint rampStartTime;
private:
    EmotionType name = EmotionType::EMOTION_NOT_ASSIGNED;
    TimePoint lastUpdate;
};

class EmotionManager{
public:
    EmotionManager();
    ~EmotionManager();
    Emotion getEmotion(Emotion::EmotionType emotion);
    bool setEmotion(Emotion::EmotionType emotion, int value);
    bool setEmotion(Emotion::EmotionType emotion, TimePoint expiration, int value);
    bool setEmotion(Emotion::EmotionType emotion, TimePoint targetValueTime, int targetValue, TimePoint expiration, Emotion::TimeTargetType rampType);
    int getEmotionValue(Emotion::EmotionType emotion);
private:
    void managerThreadFunction(std::stop_token st);
    Emotion curiosity;
    Emotion playfulness;
    Emotion empathy;
    Emotion assertiveness;
    Emotion attentiveness;
    Emotion caution;

    std::unordered_map<Emotion::EmotionType, Emotion&> emotions;
    
    std::jthread* managerThread;
    std::mutex managerMutex;
};

}; // namespace Personality

#endif// PERSONALITYMANAGER_H
