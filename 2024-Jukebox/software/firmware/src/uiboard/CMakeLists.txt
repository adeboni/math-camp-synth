add_executable(uiboard
        uiboard.c
        )

target_sources(uiboard PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/socket.c
        ${CMAKE_CURRENT_LIST_DIR}/timer.c
        ${CMAKE_CURRENT_LIST_DIR}/w5x00_gpio_irq.c
        ${CMAKE_CURRENT_LIST_DIR}/w5x00_spi.c
        ${CMAKE_CURRENT_LIST_DIR}/w5500.c
        ${CMAKE_CURRENT_LIST_DIR}/wizchip_conf.c
        ${CMAKE_CURRENT_LIST_DIR}/e131.c
	${CMAKE_CURRENT_LIST_DIR}/MAX7313.c
        ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
        ${CMAKE_CURRENT_LIST_DIR}/lcd_display.c
)

target_include_directories(uiboard PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(uiboard PUBLIC pico_stdlib pico_multicore pico_unique_id 
                                     tinyusb_device tinyusb_board
                                     hardware_i2c hardware_spi hardware_clocks 
                                     hardware_pwm hardware_dma hardware_adc)

pico_add_extra_outputs(uiboard)
