#ifdef TEMP_PID
#ifdef TEMPERATURE_CTRL

#include <PID_v1.h>

#define SAFETY_MAX_PCB_TEMP 7000  // pcb temperature is max 70°C
#define SAFETY_MIN_PCB_TEMP 1000  // pcb temperatire is min 10°C
#define SAFETY_MAX_LIQ_TEMP 6000  // liquid temperature is max 60°C
#define SAFETY_MIN_LIQ_TEMP 1000  // liquid temperature is min 10°C

#define PID_OUTPUT_LIMIT    200  //200 is ~80% of max PWM --> Limits max avg power to ~8A

void pid_ctrl();
void heatingSetup();

double heatingRegInput;
double heatingRegOutput;
double heatingRegSetpoint;
//Specify the heating regulation links and initial tuning parameters
PID heatingRegPID(&heatingRegInput, &heatingRegOutput, &heatingRegSetpoint, 1, 0.0002, 5, DIRECT);

NIL_WORKING_AREA(waThread_PID, 120); //tune the allocated mem (here extra is provided)
NIL_THREAD(Thread_PID, arg)
{
  nilThdSleepMilliseconds(5000);
  pinMode(TEMP_PID, OUTPUT);
  heatingSetup();

  while (TRUE) {
    pid_ctrl();
    nilThdSleepMilliseconds(500);  //refresh every 500ms --> the faster the better the control
  }
}

/*Temperature PID Control addressing relay*/

void pid_ctrl() {
  saveAndLogError(getParameter(PARAM_TEMP_LIQ) < SAFETY_MIN_LIQ_TEMP || getParameter(PARAM_TEMP_LIQ) > SAFETY_MAX_LIQ_TEMP, FLAG_TEMP_LIQ_RANGE_ERROR);
  saveAndLogError(getParameter(PARAM_TEMP_PCB) < SAFETY_MIN_PCB_TEMP || getParameter(PARAM_TEMP_PCB) > SAFETY_MAX_PCB_TEMP, FLAG_TEMP_PCB_RANGE_ERROR);
  saveAndLogError(getParameter(PARAM_TEMP_TARGET) < SAFETY_MIN_LIQ_TEMP || getParameter(PARAM_TEMP_TARGET) > SAFETY_MAX_LIQ_TEMP, FLAG_TEMP_TARGET_RANGE_ERROR);

  if (! getStatus(FLAG_PID_CONTROL)) { // PID is disabled
    analogWrite(TEMP_PID, 0);
    return;
  }

  if (isError(MASK_TEMP_ERROR)) {
    analogWrite(TEMP_PID, 0);
    return;
  }

  heatingRegInput = getParameter(PARAM_TEMP_LIQ);
  heatingRegSetpoint = getParameter(PARAM_TEMP_TARGET);
  heatingRegPID.Compute();                                   // the computation takes only 30ms!
  analogWrite(TEMP_PID, heatingRegOutput);
}

// see the rest of oliver's code for sanity checks
void heatingSetup()
{
  //tell the PID to range between 0 and the full window size
  heatingRegPID.SetOutputLimits(0, PID_OUTPUT_LIMIT);
  heatingRegPID.SetMode(AUTOMATIC);      //turn the PID on, cf. PID library
  heatingRegPID.SetSampleTime(950);      //set PID sampling time to 450ms
}

#endif
#endif
