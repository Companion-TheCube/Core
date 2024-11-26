#ifndef AUTOREGISTER_H
#define AUTOREGISTER_H
#ifndef BUILDER_H
#include "builder.h"
#endif

template <typename T> class AutoRegisterAPI {
protected:
    AutoRegisterAPI() {
        if(this->___api_registered) return;
        API_Builder::addInterfaceStatic(std::make_shared<T>(this));
        this->___api_registered = true;
    }
    bool ___api_registered = false;
};

#endif// AUTOREGISTER_H
