// src/stubs/stub_log.h
// STUB_LOG("Name") — log a stub's name the first time it is called.
// Every function in src/stubs/ must start with STUB_LOG; tools/census.py
// counts the STUB_LOG(" call sites in src/stubs/*.cpp to report the stub total.
#ifndef PANZERS_STUB_LOG_H
#define PANZERS_STUB_LOG_H

void StubLogFirstCall(const char* name);

#define STUB_LOG(name)                          \
    do {                                        \
        static bool s_stubLogged = false;       \
        if (!s_stubLogged) {                    \
            s_stubLogged = true;                \
            StubLogFirstCall(name);             \
        }                                       \
    } while (0)

#endif // PANZERS_STUB_LOG_H
