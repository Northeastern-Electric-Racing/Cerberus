/* Sampling Intervals */
#define YELLOW_LED_BLINK_DELAY 500  /* ms */
#define TEMP_SENS_SAMPLE_DELAY 200  /* ms */
#define IMU_SAMPLE_DELAY	   5    /* ms */
#define FUSES_SAMPLE_DELAY	   50   /* ms */
#define SHUTDOWN_MONITOR_DELAY 50   /* ms */
#define SERIAL_MONITOR_DELAY
#define CAN_ROUTER_DELAY
#define CAN_DISPATCH_DELAY      5000
#define BMS_CAN_MONITOR_DELAY   5000 
#define STATE_MACHINE_DELAY
#define TORQUE_CALC_DELAY
#define FAULT_HANDLE_DELAY

/* Pedal tuning */
#define PEDALS_SAMPLE_DELAY	   5    /* ms */
#define ACCEL1_OFFSET           2200
#define ACCEL1_MAX_VAL          1200
#define ACCEL2_OFFSET           2200
#define ACCEL2_MAX_VAL          1200

/* Torque Tuning */
#define MAX_TORQUE              110 /* Nm * 10 */

#define STEERING_WHEEL_DEBOUNCE 25  /* ms */

/* Pin Assignments */
#define FAULT_MCU_Pin		GPIO_PIN_3
#define FAULT_MCU_GPIO_Port GPIOC
#define BSE_1_Pin			GPIO_PIN_0
#define BSE_1_GPIO_Port		GPIOA
#define BSE_2_Pin			GPIO_PIN_1
#define BSE_2_GPIO_Port		GPIOA
#define APPS_1_Pin			GPIO_PIN_2
#define APPS_1_GPIO_Port	GPIOA
#define APPS_2_Pin			GPIO_PIN_3
#define APPS_2_GPIO_Port	GPIOA
#define GPIO_1_Pin			GPIO_PIN_4
#define GPIO_1_GPIO_Port	GPIOC
#define GPIO_2_Pin			GPIO_PIN_5
#define GPIO_2_GPIO_Port	GPIOC
#define GPIO_3_Pin			GPIO_PIN_0
#define GPIO_3_GPIO_Port	GPIOB
#define GPIO_4_Pin			GPIO_PIN_1
#define GPIO_4_GPIO_Port	GPIOB
#define WATCHDOG_Pin		GPIO_PIN_15
#define WATCHDOG_GPIO_Port	GPIOB

#define CANID_TEMP_SENSOR  0xBEEF
#define CANID_PEDAL_SENSOR 0xDEAD
// TODO: GET CORRECT CAN ID
#define CANID_IMU			0xDEAD
#define CANID_TEMP_SENSOR	0xBEEF
#define CANID_OUTBOUND_MSG	0xA55
#define CANID_TORQUE_MSG	0xBA115
#define CANID_FUSE			0x111
#define CANID_SHUTDOWN_LOOP 0x123