/**
 * @file emrax.h
 * @author Nick DePatie
 * @brief Config file to hold some important values for Emrax 228-MV motor
 * @version 0.1
 * @date 2023-12-28
 *
 * @copyright Copyright (c) 2023
 *
 */

/* 
 * Values retrieved from datasheet
 * Note: Emrax is Liquid Cooled
 * Note: We are using an encoder for position data
 */
#define EMRAX_MAX_MOTOR_TEMP    120     /* degC */
#define EMRAX_NOMINAL_VOLTAGE   520     /* V */
#define EMRAX_PEAK_EFFICIENCY   90      /* % */
#define EMRAX_PEAK_POWER        124     /* kW, at 5500 RPM */
#define EMRAX_PEAK_TORQUE       230     /* Nm */
#define EMRAX_CONT_TORQUE       112     /* Nm */
#define EMRAX_LIMITING_SPEED    6500    /* RPM */
#define EMRAX_KV                15.53
#define EMRAX_KT                0.61
#define EMRAX_PEAK_CURRENT      380     /* A_RMS */
#define EMRAX_CONT_CURRENT      180     /* A_RMS */
#define EMRAX_INTERN_PHASE_RES  7.06    /* mOhm, at 25 degC */
#define EMRAX_INDUCTION_2PHASE  96.5    /* mHenry */
#define EMRAX_INDUCED_VOLTAGE   0.04793 /* V_RMS / RPM */
#define EMRAX_NUM_POLE_PAIRS    10
#define EMRAX_MOTOR_INTERTIA    0.02521 /* kg * m^2 */
#define EMRAX_WEIGHT            13.5    /* kg */
