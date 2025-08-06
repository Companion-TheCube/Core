/*

The function registry provides a way to register and manage functions, which are data sources that are provided by apps.
Functions can be registered with a name, action, parameters, and other metadata.

It also provides HTTP endpoints for interacting with the function registry, allowing other applications to register, unregister, and query functions.
The decision engine can execute functions based on user input or other triggers, and it can also manage scheduled tasks that execute functions at 
specified times or intervals.

Functions are registered with the following information:
- Function name
- Action to execute
- Parameters for the action. These can be static or can be loaded from a data source.
- Brief description of the function
- Response string that can include placeholders for parameters
- Emotional score ranges for the function to be enabled or disabled based on the user's emotional state
- Matching params for Spacy
- Matching phrases for other matching methods

*/

#include "functionRegistry.h"

namespace DecisionEngine {

FunctionRegistry& FunctionRegistry::instance()
{
    static FunctionRegistry instance;
    return instance;
}

}