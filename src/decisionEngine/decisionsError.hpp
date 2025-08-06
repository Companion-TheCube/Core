#pragma once
#ifndef DECISIONS_ERROR_HPP
#define DECISIONS_ERROR_HPP
#include <stdexcept>
#include <memory>

namespace DecisionEngine {

// enum class DecisionErrorType {
//     ERROR_NONE,
//     INVALID_PARAMS,
//     INTERNAL_ERROR,
//     NO_ACTION_SET,
//     UNKNOWN_ERROR
// };

// /////////////////////////////////////////////////////////////////////////////////////

// class DecisionEngineError : public std::runtime_error {
//     static uint32_t errorCount;
//     DecisionErrorType errorType;

// public:
//     DecisionEngineError(const std::string& message, DecisionErrorType errorType = DecisionErrorType::UNKNOWN_ERROR)
//     : std::runtime_error(message)
// {
//     this->errorType = errorType;
//     CubeLog::error("DecisionEngineError: " + message);
//     errorCount++;
// }
// };

// uint32_t DecisionEngineError::errorCount = 0;

}

#endif // DECISIONS_ERROR_HPP