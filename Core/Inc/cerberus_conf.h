#define YELLOW_LED_BLINK_DELAY  500 /* ms */

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

#define CANID_TEMP_SENSOR 0xBEEF
#define CANID_PEDAL_SENSOR 0xDEAD
//TODO: GET CORRECT CAN ID
#define CANID_IMU 0xDEAD
#define CANID_TEMP_SENSOR 0xBEEF
#define CANID_OUTBOUND_MSG 0xA55