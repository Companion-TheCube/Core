#pragma once
#ifndef PERSONALITYMANAGER_H
#define PERSONALITYMANAGER_H

#include "utils.h"
#include <chrono>
#ifndef LOGGER_H
#include <logger.h>
#endif
#include <mutex>
#include <numbers>
#include <stop_token>
#include <string>
#include <thread>
#include <unordered_map>
#ifndef API_I_H
#include "../api/api.h"
#endif
#ifndef GLOBALSETTINGS_H
#include "../settings/globalSettings.h"
#endif

#define EXPONENTIAL_RAMP_EXPONENT 2.0
#define EMOTION_MAX_VALUE 100
#define EMOTION_MIN_VALUE 1

namespace Personality {

using TimePoint = std::chrono::system_clock::time_point;

class Emotion {
public:
    enum class EmotionType {
        EMOTION_NOT_ASSIGNED,
        CURIOSITY,
        PLAYFULNESS,
        EMPATHY,
        ASSERTIVENESS,
        ATTENTIVENESS,
        CAUTION,
        ANNOYANCE,
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
    enum class RampStage {
        RAMP_NOT_STARTED,
        RAMP_TO_TARGET,
        RAMP_TO_DEFAULT,
        RAMP_COMPLETE
    };
    Emotion() = default;
    Emotion(int value, EmotionType name);
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
    operator int() const { return currentValue; }
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
    EmotionType name = EmotionType::EMOTION_NOT_ASSIGNED;
private:
    TimePoint lastUpdate;
};

struct EmotionSimple{
    Emotion::EmotionType emotion;
    int value;
};

// Struct to hold the range of values for an emotion.
struct EmotionRange {
    int min;
    int max;
    float weight;
    Emotion::EmotionType emotion;
};

const int interpretScore(const float score);
const std::string emotionToString(const Emotion::EmotionType emotion);

class PersonalityManager : public AutoRegisterAPI<PersonalityManager> {
public:
    PersonalityManager();
    ~PersonalityManager();
    const Emotion getEmotion(Emotion::EmotionType emotion);
    // TODO: add methods that allow for ramp type of expiration
    bool setEmotion(Emotion::EmotionType emotion, int value);
    bool setEmotion(Emotion::EmotionType emotion, TimePoint expiration, int value);
    bool setEmotion(Emotion::EmotionType emotion, TimePoint targetValueTime, int targetValue, Emotion::TimeTargetType rampType);
    bool setEmotion(Emotion::EmotionType emotion, TimePoint targetValueTime, int targetValue, TimePoint expiration, Emotion::TimeTargetType rampType);
    int getEmotionValue(Emotion::EmotionType emotion);
    std::vector<EmotionSimple> getAllEmotionsCurrent();
    float calculateEmotionalMatchScore(std::vector<EmotionRange> emotionRanges);
    // API Interface
    HttpEndPointData_t getHttpEndpointData() override;
    constexpr std::string getInterfaceName() const override;

private:
    void managerThreadFunction(std::stop_token st);
    Emotion curiosity;
    Emotion playfulness;
    Emotion empathy;
    Emotion assertiveness;
    Emotion attentiveness;
    Emotion caution;
    Emotion annoyance;

    std::unordered_map<Emotion::EmotionType, Emotion&> emotions;

    std::jthread* managerThread;
    std::mutex managerMutex;
};

}; // namespace Personality

#endif // PERSONALITYMANAGER_H
