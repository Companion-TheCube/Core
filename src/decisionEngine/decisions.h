#ifndef DECISIONS_H
#define DECISIONS_H

#include "../database/cubeDB.h"
#include <logger.h>
#include <stdexcept>
#include <string>
#include <iostream>

namespace DecisionEngine {
    enum class DecisionErrorType {
        ERROR_NONE,
        INVALID_PARAMS,
        INTERNAL_ERROR,
        UNKNOWN_ERROR
    };
class DecisionEngineError : public std::runtime_error {
    static uint32_t errorCount;
    DecisionErrorType errorType;
public:
    DecisionEngineError(const std::string& message, DecisionErrorType errorType = DecisionErrorType::UNKNOWN_ERROR);
};
}; // namespace DecisionEngine

#endif // DECISIONS_H
