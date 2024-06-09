# copy constants from code cerberus_conf
ACCEL1_MAX_VAL = 1866
ACCEL1_OFFSET = 980
ACCEL2_MAX_VAL = 3365
ACCEL2_OFFSET = 1780

# pedal values
pedal_value_1 = 1866
pedal_value_2 = 3365

# PEDAL1_TEN_PERCENT_OF_MAX = (ACCEL1_MAX_VAL - ACCEL1_OFFSET) * 0.00 + ACCEL1_OFFSET
# PEDAL2_TEN_PERCENT_OF_MAX = (ACCEL2_MAX_VAL - ACCEL2_OFFSET) * 0.00 + ACCEL2_OFFSET
#
# pedal_value_1 = PEDAL1_TEN_PERCENT_OF_MAX
# pedal_value_2 = PEDAL2_TEN_PERCENT_OF_MAX


# what is done in monitor.c, should clamp to 0 to 100
pedal_value_1 = 0 if (pedal_value_1 - ACCEL1_OFFSET <= 0) else (pedal_value_1 - ACCEL1_OFFSET )*100 / (ACCEL1_MAX_VAL - ACCEL1_OFFSET)
print("Clamped 1", pedal_value_1)
pedal_value_2= 0 if (pedal_value_2 - ACCEL2_OFFSET <= 0) else  (pedal_value_2 - ACCEL2_OFFSET )*100 / (ACCEL2_MAX_VAL - ACCEL2_OFFSET)
print("Clamped 2", pedal_value_2)

accelerator_value = (pedal_value_1+pedal_value_2) /2.0



# some stuff done in torque calc main thread
accelerator_value = accelerator_value/100.0
print("Accel val", accelerator_value)

# copy the torque multipliers, one for forwards and one for regen
TORQUE_FACTOR_FORWARDS = 220
TORQUE_FACTOR_BACKWARDS = 50

# AUTOCROSS
torque = accelerator_value*TORQUE_FACTOR_FORWARDS
print("Normal torque", accelerator_value)

#print("Ac amps", (torque/0.61) )

# ENDURANCE
max_ac_brake = 15; # Amps AC
OFFSET_VALUE_REGEN = 0.2
OFFSET_VALUE_ACCEL = 0.25

regen_torque = -1;
if accelerator_value >= OFFSET_VALUE_ACCEL:
    print("Offset accel go forward", accelerator_value)
    accelerator_value *= (1 / (1 - OFFSET_VALUE_ACCEL))
    torque = accelerator_value*TORQUE_FACTOR_FORWARDS
elif accelerator_value < OFFSET_VALUE_REGEN:
    regen_factor = max_ac_brake / (OFFSET_VALUE_REGEN*100)
    print("Offset accel go backward", accelerator_value)
    regen_torue = (accelerator_value*-1.0*regen_factor) * ((1 / OFFSET_VALUE) + (1 / (1 - OFFSET_VALUE))) *10
    if (torque/0.61) > max_ac_brake:
        torque = max_ac_brake * 0.61;
else:
    torque = 0
    regen = 0

print("Ac amps", (torque/0.61) )

# for num in range(-1,101,1):
#     PEDAL1_TEN_PERCENT_OF_MAX = (ACCEL1_MAX_VAL - ACCEL1_OFFSET) * (num / 100.0) + ACCEL1_OFFSET
#     PEDAL2_TEN_PERCENT_OF_MAX = (ACCEL2_MAX_VAL - ACCEL2_OFFSET) * (num / 100.0) + ACCEL2_OFFSET
#
#     pedal_value_1 = PEDAL1_TEN_PERCENT_OF_MAX
#     pedal_value_2 = PEDAL2_TEN_PERCENT_OF_MAX
#
#
#     # what is done in monitor.c, should clamp to 0 to 100
#     pedal_value_1 = 0 if (pedal_value_1 - ACCEL1_OFFSET <= 0) else (pedal_value_1 - ACCEL1_OFFSET )*100 / (ACCEL1_MAX_VAL - ACCEL1_OFFSET)
#     pedal_value_2= 0 if (pedal_value_2 - ACCEL2_OFFSET <= 0) else  (pedal_value_2 - ACCEL2_OFFSET )*100 / (ACCEL2_MAX_VAL - ACCEL2_OFFSET)
#
#     accelerator_value = (pedal_value_1+pedal_value_2) /2.0
#     print("Accel val", accelerator_value)
#
#     # copy the torque multipliers, one for forwards and one for regen
#     TORQUE_FACTOR_FORWARDS = 22
#
#     # AUTOCROSS
#     torque = accelerator_value*TORQUE_FACTOR_FORWARDS
#
#     # ENDURANCE
#     max_ac_brake = 25; # Amps AC
#     OFFSET_VALUE = 0.2
#     regen_factor = max_ac_brake / (OFFSET_VALUE * 100)
#
#     accelerator_value = accelerator_value - (100*OFFSET_VALUE)
#     if accelerator_value >= 0:
#         accelerator_value *= (1 / (1 - OFFSET_VALUE))
#         torque = accelerator_value*TORQUE_FACTOR_FORWARDS
#     else:
#         torque = (accelerator_value*-1.0*regen_factor) * ((1 / OFFSET_VALUE) + (1 / (1 - OFFSET_VALUE)))
#         if (torque/0.61)/10 > max_ac_brake:
#             torque = max_ac_brake * 0.61 * 10;
#
#
#     print("Ac amps", (torque/10.0)/0.61 , "\n")
