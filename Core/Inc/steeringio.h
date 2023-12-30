Button::Button(){}

Button::~Button(){}

void Button::checkButtonPin()
{
    if(isPressed == true && state == NOT_PRESSED)
    {
        state = DEBOUNCING;
        debounce.startTimer(BUTTON_DEBOUNCE_TIME);
    }
    else if(isPressed == false)
    {
        debounce.cancelTimer();
        state = NOT_PRESSED;
    }
}

bool Button::isButtonPressed()
{
    if(state == PRESSED) return true;

    /**
     * Returns true if
     * 1. The button has successfully passed the debounce period
     * 2. The debounce timer has not been canceled/reset
     * 3. The button is still being held
     */
    if(isPressed && debounce.isTimerExpired() && !debounce.isTimerReset())
    {
        state = PRESSED;
        return true;
    }
    return false;
}

bool Button::isButtonPressed_Pulse()
{
    if(state == PRESSED) return false;

    /**
     * Returns true if
     * 1. The button has successfully passed the debounce period
     * 2. The debounce timer has not been canceled/reset
     * 3. The button is still being held
     * 
     * AND
     * 
     * 4. The state of the button has not been read since the button has passed the debounce
     */
    if(isPressed && debounce.isTimerExpired() && !debounce.isTimerReset())
    {
        state = PRESSED;
        return true;
    }

    return false;
}

void Button::setButtonState(bool buttonState)
{
    isPressed = buttonState;
}

void DriverIO::wheelIO_cb(const CAN_message_t &msg)
{
    union
    {
        uint8_t msg[8];

        struct
        {
            uint16_t pot_l;
            uint16_t pot_r;
            bool button5 : 1;
            bool button6 : 1;
            bool button3 : 1;
            uint8_t x1   : 1;
            bool paddle_r : 1;
            bool paddle_l : 1;
            bool button4 : 1;
            bool button2 : 1;
            uint8_t x2;
            uint8_t x3;
            uint8_t x4;
        } io;
    } wheelio;

    for(uint8_t byte = 0; byte < 8; byte++)
    {
        wheelio.msg[byte] = msg.buf[byte];
    }

    wheelio.io.pot_l = SWITCHBYTES(wheelio.io.pot_l);
    wheelio.io.pot_r = SWITCHBYTES(wheelio.io.pot_r);

    decrButton.setButtonState(wheelio.io.button2);
    incrButton.setButtonState(wheelio.io.button4);
    regenButton.setButtonState(wheelio.io.button5);
    torqueIncreasePaddle.setButtonState(wheelio.io.paddle_r);
    torqueDecreasePaddle.setButtonState(wheelio.io.paddle_l);
    accumulatorFanDial.setDialValue(wheelio.io.pot_l);
    motorFanDial.setDialValue(wheelio.io.pot_r);
}