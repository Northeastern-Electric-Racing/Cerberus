/* Sampling Intervals */
#define YELLOW_LED_BLINK_DELAY 500  /* ms */
#define TEMP_SENS_SAMPLE_DELAY 200  /* ms */
#define IMU_SAMPLE_DELAY	   500    /* ms */
#define FUSES_SAMPLE_DELAY	   1000   /* ms */
#define SHUTDOWN_MONITOR_DELAY 500   /* ms */
#define NERO_DELAY_TIME        100 /* ms*/
#define SERIAL_MONITOR_DELAY
#define CAN_ROUTER_DELAY
#define CAN_DISPATCH_DELAY      5
#define BMS_CAN_MONITOR_DELAY   7000
#define STATE_MACHINE_DELAY
#define TORQUE_CALC_DELAY
#define FAULT_HANDLE_DELAY

/* Pedal tuning */
#define PEDALS_SAMPLE_DELAY     20    /* ms */
#define ACCEL1_OFFSET           980
#define ACCEL1_MAX_VAL          1866
#define ACCEL2_OFFSET           1780
#define ACCEL2_MAX_VAL          3365
#define PEDAL_BRAKE_THRESH      650

/* Torque Tuning */
#define MAX_TORQUE              220 /* Nm * 10 */

// DEBUGGING: Start with a high debounce period, lower it if it is too slow
// NOTE: SteeringIOMonitor updates every 25 ms. Previous value of 25 ms doesn't
// give enough time to differentiate signal from noise.
#define STEERING_WHEEL_DEBOUNCE 100  /* ms */

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

#define CANID_PEDAL_SENSOR 0x002
// TODO: GET CORRECT CAN ID
#define CANID_IMU			0x003
#define CANID_TEMP_SENSOR	0x004
#define CANID_TORQUE_MSG	0x005
#define CANID_OUTBOUND_MSG	0xA55
#define CANID_FUSE			0x111
#define CANID_SHUTDOWN_LOOP 0x123
#define CANID_NERO_MSG      0x501
#define CANID_FAULT_MSG 	0x502
#define CANID_PEDALS_ACCEL_MSG 	0x504
#define CANID_PEDALS_BRAKE_MSG 	0x505
#define CANID_EXTRA_MSG     0x701 // Reserved for MPU debug message, see yaml for format