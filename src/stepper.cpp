#include "stepper.h"

#include <Arduino.h>
#include <FastAccelStepper.h>

#include "config.h"

namespace
{
FastAccelStepperEngine engine;
FastAccelStepper *stepper = nullptr;
} // namespace

void setupStepper()
{
  engine.init();
  stepper = engine.stepperConnectToPin(STEPPER_STEP_PIN);

  if (stepper == nullptr)
  {
    Serial.println("Failed to initialize stepper!");
    return;
  }

  stepper->setDirectionPin(STEPPER_DIR_PIN, true, 5000);
  stepper->setEnablePin(STEPPER_ENABLE_PIN, true);
  // Stepper should always be enabled (even if not moving).
  stepper->setAutoEnable(true);
  stepper->setDelayToDisable(10000);

  stepper->setSpeedInHz(30000);
  stepper->setAcceleration(5000);
}

void openBallReturn()
{
  if (stepper != nullptr)
    stepper->move(BALL_RETURN_STEPS);
}

void closeBallReturn()
{
  if (stepper != nullptr)
    stepper->move(-BALL_RETURN_STEPS);
}
