# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(SmartMirror_ESP32)

# --- 1. BUILD START MESSAGE ---
add_custom_target(print_build_start
    COMMAND ${CMAKE_COMMAND} -E echo " "
    COMMAND ${CMAKE_COMMAND} -E echo "######################################"
    COMMAND ${CMAKE_COMMAND} -E echo "###        COMPILING STARTED       ###"
    COMMAND ${CMAKE_COMMAND} -E echo "######################################"
    COMMAND ${CMAKE_COMMAND} -E echo " "
    VERBATIM
)

# Force the main app to depend on this message so it prints early
add_dependencies(${CMAKE_PROJECT_NAME}.elf print_build_start)


# --- 3. SUCCESS MESSAGE (FORCE RUN EVERY TIME) ---
# The "ALL" keyword forces this to run on every build
add_custom_target(print_flash_ready ALL
    COMMAND ${CMAKE_COMMAND} -E echo " "
    COMMAND ${CMAKE_COMMAND} -E echo "################################################"
    COMMAND ${CMAKE_COMMAND} -E echo "###        FINISHED, READY TO FLASH          ###"
    COMMAND ${CMAKE_COMMAND} -E echo "################################################"
    COMMAND ${CMAKE_COMMAND} -E echo " "
    VERBATIM
)

# This ensures the message waits until the APP is actually finished
add_dependencies(print_flash_ready ${CMAKE_PROJECT_NAME}.elf)