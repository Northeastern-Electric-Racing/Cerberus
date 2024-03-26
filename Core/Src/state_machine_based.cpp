// state-machine based code for pedal faulting

void vPedalsMonitor(void* pv_params)
{
	const uint8_t num_samples = 10;
	enum { ACCELPIN_1, ACCELPIN_2, BRAKEPIN_1, BRAKEPIN_2 };
	// nertimer_t diff_timer;
	// nertimer_t sc_timer;
	// nertimer_t oc_timer;

	/*diff_timer: timer for difference between pedal sensor values*/
	nertimer_t diff_timer;
	/*sc: short circuit tiimer*/
	nertimer_t sc_timer;
	/*oc: open circuit timer*/
	nertimer_t oc_timer; 

	static pedals_t sensor_data;
	fault_data_t fault_data = { .id = ONBOARD_PEDAL_FAULT, .severity = DEFCON1 };
	//can_msg_t pedal_msg		= { .id = CANID_PEDAL_SENSOR, .len = 4, .data = { 0 } };
	uint16_t adc_data[3];

	/* Handle ADC Data for two input accelerator value and two input brake value*/
	mpu_t *mpu = (mpu_t *)pv_params;

	//uint32_t curr_tick = HAL_GetTick();

	for (;;) {
		//serial_print("Pedals Task: %d\r\n", HAL_GetTick() - curr_tick);
		//curr_tick = HAL_GetTick();
		if (read_adc(mpu, adc_data)) {
			fault_data.diag = "Failed to collect ADC Data!";
			queue_fault(&fault_data);
		}

		

		// declaration of funtion to evaluate pedal faulting conditions
		void eval_pedal_fault (int ACCELPIN_1, int ACCELPIN_2);

		// definition of function to evaluate pedal faulting conditions
		void eval_pedal_fault(ACCELPIN_1, ACCELPIN_2) {
        
        // state machine
            // Evaluating fault
            // Faulted
            // Not Faulted
        // in every cycle of the function all the three timers are triggered 

		
        /* Evaluate accelerator faults */

		// checking if the timer has expired for the oc:open circuit timer
        // when no values are being read

		// checking if the values given by sensor - 1/sensor - 2 are
		// the "max ADC"  values possible. Signalling a open - circuit
		if ((adc_data[ACCELPIN_1] == MAX_ADC_VAL_12b || adc_data[ACCELPIN_2] == MAX_ADC_VAL_12b) && !is_timer_active(&oc_timer)) {
        	// starting the open circuit timer
			start_timer(&oc_timer, PEDAL_FAULT_TIME);
            if ((adc_data[ACCELPIN_1] == MAX_ADC_VAL_12b || adc_data[ACCELPIN_2] == MAX_ADC_VAL_12b ) &&  is_timer_active(&oc_timer)) {
                if (is_timer_expired(&oc_timer)) {
			    //todo queue fault
			    fault_data.diag = "Failed to send CAN message";
			    queue_fault(&fault_data);
            } 
		    else {
		    // if there is no pedal faulting condition cancel
		    // timer and return to not faulted condition
			cancel_timer(&oc_timer);
            }
            }
        }

		

        // continue to evaluation faults
		
		// checking if the values given by sensor - 1/sensor - 2 are
		// the "min ADC" values possible. Signalling a short - circuit

        // logic - short circuit
        // check for fault conditions
            // if yes start timer
            // while the timer is progressing check for the fault condition again
            // if the timer is expired in the process print fault
                // else cancel the timer and start checking for faults again


		else if ((adc_data[ACCELPIN_1] == 0 || adc_data[ACCELPIN_2] == 0) &&  !is_timer_active(&sc_timer)) {
			start_timer(&sc_timer, PEDAL_FAULT_TIME);
            
            if ((adc_data[ACCELPIN_1] == 0 || adc_data[ACCELPIN_2] == 0) &&  is_timer_active(&sc_timer)) {
                if (is_timer_expired(&sc_timer)) {
			    //todo queue fault
			    fault_data.diag = "Failed to send CAN message";
			    queue_fault(&fault_data);
            } 
            }
            // checking if the timer has expired for the sc:short circuit timer
		    
            else {
            // moving to not faulted conditions
                cancel_timer(&sc_timer);
            }
        }
		

		// checking if the values given by sensor - 1/sensor - 2 are
		// the "min ADC" values possible. Signalling a short - circuit
		else if ((adc_data[ACCELPIN_1] - adc_data[ACCELPIN_2] > PEDAL_DIFF_THRESH * MAX_ADC_VAL_12b) && !is_timer_active(&diff_timer)) {
		// starting diff timer
		start_timer(&diff_timer, PEDAL_FAULT_TIME);
        if ((adc_data[ACCELPIN_1] - adc_data[ACCELPIN_2] > PEDAL_DIFF_THRESH * MAX_ADC_VAL_12b) && is_timer_active(&diff_timer)) {
            // check if diff timer:difference between generated values is > 100ms // has expired
		    if (is_timer_expired(&diff_timer)) {
			//todo queue fault
			    fault_data.diag = "Failed to send CAN message";
			    queue_fault(&fault_data);
			    continue;
            }
        }
		else {
			cancel_timer(&diff_timer);
		} 
        }

		

		/*Evaluate brake faults*/

		/*Open circuit*/

		if(is_timer_expired(&oc_timer))
			// todo queue fault
			continue:
		else if ((adc_data[BRAKEPIN_1] == MAX_ADC_VAL_12b || adc_data[BRAKEPIN_2] == MAX_ADC_VAL_12b) &&
			!is_timer_active(&oc_timer))
			start_timer(&oc_timer, PEDAL_FAULT_TIME);
		else
			cancel_timer(&oc_timer);

		/*Short circuit*/

		if (is_timer_expired(&sc_timer))
			//todo queue fault
			continue;
		else if ((adc_data[BRAKEPIN_1] == 0 || adc_data[BRAKEPIN_2] == 0) &&
			!is_timer_active(&sc_timer))
			start_timer(&sc_timer, PEDAL_FAULT_TIME);
		else
			cancel_timer(&sc_timer);

		/*Diff between generated values > 100 ms*/
		if (is_timer_expired(&diff_timer))
			//todo queue fault
			continue;
		else if ((adc_data[BRAKEPIN_1] - adc_data[BRAKEPIN_2] > PEDAL_DIFF_THRESH * MAX_ADC_VAL_12b) &&
			!is_timer_active(&diff_timer))
			start_timer(&diff_timer, PEDAL_FAULT_TIME);
		else
			cancel_timer(&diff_timer);



		/* Low Pass Filter */
		sensor_data.accelerator_value
			= (sensor_data.accelerator_value + (adc_data[ACCELPIN_1] + adc_data[ACCELPIN_2]) / 2)
			  / num_samples;
		 sensor_data.brake_value
			= (sensor_data.brake_value + (adc_data[BRAKEPIN_1] + adc_data[BRAKEPIN_2]) / 2)
			  / num_samples;

		/* Publish to Onboard Pedals Queue */
		osMessageQueuePut(pedal_data_queue, &sensor_data, 0U, 0U);

		osDelay(PEDALS_SAMPLE_DELAY);
	}
}