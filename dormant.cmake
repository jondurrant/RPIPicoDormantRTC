# Add library cpp files

if (NOT DEFINED DORMANT_DIR)
    set(DORMANT_DIR "${CMAKE_CURRENT_LIST_DIR}/")
endif()

add_library(dormant STATIC)
target_sources(dormant PUBLIC
    ${DORMANT_DIR}/src/DS3231.cpp
    ${DORMANT_DIR}/src/Dormant.cpp
    ${DORMANT_DIR}/src/DeepSleep.cpp
    ${DORMANT_DIR}/src/DormantNotification.cpp
)

# Add include directory
target_include_directories(dormant PUBLIC 
   ${DORMANT_DIR}/src
)

# Add the standard library to the build
target_link_libraries(dormant PUBLIC 
	pico_stdlib
	hardware_i2c
    hardware_rtc
    hardware_clocks
	hardware_rosc
	hardware_sleep
	)