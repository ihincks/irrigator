#include <LiquidCrystal.h>
#include "Button.h"
#include "LowPower.h"

// LCD
const int lcd_rs = 9;
const int lcd_en = 8;
const int lcd_d4 = 7;
const int lcd_d5 = 6;
const int lcd_d6 = 5;
const int lcd_d7 = 4;
LiquidCrystal lcd(lcd_rs, lcd_en, lcd_d4, lcd_d5, lcd_d6, lcd_d7);

// OUTPUT
const int pump = 2;
bool pumpState = false;

int i = 0;

/// Stores the state of the LCD backlight.
class Backlight {
  private:
    /// ms to keep the LCD backlight on past the last action.
    static const unsigned int MS_ON = 30000;

    /// Pin of the backlight.
    const unsigned int _pin;

    /// Current state of the LCD backlight.
    bool _on;

    /// How long the LCD backlight has been on since last action.
    unsigned int _msCount;

  public:
    Backlight() = delete;

    Backlight(unsigned int pin) : _msCount(0), _on(false), _pin(pin) {
    }

    void begin() {
        pinMode(_pin, OUTPUT);
        turnOn();
    }

    void callback() {
        if (_on && (++_msCount > MS_ON)) {
            _on = false;
            digitalWrite(_pin, LOW);
        }
    }

    const decltype(_on) isOn() const {
        return _on;
    }

    void turnOn() {
        if (!_on) {
            digitalWrite(_pin, HIGH);
            _on = true;
        }
        _msCount = 0;
    }
};

class Screen {
  protected:
    unsigned int _page = 0;

  public:
    virtual void actionDown(){};
    virtual void actionUp(){};

    void clear() const {
        lcd.clear();
    }

    virtual void display(){};

    void firstPage() {
        _page = 0;
    }

    const bool next() {
        ++_page;
        clear();
        if (_page >= numPages()) {
            _page = 0;
            return true;
        }
        return false;
    };

    virtual unsigned int numPages() {
        return 1;
    };
};

class Screen1 : public Screen {
  public:
    void display() {
        lcd.setCursor(0, 0);
        lcd.write("Screen1");
        lcd.setCursor(0, 1);
        lcd.print(_page);
    };

    unsigned int numPages() {
        return 3;
    };
};

class Screen2 : public Screen {
  public:
    void display() {
        lcd.setCursor(0, 0);
        lcd.write("Screen2");
        lcd.setCursor(0, 1);
        lcd.print(_page);
    };

    unsigned int numPages() {
        return 2;
    };
};

class Time : public Screen {
  private:
    enum STATES { IDLE, SET_MINUTES, SET_HOURS };

    static const unsigned long MS_DAY = 86400000ul;
    static const unsigned long MS_HOUR = 86400000ul / 24ul;
    static const unsigned long MS_MINUTE = 86400000ul / (24ul * 60ul);
    static const unsigned long MS_SECOND = 86400000ul / (24ul * 60ul * 60ul);
    unsigned long _msCount = 0;

    void dec(const unsigned long amount) {
        if (_msCount < amount) {
            _msCount += MS_DAY;
        }
        _msCount -= amount;
    }

    void inc(const unsigned long amount) {
        _msCount += amount;
        if (_msCount > MS_DAY) {
            _msCount -= MS_DAY;
        }
    }

  public:
    void actionUp() {
        if (_page == SET_MINUTES) {
            inc(MS_MINUTE);
        } else if (_page == SET_HOURS) {
            inc(MS_HOUR);
        }
    }

    void actionDown() {
        if (_page == SET_MINUTES) {
            dec(MS_MINUTE);
        } else if (_page == SET_HOURS) {
            dec(MS_HOUR);
        }
    }

    void callback() {
        ++_msCount;
        _msCount *= (_msCount < MS_DAY);
    }

    const unsigned int seconds() const {
        return (_msCount / MS_SECOND) % 60;
    }

    const unsigned int minutes() const {
        return (_msCount / MS_MINUTE) % 60;
    }

    const unsigned int hours() const {
        return (_msCount / MS_HOUR) % 24;
    }

    void show(const int row, const int col, const int blank = -1) const {
        char buff[8];
        sprintf(buff, "%02d:%02d:%02d", hours(), minutes(), seconds());
        if (blank >= 0) {
            buff[blank] = ' ';
            buff[blank + 1] = ' ';
        }
        lcd.setCursor(col, row);
        lcd.write(buff);
    }

    void display() {
        auto blank = -1;
        if (2 * (_msCount % MS_SECOND) < MS_SECOND) {
            if (_page == SET_MINUTES) {
                blank = 3;
            } else if (_page == SET_HOURS) {
                blank = 0;
            }
        }
        show(0, 0, blank);
    };

    unsigned int numPages() {
        return 3;
    };
};

class Irrigator : public Screen {
  private:
    enum STATES { SET_TIMES_PER_DAY, SET_SECONDS_PER_WATER };
    unsigned int _timesPerDay;
    unsigned int _secondsPerWater;

  public:
    void actionUp() {
        if ((_page == SET_TIMES_PER_DAY) && (_timesPerDay < 10)) {
            ++_timesPerDay;
        } else if ((_page == SET_SECONDS_PER_WATER) && (_secondsPerWater < 1000)) {
            _secondsPerWater += 10;
        }
    }

    void actionDown() {
        if ((_page == SET_TIMES_PER_DAY) && _timesPerDay) {
            --_timesPerDay;
        } else if (_page == SET_SECONDS_PER_WATER) {
            _secondsPerWater -= (_secondsPerWater > 10) ? 10 : _secondsPerWater;
        }
    }

    void display(){

    };

    unsigned int numPages() {
        return 2;
    };
};

class Display {
  public:
    static const unsigned int MAX_DISPLAYS = 4;
    unsigned int _numScreens = 0;
    Screen* _screens[MAX_DISPLAYS];
    unsigned int _idx;

  public:
    void addScreen(Screen& display) {
        if (_numScreens <= MAX_DISPLAYS - 1) {
            _screens[_numScreens++] = &display;
        }
    }

    Screen& currentScreen() const {
        return *_screens[_idx];
    }

    void firstScreen() {
        _idx = 0;
        currentScreen().firstPage();
    }

    void next() {
        if (currentScreen().next() && (++_idx >= _numScreens)) {
            _idx = 0;
        }
    }
};

// BUTTONS
Button btnBlack(10);
Button btnGreen(11);
Button btnOrange(12);

Backlight backlight(3);

Screen1 d1;
Screen2 d2;
Time time;
Display display;

// Timer 0 Interrupt is called once a millisecond
SIGNAL(TIMER0_COMPA_vect) {
    btnBlack.callback();
    btnGreen.callback();
    btnOrange.callback();
    backlight.callback();
    time.callback();
}

void setup() {
    lcd.begin(16, 2);
    backlight.begin();
    btnBlack.begin();
    btnGreen.begin();
    btnOrange.begin();

    // IO
    pinMode(pump, OUTPUT);

    // INTERUPTS
    // Timer0 is already used for millis()
    // set up  "TIMER0_COMPA_vect" interrupt function
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);

    // OTHER
    digitalWrite(pump, pumpState);
    display.addScreen(time);
    display.addScreen(d1);
    display.addScreen(d2);
    display.firstScreen();
}

void loop() {
    if (!backlight.isOn() && (btnBlack.isSinglePressed() || btnGreen.isSinglePressed() ||
                              btnOrange.isSinglePressed())) {
        backlight.turnOn();
        display.firstScreen();
    }

    if (btnBlack.isSinglePressed()) {
        display.next();
    }

    if (btnOrange.isHeldRepeat() || btnOrange.isSinglePressed()) {
        display.currentScreen().actionUp();
    }

    if (btnGreen.isHeldRepeat() || btnGreen.isSinglePressed()) {
        display.currentScreen().actionDown();
    }

    display.currentScreen().display();

    delay(50);
}