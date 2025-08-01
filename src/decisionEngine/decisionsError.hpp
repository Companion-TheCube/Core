#include <stdexcept>
#include <memory>

namespace DecisionEngine {

enum class DecisionErrorType {
    ERROR_NONE,
    INVALID_PARAMS,
    INTERNAL_ERROR,
    NO_ACTION_SET,
    UNKNOWN_ERROR
};

/////////////////////////////////////////////////////////////////////////////////////

class DecisionEngineError : public std::runtime_error {
    static uint32_t errorCount;
    DecisionErrorType errorType;

public:
    DecisionEngineError(const std::string& message, DecisionErrorType errorType = DecisionErrorType::UNKNOWN_ERROR);
};

}