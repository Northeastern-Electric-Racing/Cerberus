/**
 * @file can_config.h
 * @author Hamza Iqbal
 * @brief Configuration File for CAN driver
 *
 */


#define CAN_BAUD_RATE                       1000000     //Change if needed
#define CAN_PRESCALER                       1
#define CAN_MODE                            CAN_MODE_NORMAL
#define CAN_SYNC_JUMP_WIDTH                 CAN_SJW_3TQ
#define CAN_TIME_SEG_1                      CAN_BS1_13TQ
#define CAN_TIME_SEG_2                      CAN_BS2_2TQ
#define CAN_TIME_TRIGGERED_MODE             DISABLE
#define CAN_AUTO_BUS_OFF                    DISABLE
#define CAN_AUTO_WAKEUP                     DISABLE
#define CAN_AUTO_RETRANSMISSION             DISABLE
#define CAN_RECEIVE_FIFO_LOCKED             DISABLE
#define CAN_TRANSMIT_FIFO_PRIORITY          DISABLE
#define CAN_SLAVE_START_FLITER_BANK
#define FAULT_MCU_Pin                       GPIO_PIN_3
#define FAULT_MCU_GPIO_Port                 GPIOC
#define BSE_1_Pin                           GPIO_PIN_0
#define BSE_1_GPIO_Port                     GPIOA
#define BSE_2_Pin                           GPIO_PIN_1
#define BSE_2_GPIO_Port                     GPIOA
#define APPS_1_Pin                          GPIO_PIN_2
#define APPS_1_GPIO_Port                    GPIOA
#define APPS_2_Pin                          GPIO_PIN_3
#define APPS_2_GPIO_Port                    GPIOA
#define GPIO_1_Pin                          GPIO_PIN_4
#define GPIO_1_GPIO_Port                    GPIOC
#define GPIO_2_Pin                          GPIO_PIN_5
#define GPIO_2_GPIO_Port                    GPIOC
#define GPIO_3_Pin                          GPIO_PIN_0
#define GPIO_3_GPIO_Port                    GPIOB
#define GPIO_4_Pin                          GPIO_PIN_1
#define GPIO_4_GPIO_Port                    GPIOB
#define WATCHDOG_Pin                        GPIO_PIN_15
#define WATCHDOG_GPIO_Port                  GPIOB
#define LED_1_Pin                           GPIO_PIN_8
#define LED_1_GPIO_Port                     GPIOC
#define LED_2_Pin                           GPIO_PIN_9
#define LED_2_GPIO_Port                     GPIOC
#define CAN_STD_ID                          0x0FF
#define CAN_EXT_ID                          0xFFF
