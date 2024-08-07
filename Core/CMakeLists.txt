add_library(cerb_lib STATIC
    ${CMAKE_CURRENT_LIST_DIR}/Src/bms.c
    ${CMAKE_CURRENT_LIST_DIR}/Src/can_handler.c
    ${CMAKE_CURRENT_LIST_DIR}/Src/dti.c
    ${CMAKE_CURRENT_LIST_DIR}/Src/fault.c
    ${CMAKE_CURRENT_LIST_DIR}/Src/monitor.c
    ${CMAKE_CURRENT_LIST_DIR}/Src/mpu.c
    ${CMAKE_CURRENT_LIST_DIR}/Src/nero.c
    ${CMAKE_CURRENT_LIST_DIR}/Src/pdu.c
    ${CMAKE_CURRENT_LIST_DIR}/Src/serial_monitor.c
    ${CMAKE_CURRENT_LIST_DIR}/Src/state_machine.c
    ${CMAKE_CURRENT_LIST_DIR}/Src/steeringio.c
    ${CMAKE_CURRENT_LIST_DIR}/Src/torque.c
)

target_include_directories(cerb_lib
PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/Inc
  )

target_link_libraries(cerb_lib PRIVATE stm32cubemx)

