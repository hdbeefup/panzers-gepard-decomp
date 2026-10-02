# soundtest — console smoke test for SMilesConcert (Miles) and SBinkVideo (Bink).
# Only built with the Miles audio backend. Run it from a directory holding the
# game's mss32.dll / binkw32.dll / miles\ / intro.bik (see soundtest.cpp).
add_executable(soundtest soundtest.cpp)
target_link_libraries(soundtest PRIVATE sound panzers_bink core)
