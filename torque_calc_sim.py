# copy constants from code cerberus_conf
ACCEL1_MAX_VAL = 1866
ACCEL1_OFFSET = 980
ACCEL2_MAX_VAL = 3365
ACCEL2_OFFSET = 1780

# pedal values
# pedal_value_1 = 1367
# pedal_value_2 = 2833

pedal_value_1 = 1866
pedal_value_2 = 3365


# what is done in monitor.c, should clamp to 0 to 100
pedal_value_1 = 0 if (pedal_value_1 - ACCEL1_OFFSET <= 0) else (pedal_value_1 - ACCEL1_OFFSET )*100 / (ACCEL1_MAX_VAL - ACCEL1_OFFSET)
print("Clamped 1", pedal_value_1)
pedal_value_2= 0 if (pedal_value_2 - ACCEL2_OFFSET <= 0) else  (pedal_value_2 - ACCEL2_OFFSET )*100 / (ACCEL2_MAX_VAL - ACCEL2_OFFSET)
print("Clamped 2", pedal_value_2)

accelerator_value = (pedal_value_1+pedal_value_2) /2.0



# some stuff done in torque calc main thread
accelerator_value = accelerator_value
print("Accel val", accelerator_value)
perc_accel = accelerator_value / (((ACCEL1_MAX_VAL - ACCEL1_OFFSET) + (ACCEL2_MAX_VAL - ACCEL2_OFFSET)) / 2.0)


# copy the torque multipliers, one for forwards and one for regen
TORQUE_FACTOR_FORWARDS = 22
TORQUE_FACTOR_BACKWARDS = 5

# AUTOCROSS
print("Perc accel ", perc_accel)
torque = accelerator_value*TORQUE_FACTOR_FORWARDS
print("Normal torque", accelerator_value)

print("Ac amps", (torque/0.61)/10 )

# ENDURANCE
OFFSET_VALUE = 0.2

accelerator_value = accelerator_value - (100*OFFSET_VALUE)
if accelerator_value > 0:
    print("Offset accel go forward", accelerator_value)
    torque = accelerator_value*TORQUE_FACTOR_FORWARDS
else:
    print("Offset accel go backward", accelerator_value)
    torque = accelerator_value*TORQUE_FACTOR_BACKWARDS


print("Ac amps", (torque/0.61)/10 )
