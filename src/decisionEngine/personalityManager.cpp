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
    if (value == this->currentValue)
        return this->currentValue;
    if (value > EMOTION_MAX_VALUE)
        value = EMOTION_MAX_VALUE;
    if (value < EMOTION_MIN_VALUE)
        value = EMOTION_MIN_VALUE;
    this->currentValue = value;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator+(int value)
{
    if (value == 0)
        return this->currentValue;
    if (this->currentValue + value > EMOTION_MAX_VALUE)
        this->currentValue = EMOTION_MAX_VALUE;
    else
        this->currentValue += value;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator-(int value)
{
    if (value == 0)
        return this->currentValue;
    if (this->currentValue - value < EMOTION_MIN_VALUE)
        this->currentValue = EMOTION_MIN_VALUE;
    else
        this->currentValue -= value;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator*(int value)
{
    if (value == 1)
        return this->currentValue;
    if (this->currentValue * value > EMOTION_MAX_VALUE)
        this->currentValue = EMOTION_MAX_VALUE;
    else
        this->currentValue *= value;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator/(int value)
{
    if (value == 1)
        return this->currentValue;
    if (this->currentValue == 0)
        return 0;
    if (this->currentValue / value < EMOTION_MIN_VALUE)
        this->currentValue = EMOTION_MIN_VALUE;
    else
        this->currentValue /= value;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator+=(int value)
{
    if (value == 0)
        return this->currentValue;
    if (this->currentValue + value > EMOTION_MAX_VALUE)
        this->currentValue = EMOTION_MAX_VALUE;
    else
        this->currentValue += value;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator-=(int value)
{
    if (value == 0)
        return this->currentValue;
    if (this->currentValue - value < EMOTION_MIN_VALUE)
        this->currentValue = EMOTION_MIN_VALUE;
    else
        this->currentValue -= value;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator*=(int value)
{
    if (value == 1)
        return this->currentValue;
    if (this->currentValue * value > EMOTION_MAX_VALUE)
        this->currentValue = EMOTION_MAX_VALUE;
    else
        this->currentValue *= value;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator/=(int value)
{
    if (value == 1)
        return this->currentValue;
    if (this->currentValue == 0)
        return 0;
    if (this->currentValue / value < EMOTION_MIN_VALUE)
        this->currentValue = EMOTION_MIN_VALUE;
    else
        this->currentValue /= value;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator++()
{
    this->currentValue++;
    if (this->currentValue > EMOTION_MAX_VALUE)
        this->currentValue = EMOTION_MAX_VALUE;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator--()
{
    this->currentValue--;
    if (this->currentValue < EMOTION_MIN_VALUE)
        this->currentValue = EMOTION_MIN_VALUE;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue;
}

int Emotion::operator++(int)
{
    if (this->currentValue == EMOTION_MAX_VALUE)
        return this->currentValue;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue++;
}

int Emotion::operator--(int)
{
    if (this->currentValue == EMOTION_MIN_VALUE)
        return this->currentValue;
    this->lastUpdate = std::chrono::system_clock::now();
    return this->currentValue--;
}

TimePoint Emotion::getLastUpdate()
{
    return this->lastUpdate;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PersonalityManager::PersonalityManager()
{
    curiosity = Emotion(80, Emotion::EmotionType::CURIOSITY);
    playfulness = Emotion(70, Emotion::EmotionType::PLAYFULNESS);
    empathy = Emotion(85, Emotion::EmotionType::EMPATHY);
    assertiveness = Emotion(60, Emotion::EmotionType::ASSERTIVENESS);
    attentiveness = Emotion(90, Emotion::EmotionType::ATTENTIVENESS);
    caution = Emotion(50, Emotion::EmotionType::CAUTION);
    annoyance = Emotion(1, Emotion::EmotionType::ANNOYANCE);

    auto curiosityInt = GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::EMOTION_CURIOSITY);
    auto playfulnessInt = GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::EMOTION_PLAYFULNESS);
    auto empathyInt = GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::EMOTION_EMPATHY);
    auto assertivenessInt = GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::EMOTION_ASSERTIVENESS);
    auto attentivenessInt = GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::EMOTION_ATTENTIVENESS);
    auto cautionInt = GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::EMOTION_CAUTION);
    auto annoyanceInt = GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::EMOTION_ANNOYANCE);

    // Assume that a value of 0 means that the setting was not previously set.
    curiosity = curiosityInt > 0 ? curiosityInt : curiosity;
    playfulness = playfulnessInt > 0 ? playfulnessInt : playfulness;
    empathy = empathyInt > 0 ? empathyInt : empathy;
    assertiveness = assertivenessInt > 0 ? assertivenessInt : assertiveness;
    attentiveness = attentivenessInt > 0 ? attentivenessInt : attentiveness;
    caution = cautionInt > 0 ? cautionInt : caution;
    annoyance = annoyanceInt > 0 ? annoyanceInt : annoyance;

    // When the settings are changed, such as in the menu, update the values in the personality manager
    GlobalSettings::setSettingCB(GlobalSettings::SettingType::EMOTION_CURIOSITY, [&]() {
        std::unique_lock<std::mutex> lock(managerMutex);
        curiosity = GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::EMOTION_CURIOSITY);
        // This is necessary in case the value is out of bounds
        GlobalSettings::setSetting(GlobalSettings::SettingType::EMOTION_CURIOSITY, curiosity.currentValue);
    });
    GlobalSettings::setSettingCB(GlobalSettings::SettingType::EMOTION_PLAYFULNESS, [&]() {
        std::unique_lock<std::mutex> lock(managerMutex);
        playfulness = GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::EMOTION_PLAYFULNESS);
        GlobalSettings::setSetting(GlobalSettings::SettingType::EMOTION_PLAYFULNESS, playfulness.currentValue);
    });
    GlobalSettings::setSettingCB(GlobalSettings::SettingType::EMOTION_EMPATHY, [&]() {
        std::unique_lock<std::mutex> lock(managerMutex);
        empathy = GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::EMOTION_EMPATHY);
        GlobalSettings::setSetting(GlobalSettings::SettingType::EMOTION_EMPATHY, empathy.currentValue);
    });
    GlobalSettings::setSettingCB(GlobalSettings::SettingType::EMOTION_ASSERTIVENESS, [&]() {
        std::unique_lock<std::mutex> lock(managerMutex);
        assertiveness = GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::EMOTION_ASSERTIVENESS);
        GlobalSettings::setSetting(GlobalSettings::SettingType::EMOTION_ASSERTIVENESS, assertiveness.currentValue);
    });
    GlobalSettings::setSettingCB(GlobalSettings::SettingType::EMOTION_ATTENTIVENESS, [&]() {
        std::unique_lock<std::mutex> lock(managerMutex);
        attentiveness = GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::EMOTION_ATTENTIVENESS);
        GlobalSettings::setSetting(GlobalSettings::SettingType::EMOTION_ATTENTIVENESS, attentiveness.currentValue);
    });
    GlobalSettings::setSettingCB(GlobalSettings::SettingType::EMOTION_CAUTION, [&]() {
        std::unique_lock<std::mutex> lock(managerMutex);
        caution = GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::EMOTION_CAUTION);
        GlobalSettings::setSetting(GlobalSettings::SettingType::EMOTION_CAUTION, caution.currentValue);
    });
    GlobalSettings::setSettingCB(GlobalSettings::SettingType::EMOTION_ANNOYANCE, [&]() {
        std::unique_lock<std::mutex> lock(managerMutex);
        annoyance = GlobalSettings::getSettingOfType<int>(GlobalSettings::SettingType::EMOTION_ANNOYANCE);
        GlobalSettings::setSetting(GlobalSettings::SettingType::EMOTION_ANNOYANCE, annoyance.currentValue);
    });

    emotions.insert({ Emotion::EmotionType::CURIOSITY, curiosity });
    emotions.insert({ Emotion::EmotionType::PLAYFULNESS, playfulness });
    emotions.insert({ Emotion::EmotionType::EMPATHY, empathy });
    emotions.insert({ Emotion::EmotionType::ASSERTIVENESS, assertiveness });
    emotions.insert({ Emotion::EmotionType::ATTENTIVENESS, attentiveness });
    emotions.insert({ Emotion::EmotionType::CAUTION, caution });
    emotions.insert({ Emotion::EmotionType::ANNOYANCE, annoyance });
    // Start the manager thread
    managerThread = new std::jthread([this](std::stop_token st) {
        managerThreadFunction(st);
    });
}

PersonalityManager::~PersonalityManager()
{
    delete managerThread;
}

const Emotion PersonalityManager::getEmotion(Emotion::EmotionType emotion)
{
    std::unique_lock<std::mutex> lock(managerMutex);
    return emotions.at(emotion);
}

bool PersonalityManager::setEmotion(Emotion::EmotionType emotion, int value)
{
    std::unique_lock<std::mutex> lock(managerMutex);
    if (emotions.find(emotion) == emotions.end())
        return false;
    emotions.at(emotion) = value;
    return true;
}

bool PersonalityManager::setEmotion(Emotion::EmotionType emotion, TimePoint expiration, int value)
{
    if(expiration <= std::chrono::system_clock::now())
        return false;
    std::unique_lock<std::mutex> lock(managerMutex);
    if (emotions.find(emotion) == emotions.end())
        return false;
    emotions.at(emotion) = value;
    emotions.at(emotion).expirationTime = expiration;
    emotions.at(emotion).expirationType = Emotion::TimeTargetType::TARGET_RAMP_STEP;
    emotions.at(emotion).rampStage = Emotion::RampStage::RAMP_TO_DEFAULT; // Short circuit the ramping process
    emotions.at(emotion).rampStartTime = std::chrono::system_clock::now();
    emotions.at(emotion).rampStartValue = value;
    emotions.at(emotion).rampEndValue = emotions.at(emotion).defaultValue;
    return true;
}

bool PersonalityManager::setEmotion(Emotion::EmotionType emotion, TimePoint targetValueTime, int targetValue, Emotion::TimeTargetType rampType)
{
    if(targetValueTime <= std::chrono::system_clock::now())
        return false;
    std::unique_lock<std::mutex> lock(managerMutex);
    if (emotions.find(emotion) == emotions.end())
        return false;
    emotions.at(emotion).targetValue = targetValue;
    emotions.at(emotion).targetValueTime = targetValueTime;
    emotions.at(emotion).expirationTime = targetValueTime;
    emotions.at(emotion).timeTargetType = rampType;
    emotions.at(emotion).rampStage = Emotion::RampStage::RAMP_NOT_STARTED;
    emotions.at(emotion).rampStartTime = std::chrono::system_clock::now();
    return true;
}

bool PersonalityManager::setEmotion(Emotion::EmotionType emotion, TimePoint targetValueTime, int targetValue, TimePoint expiration, Emotion::TimeTargetType rampType)
{
    if(targetValueTime >= expiration)
        return false;
    if(expiration <= std::chrono::system_clock::now())
        return false;
    std::unique_lock<std::mutex> lock(managerMutex);
    if (emotions.find(emotion) == emotions.end())
        return false;
    emotions.at(emotion).targetValue = targetValue;
    emotions.at(emotion).targetValueTime = targetValueTime;
    emotions.at(emotion).expirationTime = expiration;
    emotions.at(emotion).timeTargetType = rampType;
    emotions.at(emotion).rampStage = Emotion::RampStage::RAMP_NOT_STARTED;
    emotions.at(emotion).rampStartTime = std::chrono::system_clock::now();
    return true;
}

int PersonalityManager::getEmotionValue(Emotion::EmotionType emotion)
{
    std::unique_lock<std::mutex> lock(managerMutex);
    if (emotions.find(emotion) == emotions.end())
        return -1;
    return emotions.at(emotion).currentValue;
}

std::vector<EmotionSimple> PersonalityManager::getAllEmotionsCurrent()
{
    std::unique_lock<std::mutex> lock(managerMutex);
    std::vector<EmotionSimple> currentEmotions;
    for (auto& [name, emote] : emotions) {
        EmotionSimple simple;
        simple.emotion = name;
        simple.value = emote.currentValue;
        currentEmotions.push_back(simple);
    }
}

void PersonalityManager::managerThreadFunction(std::stop_token st)
{
    while (!st.stop_requested()) {
        genericSleep(250);
        std::unique_lock<std::mutex> lock(managerMutex);
        for (auto& [time, emote] : emotions) {
            auto now = std::chrono::system_clock::now();
            switch (emote.rampStage) {
            case Emotion::RampStage::RAMP_NOT_STARTED:
                emote.rampStage = Emotion::RampStage::RAMP_TO_TARGET;
                emote.rampStartTime = now;
                emote.rampStartValue = emote.currentValue;
                emote.rampEndValue = emote.targetValue;
                break;
            case Emotion::RampStage::RAMP_TO_TARGET:
                emote.currentValue = calculateCurrentValue(emote.rampStartValue, emote.rampEndValue, emote.rampStartTime, emote.targetValueTime, now, emote.rampType);
                if (now >= emote.targetValueTime) {
                    emote.rampStage = Emotion::RampStage::RAMP_TO_DEFAULT;
                    emote.rampStartTime = now;
                    emote.rampStartValue = emote.currentValue;
                    emote.rampEndValue = emote.defaultValue;
                }
                break;
            case Emotion::RampStage::RAMP_TO_DEFAULT:
                emote.currentValue = calculateCurrentValue(emote.rampEndValue, emote.rampStartValue, emote.rampStartTime, emote.expirationTime, now, emote.rampType);
                if (now >= emote.expirationTime) {
                    emote.rampStage = Emotion::RampStage::RAMP_COMPLETE;
                }
                break;
            case Emotion::RampStage::RAMP_COMPLETE:
                emote.currentValue = emote.defaultValue;
                break;
            }
        }
    }
}

float PersonalityManager::calculateEmotionalMatchScore(std::vector<EmotionRange> emotionRanges)
{
    std::unique_lock<std::mutex> lock(managerMutex);
    // verify that emotionRanges has all emotions. If not, add them with default values.
    for(auto& emote : emotions) {
        bool found = false;
        for(auto& range : emotionRanges) {
            if(range.emotion == emote.second.name) {
                found = true;
                break;
            }
        }
        if(!found) {
            EmotionRange range;
            range.emotion = emote.second.name;
            range.min = EMOTION_MIN_VALUE;
            range.max = EMOTION_MAX_VALUE;
            range.weight = 1.0f;
            emotionRanges.push_back(range);
        }
    }
    // Make sure all the values in the emotionRanges make sense
    for(size_t i = 0; i < emotionRanges.size(); i++) {
        // clamp weight to 0.0f to 1.0f
        if(emotionRanges.at(i).weight < 0.0f)
            emotionRanges.at(i).weight = 0.0f;
        if(emotionRanges.at(i).weight > 1.0f)
            emotionRanges.at(i).weight = 1.0f;
        if(emotionRanges.at(i).min > emotionRanges.at(i).max)
            std::swap(emotionRanges.at(i).min, emotionRanges.at(i).max);
        if(emotionRanges.at(i).min < EMOTION_MIN_VALUE)
            emotionRanges.at(i).min = std::abs(emotionRanges.at(i).min);
        if(emotionRanges.at(i).max < EMOTION_MIN_VALUE)
            emotionRanges.at(i).max = std::abs(emotionRanges.at(i).max);
        if(emotionRanges.at(i).min > EMOTION_MAX_VALUE)
            emotionRanges.at(i).min = EMOTION_MAX_VALUE;
        if(emotionRanges.at(i).max > EMOTION_MAX_VALUE)
            emotionRanges.at(i).max = EMOTION_MAX_VALUE;
    }
    // Also create and populate a vector of current values in the same order as emotionRanges
    std::vector<float> currentValues;
    for(auto& range : emotionRanges) {
        currentValues.push_back(emotions.at(range.emotion).currentValue);
    }
    // Calculate the score
    std::array<float, 7> distances = {0.0f}; // Array to store distances for each dimension
    for (size_t i = 0; i < 7; ++i) {
        if (currentValues[i] < emotionRanges[i].min) {
            distances[i] = emotionRanges[i].min - currentValues[i]; // Below the range
        } else if (currentValues[i] > emotionRanges[i].max) {
            distances[i] = currentValues[i] - emotionRanges[i].max; // Above the range
        } else {
            distances[i] = 0.0f; // Within the range
        }
    }
    // Compute the Euclidean distance (length of the distance vector) multiplied by the weight
    float sumOfSquares = 0.0f;
    for (size_t i = 0; i < 7; ++i) {
        sumOfSquares += distances[i] * distances[i] * emotionRanges[i].weight;
    }
    return std::sqrt(sumOfSquares);
}

constexpr std::string PersonalityManager::getInterfaceName() const
{
    return "PersonalityManager";
}

HttpEndPointData_t PersonalityManager::getHttpEndpointData()
{
    HttpEndPointData_t data;
    // 1. Get Emotion value
    // 2. Set Emotion value [with target value], [target time], [expiration], [ramp type]
    data.push_back({ PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            auto emotion = req.get_param_value("emotion");
            auto emote = Emotion::EmotionType::EMOTION_NOT_ASSIGNED;
            if (emotion == "curiosity")
                emote = Emotion::EmotionType::CURIOSITY;
            else if (emotion == "playfulness")
                emote = Emotion::EmotionType::PLAYFULNESS;
            else if (emotion == "empathy")
                emote = Emotion::EmotionType::EMPATHY;
            else if (emotion == "assertiveness")
                emote = Emotion::EmotionType::ASSERTIVENESS;
            else if (emotion == "attentiveness")
                emote = Emotion::EmotionType::ATTENTIVENESS;
            else if (emotion == "caution")
                emote = Emotion::EmotionType::CAUTION;
            else {
                nlohmann::json j;
                j["success"] = false;
                j["message"] = "getEmotionValue called: failed to get emotion value, emotion not recognized.";
                res.set_content(j.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NOT_FOUND, "Emotion not recognized");
            }
            nlohmann::json j;
            j["success"] = true;
            j["emotion"] = emotion;
            j["value"] = getEmotionValue(emote);
            res.set_content(j.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Emotion value retrieved");
        },
        "getEmotionValue",
        { "emotion" },
        "Get the value of an emotion. Emotion can be curiosity, playfulness, empathy, assertiveness, attentiveness, or caution." });
    data.push_back({ PRIVATE_ENDPOINT | GET_ENDPOINT,
        [&](const httplib::Request& req, httplib::Response& res) {
            if (!req.has_param("emotion")) {
                nlohmann::json j;
                j["success"] = false;
                j["message"] = "setEmotionValue called: failed to set emotion value, emotion not provided.";
                res.set_content(j.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_REQUEST, "Emotion not provided");
            }
            auto emotion = req.get_param_value("emotion");
            auto emote = Emotion::EmotionType::EMOTION_NOT_ASSIGNED;
            if (emotion == "curiosity")
                emote = Emotion::EmotionType::CURIOSITY;
            else if (emotion == "playfulness")
                emote = Emotion::EmotionType::PLAYFULNESS;
            else if (emotion == "empathy")
                emote = Emotion::EmotionType::EMPATHY;
            else if (emotion == "assertiveness")
                emote = Emotion::EmotionType::ASSERTIVENESS;
            else if (emotion == "attentiveness")
                emote = Emotion::EmotionType::ATTENTIVENESS;
            else if (emotion == "caution")
                emote = Emotion::EmotionType::CAUTION;
            else {
                nlohmann::json j;
                j["success"] = false;
                j["message"] = "setEmotionValue called: failed to set emotion value, emotion not recognized.";
                res.set_content(j.dump(), "application/json");
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NOT_FOUND, "Emotion not recognized");
            }
            int requestType = 0;
            if (req.has_param("value"))
                requestType += 1;
            if (req.has_param("expiration"))
                requestType += 2;
            if (req.has_param("targetValue"))
                requestType += 4;
            if (req.has_param("targetTime"))
                requestType += 8;
            if (req.has_param("rampType"))
                requestType += 16;
            nlohmann::json j_response;
            j_response["emotion"] = emotion;
            try {
                switch (requestType) {
                case 1: {
                    auto value = req.get_param_value("value");
                    if (setEmotion(emote, std::stoi(value))) {
                        j_response["success"] = true;
                        j_response["message"] = "Emotion value set to " + value;
                    } else {
                        j_response["success"] = false;
                        j_response["message"] = "setEmotionValue called: failed to set emotion value, emotion not found.";
                    }
                    break;
                }
                case 3: {
                    auto value = req.get_param_value("value");
                    long long expirationTime = std::stoll(req.get_param_value("expiration"));
                    TimePoint expirationPoint = TimePoint(std::chrono::milliseconds(expirationTime));
                    if (setEmotion(emote, expirationPoint, std::stoi(value))) {
                        j_response["success"] = true;
                        j_response["message"] = "Emotion value set to " + value + " with expiration time " + std::to_string(expirationTime);
                    } else {
                        j_response["success"] = false;
                        j_response["message"] = "setEmotionValue called: failed to set emotion value, emotion not found.";
                    }
                    break;
                }
                case 17:
                case 33: {
                    auto value = req.get_param_value("value");
                    auto targetValue = req.get_param_value("targetValue");
                    auto targetTime = req.get_param_value("targetTime");
                    std::string rampType = "";
                    if (requestType == 33)
                        rampType = req.get_param_value("rampType");
                    long long targetTimeLong = std::stoll(targetTime);
                    TimePoint targetTimePoint = TimePoint(std::chrono::milliseconds(targetTimeLong));
                    bool setResult = false;
                    if (rampType.length() > 0) {
                        auto rampTypeInt = std::stoi(rampType);
                        if (rampTypeInt < 0 || rampTypeInt > (int)Emotion::TimeTargetType::TARGET_RAMP_COUNT)
                            rampTypeInt = 0;
                        auto rampTypeType = Emotion::TimeTargetType(rampTypeInt);
                        setResult = setEmotion(emote, targetTimePoint, std::stoi(targetValue), TimePoint(std::chrono::milliseconds(0)), rampTypeType);
                    } else {
                        setResult = setEmotion(emote, targetTimePoint, std::stoi(targetValue), Emotion::TimeTargetType::TARGET_RAMP_LINEAR);
                    }
                    if (setResult) {
                        j_response["success"] = true;
                        j_response["message"] = "Emotion value set to " + targetValue + " at " + targetTime;
                    } else {
                        j_response["success"] = false;
                        j_response["message"] = "setEmotionValue called: failed to set emotion value, emotion not found.";
                    }
                    break;
                }
                case 19:
                case 35: {
                    auto value = req.get_param_value("value");
                    auto targetValue = req.get_param_value("targetValue");
                    auto targetTime = req.get_param_value("targetTime");
                    auto expiration = req.get_param_value("expiration");
                    long long targetTimeLong = std::stoll(targetTime);
                    long long expirationLong = std::stoll(expiration);
                    TimePoint targetTimePoint = TimePoint(std::chrono::milliseconds(targetTimeLong));
                    TimePoint expirationPoint = TimePoint(std::chrono::milliseconds(expirationLong));
                    std::string rampType = "";
                    if (requestType == 35)
                        rampType = req.get_param_value("rampType");
                    bool setResult = false;
                    if (rampType.length() > 0) {
                        auto rampTypeInt = std::stoi(rampType);
                        if (rampTypeInt < 0 || rampTypeInt > (int)Emotion::TimeTargetType::TARGET_RAMP_COUNT)
                            rampTypeInt = 0;
                        auto rampTypeType = Emotion::TimeTargetType(rampTypeInt);
                        setResult = setEmotion(emote, targetTimePoint, std::stoi(targetValue), expirationPoint, rampTypeType);
                    } else {
                        setResult = setEmotion(emote, targetTimePoint, std::stoi(targetValue), expirationPoint, Emotion::TimeTargetType::TARGET_RAMP_LINEAR);
                    }
                    if (setResult) {
                        j_response["success"] = true;
                        j_response["message"] = "Emotion value set to " + targetValue + " at " + targetTime + " with expiration at " + expiration;
                    } else {
                        j_response["success"] = false;
                        j_response["message"] = "setEmotionValue called: failed to set emotion value, emotion not found.";
                    }
                    break;
                }
                default: {
                    nlohmann::json j;
                    j["success"] = false;
                    j["message"] = "setEmotionValue called: failed to set emotion value, invalid request.";
                    res.set_content(j.dump(), "application/json");
                    return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_INVALID_REQUEST, "Invalid request");
                    break;
                }
                }
            } catch (std::exception& e) {
                nlohmann::json j;
                j["success"] = false;
                j["message"] = "setEmotionValue called: failed to set emotion value, " + std::string(e.what());
            }
            res.set_content(j_response.dump(), "application/json");
            return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, j_response["message"]);
        },
        "setEmotionValue",
        { "emotion", "value", "targetValue", "targetTime", "expiration", "rampType" },
        "Set the value of an emotion. Emotion can be curiosity, playfulness, empathy, assertiveness, attentiveness, or caution. Value must be between "+std::to_string(EMOTION_MIN_VALUE)+" and "+std::to_string(EMOTION_MAX_VALUE)+". Optional parameters: [targetValue, targetTime], [expiration], [rampType]." });
    return data;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int calculateCurrentValue(int startValue, int endValue, TimePoint startTime, TimePoint endTime, TimePoint currentTime, Emotion::TimeTargetType rampType, double exponent)
{
    using namespace std::chrono;
    auto totalDuration = duration_cast<milliseconds>(endTime - startTime).count();
    auto elapsedDuration = duration_cast<milliseconds>(currentTime - startTime).count();
    elapsedDuration = std::max(0LL, std::min(elapsedDuration, totalDuration));
    double t = static_cast<double>(elapsedDuration) / totalDuration;
    switch (rampType) {
    case Emotion::TimeTargetType::TARGET_RAMP_LINEAR:
        break;
    case Emotion::TimeTargetType::TARGET_RAMP_EXPONENTIAL:
        t = std::pow(t, exponent); // Exponential growth
        break;
    case Emotion::TimeTargetType::TARGET_RAMP_INVERSE_EXPONENTIAL:
        t = 1.0 - std::pow(1.0 - t, exponent); // Inverse exponential
        break;
    case Emotion::TimeTargetType::TARGET_RAMP_SINUSOIDAL:
        t = 0.5 * (1.0 - std::cos(t * std::numbers::pi)); // Smooth sinusoidal
        break;
    case Emotion::TimeTargetType::TARGET_RAMP_STEP:
        if (currentTime >= endTime)
            return endValue;
        else
            return startValue;
        break;
    }
    return static_cast<int>(startValue + t * (endValue - startValue));
}

inline const int Personality::interpretScore(const float score)
{
    if(score == 0.f) return 0;
    if(score < 35.f) return 1;
    if(score < 60.f) return 2;
    if(score < 75.f) return 3;
    if(score < 90.f) return 4;
    return 5;
}

inline const std::string Personality::emotionToString(const Emotion::EmotionType emotion)
{
    switch(emotion) {
        case Emotion::EmotionType::CURIOSITY: return "Curiosity";
        case Emotion::EmotionType::PLAYFULNESS: return "Playfulness";
        case Emotion::EmotionType::EMPATHY: return "Empathy";
        case Emotion::EmotionType::ASSERTIVENESS: return "Assertiveness";
        case Emotion::EmotionType::ATTENTIVENESS: return "Attentiveness";
        case Emotion::EmotionType::CAUTION: return "Caution";
        case Emotion::EmotionType::ANNOYANCE: return "Annoyance";
        default: return "Unknown";
    }
}