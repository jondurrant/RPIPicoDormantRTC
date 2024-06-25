add_executable(${NAME}
        main.cpp
        ../../../src/DS3231.cpp
        ../../../src/Dormant.cpp
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(${NAME} 
    pico_stdlib
    hardware_i2c
    hardware_rtc
    hardware_clocks
	hardware_rosc
	hardware_sleep
    )

target_include_directories(${NAME} PRIVATE 
	../../../src/
	$ENV{PICO_EXTRAS_PATH}/src/rp2_common/pico_sleep/include
	)
	

# create map/bin/hex file etc.
pico_add_extra_outputs(${NAME})

pico_enable_stdio_uart(${NAME} 1)
target_compile_definitions(${NAME} PRIVATE
    PICO_DEFAULT_UART_RX_PIN=16
    PICO_DEFAULT_UART_TX_PIN=17
)

