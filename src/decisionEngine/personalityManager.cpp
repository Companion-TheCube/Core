#include "personalityManager.h"

using namespace Personality;

int calculateCurrentValue(int startValue, int endValue, TimePoint startTime, TimePoint endTime, TimePoint currentTime, Emotion::TimeTargetType rampType, double exponent = 2.0);

Emotion::Emotion(int value, EmotionType name)
{
    this->name = name;
    this->currentValue = value;
    this->defaultValue = value;
    this->targetValue = value;
    this->expirationTime = std::chrono::system_clock::now();
    this->lastUpdate = std::chrono::system_clock::now();
    this->targetValueTime = std::chrono::system_clock::now();
}

int Emotion::operator=(int value)
{
    if(value == this->currentValue) return this->currentValue;
    if(value > 100) value = 100;
    if(value < 0) value = 0;
    this->currentValue = value;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator+(int value)
{
    if(value == 0) return this->currentValue;
    if(this->currentValue + value > 100) this->currentValue = 100;
    else this->currentValue += value;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator-(int value)
{
    if(value == 0) return this->currentValue;
    if(this->currentValue - value < 0) this->currentValue = 0;
    else this->currentValue -= value;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator*(int value)
{
    if(value == 1) return this->currentValue;
    if(this->currentValue * value > 100) this->currentValue = 100;
    else this->currentValue *= value;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator/(int value)
{
    if(value == 1) return this->currentValue;
    if(this->currentValue == 0) return 0;
    if(this->currentValue / value < 0) this->currentValue = 0;
    else this->currentValue /= value;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator+=(int value)
{
    if(value == 0) return this->currentValue;
    if(this->currentValue + value > 100) this->currentValue = 100;
    else this->currentValue += value;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator-=(int value)
{
    if(value == 0) return this->currentValue;
    if(this->currentValue - value < 0) this->currentValue = 0;
    else this->currentValue -= value;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator*=(int value)
{
    if(value == 1) return this->currentValue;
    if(this->currentValue * value > 100) this->currentValue = 100;
    else this->currentValue *= value;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator/=(int value)
{
    if(value == 1) return this->currentValue;
    if(this->currentValue == 0) return 0;
    if(this->currentValue / value < 0) this->currentValue = 0;
    else this->currentValue /= value;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator++()
{
    this->currentValue++;
    if(this->currentValue > 100) this->currentValue = 100;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator--()
{
    this->currentValue--;
    if(this->currentValue < 0) this->currentValue = 0;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator++(int)
{
    if(this->currentValue == 100) return this->currentValue;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue++;
}

int Emotion::operator--(int)
{
    if(this->currentValue == 0) return this->currentValue;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue--;
}

TimePoint Emotion::getLastUpdate()
{
    return this->lastUpdate;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EmotionManager::EmotionManager()
{
    curiosity = Emotion(80, Emotion::EmotionType::CURIOSITY);
    playfulness = Emotion(70, Emotion::EmotionType::PLAYFULNESS);
    empathy = Emotion(85, Emotion::EmotionType::EMPATHY);
    assertiveness = Emotion(60, Emotion::EmotionType::ASSERTIVENESS);
    attentiveness = Emotion(90, Emotion::EmotionType::ATTENTIVENESS);
    caution = Emotion(50, Emotion::EmotionType::CAUTION);

    // TODO: Here we should load the default values from the global settings

    emotions.insert({Emotion::EmotionType::CURIOSITY, curiosity});
    emotions.insert({Emotion::EmotionType::PLAYFULNESS, playfulness});
    emotions.insert({Emotion::EmotionType::EMPATHY, empathy});
    emotions.insert({Emotion::EmotionType::ASSERTIVENESS, assertiveness});
    emotions.insert({Emotion::EmotionType::ATTENTIVENESS, attentiveness});
    emotions.insert({Emotion::EmotionType::CAUTION, caution});
    // Start the manager thread
    managerThread = new std::jthread([this](std::stop_token st){
        managerThreadFunction(st);
    });
}

EmotionManager::~EmotionManager()
{
    delete managerThread;
}

Emotion EmotionManager::getEmotion(Emotion::EmotionType emotion)
{
    return emotions.at(emotion);
}

bool EmotionManager::setEmotion(Emotion::EmotionType emotion, int value)
{
    if(emotions.find(emotion) == emotions.end()) return false;
    emotions.at(emotion) = value;
    return true;
}

bool EmotionManager::setEmotion(Emotion::EmotionType emotion, TimePoint expiration, int value)
{
    if(emotions.find(emotion) == emotions.end()) return false;
    emotions.at(emotion) = value;
    emotions.at(emotion).expirationTime = expiration;
    emotions.at(emotion).expirationType = Emotion::TimeTargetType::TARGET_RAMP_STEP;
    emotions.at(emotion).rampStage = Emotion::RampStage::RAMP_TO_DEFAULT; // Short circuit the ramping process
    emotions.at(emotion).rampStartTime = std::chrono::system_clock::now();
    emotions.at(emotion).rampStartValue = value;
    emotions.at(emotion).rampEndValue = emotions.at(emotion).defaultValue;
    return true;
}

bool EmotionManager::setEmotion(Emotion::EmotionType emotion, TimePoint targetValueTime, int targetValue, TimePoint expiration, Emotion::TimeTargetType rampType)
{
    if(emotions.find(emotion) == emotions.end()) return false;
    emotions.at(emotion).targetValue = targetValue;
    emotions.at(emotion).targetValueTime = targetValueTime;
    emotions.at(emotion).expirationTime = expiration;
    emotions.at(emotion).timeTargetType = rampType;
    emotions.at(emotion).rampStage = Emotion::RampStage::RAMP_NOT_STARTED;
    emotions.at(emotion).rampStartTime = std::chrono::system_clock::now();
    return true;
}

void EmotionManager::managerThreadFunction(std::stop_token st)
{
    while(!st.stop_requested()){
        for(auto& [time, emote] : emotions){
            auto now = std::chrono::system_clock::now();
            switch(emote.rampStage){
                case Emotion::RampStage::RAMP_NOT_STARTED:
                    emote.rampStage = Emotion::RampStage::RAMP_TO_TARGET;
                    emote.rampStartTime = now;
                    emote.rampStartValue = emote.currentValue;
                    emote.rampEndValue = emote.targetValue;
                    break;
                case Emotion::RampStage::RAMP_TO_TARGET:
                    emote.currentValue = calculateCurrentValue(emote.rampStartValue, emote.rampEndValue, emote.rampStartTime, emote.targetValueTime, now, emote.rampType);
                    if(now >= emote.targetValueTime){
                        emote.rampStage = Emotion::RampStage::RAMP_TO_DEFAULT;
                        emote.rampStartTime = now;
                        emote.rampStartValue = emote.currentValue;
                        emote.rampEndValue = emote.defaultValue;
                    }
                    break;
                case Emotion::RampStage::RAMP_TO_DEFAULT:
                    emote.currentValue = calculateCurrentValue(emote.rampEndValue, emote.rampStartValue, emote.rampStartTime, emote.expirationTime, now, emote.rampType);
                    if(now >= emote.expirationTime){
                        emote.rampStage = Emotion::RampStage::RAMP_COMPLETE;
                    }
                    break;
                case Emotion::RampStage::RAMP_COMPLETE:
                    emote.currentValue = emote.defaultValue;
                    break;
            }
        }
        genericSleep(250);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: personality manger
// The personality should be able to modify things based on the current state of the emotions. It needs methods that can alter text or other values
// based on the current state of the emotions. For example, if the empathy is high, the personality should be more empathetic in its responses. If the
// playfulness is high, the frequency of jokes or funny animations should be increased.

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int calculateCurrentValue(int startValue, int endValue, TimePoint startTime, TimePoint endTime, TimePoint currentTime, Emotion::TimeTargetType rampType, double exponent = EXPONENTIAL_RAMP_EXPONENT) {
    using namespace std::chrono;

    // Calculate the total duration
    auto totalDuration = duration_cast<milliseconds>(endTime - startTime).count();
    auto elapsedDuration = duration_cast<milliseconds>(currentTime - startTime).count();

    // Clamp elapsedDuration
    elapsedDuration = std::max(0LL, std::min(elapsedDuration, totalDuration));

    // Calculate the normalized time (t) between 0 and 1
    double t = static_cast<double>(elapsedDuration) / totalDuration;

    // Apply the ramp type transformation
    switch (rampType) {
        case Emotion::TimeTargetType::TARGET_RAMP_LINEAR:
            // No transformation for linear interpolation
            break;

        case Emotion::TimeTargetType::TARGET_RAMP_EXPONENTIAL:
            t = std::pow(t, exponent);  // Exponential growth
            break;

        case Emotion::TimeTargetType::TARGET_RAMP_INVERSE_EXPONENTIAL:
            t = 1.0 - std::pow(1.0 - t, exponent);  // Inverse exponential
            break;

        case Emotion::TimeTargetType::TARGET_RAMP_SINUSOIDAL:
            t = 0.5 * (1.0 - std::cos(t * std::numbers::pi));  // Smooth sinusoidal
            break;
        case Emotion::TimeTargetType::TARGET_RAMP_STEP:
            if(currentTime >= endTime) return endValue;
            else return startValue;
            break;
    }

    // Interpolate linearly between startValue and endValue using the transformed t
    return static_cast<int>(startValue + t * (endValue - startValue));
}