/*--------------------------------------------------------------------------------------------------------------------------------------------------------
 * 
 * CHECK THE CODE FOR "TODO:" AND EDIT APPROPRIATELY 
 * 
 * All measurements are in SI units unless otherwise specified
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE
 * 
 * Code written by isaac879
 * 
 * Last modified 06/02/2019
 *--------------------------------------------------------------------------------------------------------------------------------------------------------*/

#include <Iibrary.h>//A library I created for Arduino that contains some simple functions I commonly use. Library available at: https://github.com/isaac879/Iibrary

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

//TODO:
//HC-05 baud rate
//hc-05 name
//firmware version

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

#define RED_PIN PD3
#define GREEN_PIN PD5
#define BLUE_PIN PD6

#define MAX_STRING_LENGTH 10

#define POSITIVE 1
#define NEGATIVE -1

#define ON true
#define OFF false

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

//Global variables
unsigned int base_loop_count = 200;

unsigned int phase_offset = 10;

float red_duty = 8.0;
float green_duty = 8.0;
float blue_duty = 8.0;
float base_duty = 100.0;

unsigned int red_duty_value = (float)base_loop_count * red_duty / 100.0;//switching value to achieve the desired duty cycle
unsigned int green_duty_value = (float)base_loop_count * green_duty / 100.0;//switching value to achieve the desired duty cycle
unsigned int blue_duty_value = (float)base_loop_count * blue_duty / 100.0;//switching value to achieve the desired duty cycle

unsigned int red_max_count = base_loop_count;
unsigned int green_max_count = base_loop_count;
unsigned int blue_max_count = base_loop_count;

unsigned int base_count = 0;
unsigned int red_count = 0;
unsigned int green_count = 0;
unsigned int blue_count = 0;

int number_of_increments = 100;

String stringText = "";
int flag_hue_direction = 1;
int flag_saturation_direction = 1;
int flag_value_direction = 1;
bool flag_direction = 0;

bool flag_random_hue = true;
bool flag_random_saturation = false;
bool flag_random_value = false;

bool flag_ISR = 0;

bool flag_flashing_hsv = 0;

short mode_status = 0;//just used for a status reprot
float base_loop_count_temp = base_loop_count * 0.00277777777777777777777;

int incCount = 0; //keeps track of number of function calls

float circle_hue_increment = 0.2;
float hue_increment = 0.2;
float saturation_increment = 0.01;
float value_increment = 0.01;

HSV hsv;
HSV hsv_start;
HSV hsv_end;

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

void serialFlush(){
    while(Serial.available() > 0) {
        char c = Serial.read();
    }
} 

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

void setBaseStrobeFrequency(unsigned int targetFreq){
    if(targetFreq < 0 || targetFreq > 65535){
        //printi("ERROR: loop count out of range (0-65,535). Value entered: ", (long)targetFreq);
    }
    else{
        int diff = targetFreq - base_loop_count;
        base_loop_count = targetFreq;
        base_loop_count_temp = base_loop_count * 0.00277777777777777777777;
        
        red_max_count += diff;
        red_duty_value = (float)red_max_count * red_duty / 100.0;
        green_max_count += diff;
        green_duty_value = (float)green_max_count * green_duty / 100.0;
        blue_max_count += diff;
        blue_duty_value = (float)blue_max_count * blue_duty / 100.0;
        //printi("Loop count: ", base_loop_count);
        //printi("The ISR cannot set the flag more than once a loop.");
    }
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

void setSingleDuty(int duty, char led){
    if(duty < 0 || duty > 100){
        //printi("ERROR: Duty must be between 0-100%. Duty entered: ", duty);
    }    
    else if(led == 'r' || led == 'R' ){
        red_duty = duty;
        red_duty_value = (float)base_loop_count * red_duty / 100.0;//switching value to achieve the desired duty cycle
        //printi("Red duty set to: ", duty);
    }
    else if(led == 'g' || led == 'G' ){
        green_duty = duty;
        green_duty_value = (float)base_loop_count * green_duty / 100.0;//switching value to achieve the desired duty cycle
        ////printi("Green duty set to: ", duty);
    }
    else if(led == 'b' || led == 'B' ){
        blue_duty = duty;
        blue_duty_value = (float)base_loop_count * blue_duty / 100.0;//switching value to achieve the desired duty cycle
        //printi("Blue duty set to: ", duty);
    }
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

void setLedDuty(int duty){
    if(duty < 0 || duty > 100){
        //printi("ERROR: Duty must be between 0-100%. Duty entered: ", duty);
    }
    else{
        base_duty = duty;
        setSingleDuty(duty, 'R');
        setSingleDuty(duty, 'G');
        setSingleDuty(duty, 'B');
    }
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

void coloursOn(bool redOn, bool greenOn, bool blueOn){
    if(redOn){
        setSingleDuty(base_duty, 'R');
    }
    else{
        setSingleDuty(0, 'R');
    }
    
    if(greenOn){
        setSingleDuty(base_duty, 'G');
    }
    else{
        setSingleDuty(0, 'G');
    }
    
    if(blueOn){
        setSingleDuty(base_duty, 'B');
    }
    else{
        setSingleDuty(0, 'B');
    }
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

void fadeBetweenHSV(void){
    hsv.h = hsv_start.h + hue_increment * incCount;
    hsv.s = hsv_start.s + saturation_increment * incCount;
    hsv.v = hsv_start.v + value_increment * incCount;
    
    if(hsv.h > 360){//Hue wrap around
        hsv.h -= 360;
    }
    else if(hsv.h < 0){
        hsv.h += 360;
    }

    hsv.s = boundFloat(hsv.s, 0, 1);
    hsv.v = boundFloat(hsv.v, 0, 1);

    incCount += 1 + (-2 * flag_direction);

    if(incCount >= number_of_increments || incCount <= 0){
        flag_direction = !flag_direction;
    }
    setIhsv(hsv.h, hsv.s, hsv.v);   
    flag_ISR = false;
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

void hsvTohsvSynchronization(int numCycles){
    if(numCycles > 0 && numCycles <= 65535){
        number_of_increments = numCycles;
        incCount = 0;
        flag_direction = 0;
        hue_increment = (hsv_end.h - hsv_start.h) / numCycles;
        if(hue_increment < 0){
            hue_increment += 360.0 / (float)numCycles;
        }        
        saturation_increment = (hsv_end.s - hsv_start.s) / numCycles;
        value_increment = (hsv_end.v - hsv_start.v) / numCycles;
        //printi("hue_increment: ", hue_increment);
        //printi("saturation_increment: ", saturation_increment);
        //printi("value_increment: ", value_increment);
    }
    else{
        //printi("numCycles must be between 1-65535... Value entered: ", numCycles);
    }   
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

ISR(TIMER1_COMPA_vect){//timer1 interrupt
    flag_ISR = true;
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

void circleHue(void){
    setIhsv(hsv.h, hsv.s, hsv.v);
    hsv.h += circle_hue_increment;
    if(hsv.h >= 360){
        hsv.h -= 360;
    }
    else if(hsv.h < 0){
        hsv.h += 360;
    }
    flag_ISR = false;
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

void setMode(int mode){
    mode_status = mode;
    red_count = 0;
    green_count = 0;
    blue_count = 0;
    base_count = 0;
    setIhsv(0, 1, 1);
    red_max_count = base_loop_count;
    green_max_count = base_loop_count;
    blue_max_count = base_loop_count;
    setLedDuty(base_duty);
    TIMSK1 &= ~(1 << OCIE1A);// disable timer compare interrupt
    flag_ISR = false;
    
    switch(mode){
        case 0: //Red
            coloursOn(OFF, OFF, OFF);
            break;
        case 1: //Red
            coloursOn(ON, OFF, OFF);
            break;
        case 2://green
            coloursOn(OFF, ON, OFF);
            break;
        case 3://blue
            coloursOn(OFF, OFF, ON);
            break;
        case 4://yellow
            coloursOn(ON, ON, OFF);
            break;
        case 5://purple
            coloursOn(ON, OFF, ON);
            break;
        case 6://cyan
            coloursOn(OFF, ON, ON);
            break;
        case 7://white
            coloursOn(ON, ON, ON);
            break;
        case 8://red green blue rainbow
            coloursOn(ON, ON, ON);
            red_count = base_loop_count;
            green_count = base_loop_count / 3;
            blue_count = base_loop_count * 2 / 3;
            setSingleDuty(45, 'R');
            setSingleDuty(45, 'G');
            setSingleDuty(45, 'B');
            break;
        case 9://red green blue rainbow
            coloursOn(ON, ON, ON);
            red_count = base_loop_count;
            green_count = base_loop_count / 3;
            blue_count = base_loop_count * 2 / 3;
            setSingleDuty(70, 'R');
            setSingleDuty(70, 'G');
            setSingleDuty(70, 'B');
            break;  
        case 10://red cyan flow
            red_count = base_loop_count / 2;
            coloursOn(ON, ON, ON);
            setSingleDuty(70, 'R');
            setSingleDuty(70, 'G');
            setSingleDuty(70, 'B');
            break;
        case 11://green blue flow
            green_count = base_loop_count / 2;
            coloursOn(OFF, ON, ON);
            setSingleDuty(70, 'G');
            setSingleDuty(70, 'B');
            break;
        case 12://green purple flow
            green_count = base_loop_count / 2;
            coloursOn(ON, ON, ON);
            setSingleDuty(70, 'R');
            setSingleDuty(70, 'G');
            setSingleDuty(70, 'B');
            break;
        case 13://blue yellow flow
            blue_count = base_loop_count / 2;
            coloursOn(ON, ON, ON);
            setSingleDuty(70, 'R');
            setSingleDuty(70, 'G');
            setSingleDuty(70, 'B');
            break;
        case 14://fade hues
            setISR_frequency(40);
            hsvTohsvSynchronization(number_of_increments);
            coloursOn(ON, ON, ON);
            TIMSK1 |= (1 << OCIE1A);// enable timer compare interrupt
            break;
        case 15://random hues
            setISR_frequency(3);
            coloursOn(ON, ON, ON);
            TIMSK1 |= (1 << OCIE1A);// enable timer compare interrupt
            break;
        case 16://circle hues
            setISR_frequency(40);
            coloursOn(ON, ON, ON);
            TIMSK1 |= (1 << OCIE1A);// enable timer compare interrupt
            break;
        case 17://flash hues
            setISR_frequency(3);
            coloursOn(ON, ON, ON);
            TIMSK1 |= (1 << OCIE1A);// enable timer compare interrupt
            break;
        case 18://User set mode
            setIhsv(hsv.h, hsv.s, hsv.v);
        break;
        case 19://Random flashing HSV with off between colours
            setISR_frequency(3);
            coloursOn(ON, ON, ON);
            TIMSK1 |= (1 << OCIE1A);// enable timer compare interrupt
            randomFlashingHSV(flag_random_hue, flag_random_saturation, flag_random_value);
        break;
    }
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

float boundFloat(float value, float lower, float upper){
    if(value < lower){
        value = lower;
    }
    else if(value > upper){
        value = upper;
    }
    return value;
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

int boundInt(int value, int lower, int upper){
    if(value < lower){
        value = lower;
    }
    else if(value > upper){
        value = upper;
    }
    return value;
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

void setIhsv(float hue, float saturation, float value){//hue 0 to 360, saturation 0-1, value 0-1
    float rTemp;
    if(hue >= 240){
        rTemp = boundFloat(mapNumber(abs(hue - 360), 0, 120, 100, 0), 0, 100); 
    }
    else{
        rTemp = boundFloat(mapNumber(hue, 0, 120, 100, 0), 0, 100);
    }
    float gTemp = boundFloat(mapNumber(abs(hue - 120), 0, 120, 100, 0), 0, 100);
    float bTemp = boundFloat(mapNumber(abs(hue - 240), 0, 120, 100, 0), 0, 100);

    rTemp = boundFloat(rTemp + (100.0f - saturation * 100.0f), 0 , 100);
    gTemp = boundFloat(gTemp + (100.0f - saturation * 100.0f), 0 , 100);
    bTemp = boundFloat(bTemp + (100.0f - saturation * 100.0f), 0 , 100);
    
    rTemp *= value;//Linearly scales brightness
    gTemp *= value;
    bTemp *= value;

    red_duty_value = (float)base_loop_count * rTemp / 100.0;//switching value to achieve the desired duty cycle
    green_duty_value = (float)base_loop_count * gTemp / 100.0;//switching value to achieve the desired duty cycle
    blue_duty_value = (float)base_loop_count * bTemp / 100.0;//switching value to achieve the desired duty cycle
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

void setISR_frequency(float hz){
    if(hz < 0.239 || hz > 15624){
        //printi("Frequency out of the range 0.239 - 15624");
        return;
    }
    unsigned long count = 16000000.0f / (hz * 1024.0f) - 1;
    unsigned int val= boundInt(count, 0, 15624);
    //printi("Frequency set to: ", hz, 3, "Hz\n");
    TCNT1  = 0;//initialize counter value to 0
    OCR1A = val;//m16000000 / (1*1024) - 1;// = (16*10^6) / (36*1024) - 1 (must be <65536)  // set compare match register for 25hz increments
    //printi("The ISR cannot set the flag more than once a loop.\n");
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

void randomHSV(bool randomHue, bool randomSat, bool randomValue){
    if(randomHue){
        hsv.h = random(0, 360);
    }
    if(randomSat){
        hsv.s = (float)random(0, 256) / 255.0f;
    }
    if(randomValue){
        hsv.v = (float)random(0, 256)  / 255.0f;
    }
    setIhsv(hsv.h, hsv.s, hsv.v);
    flag_ISR = false;
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

void randomFlashingHSV(bool randomHue, bool randomSat, bool randomValue){
    hsv.v = flag_flashing_hsv;
    if(flag_flashing_hsv){
        if(randomHue){
            hsv.h = random(0, 360);
        }
        if(randomSat){
            hsv.s = (float)random(0, 256) / 255.0f;
        }
        if(randomValue){
            hsv.v = (float)random(0, 256)  / 255.0f;
        }
    }
    flag_flashing_hsv = !flag_flashing_hsv;
    setIhsv(hsv.h, hsv.s, hsv.v);
    flag_ISR = false;
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

void flashColours(void){
    if(hsv.h == hsv_start.h && hsv.s == hsv_start.s && hsv.v == hsv_start.v){
        hsv.h = hsv_end.h;
        hsv.s = hsv_end.s;
        hsv.v = hsv_end.v;
    }else{
        hsv.h = hsv_start.h; 
        hsv.s = hsv_start.s; 
        hsv.v = hsv_start.v; 
    }
    setIhsv(hsv.h, hsv.s, hsv.v);
    flag_ISR = false;
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

void intruderAlert(void){
    hsv_start.h = 0;//Set default starting HSV values
    hsv_start.s = 1;
    hsv_start.v = 1;

    hsv_end.h = 0;//Set default ending HSV values
    hsv_end.s = 1;
    hsv_end.v = 0;
    setMode(14);
    setISR_frequency(100);
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

void setup(){
    Serial.begin(9600);//HC-05 Bluetooth module serial baud rate
    
    pinMode(RED_PIN, OUTPUT); //Initiaise pins as outputs
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    
    randomSeed(analogRead(0));//read analog pin to get a random seed
    
    PORTD &= ~_BV(RED_PIN);//Sets digital pin low
    PORTD &= ~_BV(GREEN_PIN);//Sets digital pin low
    PORTD &= ~_BV(BLUE_PIN);//Sets digital pin low  
    
    hsv.h = 0;//Set initial HSV values
    hsv.s = 1;
    hsv.v = 1;

    hsv_start.h = 0;//Set default starting HSV values
    hsv_start.s = 1;
    hsv_start.v = 1;

    hsv_end.h = 40;//Set default ending HSV values
    hsv_end.s = 1;
    hsv_end.v = 1;

    //set timer1 interrupt at 25Hz
    TCCR1A = 0;// set entire TCCR1A register to 0
    TCCR1B = 0;// same for TCCR1B
    TCNT1  = 0;//initialize counter value to 0
    //OCR1A = 625;//m16000000 / (1*1024) - 1;// = (16*10^6) / (36*1024) - 1 (must be <65536)  // set compare match register for 25hz increments
    //setISR_frequency(5);
    TCCR1B |= (1 << WGM12);// turn on CTC mode
    TCCR1B |= (1 << CS12) | (1 << CS10);  // Set CS12 and CS10 bits for 1024 prescaler
    setMode(16);//Initial mode to start in when powered on
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/

void loop(){   
    if(Serial.available()){//Checks if serial data is available
        delay(10);//wait to make sure all data in the serial message has arived
        char c = Serial.read();//read and store the first character sent
        stringText = "";//clear stringText
        while(Serial.available()){//set elemetns of stringText to the serial values sent
            char digit = Serial.read();
            stringText += digit; 
            if(stringText.length() >= MAX_STRING_LENGTH) break;//exit the loop when the stringText array is full
        }
        serialFlush();//Clear any excess data in the serial buffer
        
        int serialCommandValueInt = stringText.toInt();
        float serialCommandValueFloat = stringText.toFloat();

        switch(c){
            case 'f'://set base strobe frequency
                setBaseStrobeFrequency(serialCommandValueInt);
            break;
            case 'F':
                setISR_frequency(serialCommandValueFloat);
            break;
            case 'm'://set mode
                if(serialCommandValueInt >= 0 && serialCommandValueInt <= 19){//set maximum mode number
                    setMode(serialCommandValueInt);
                    //printi("Mode set to: ", serialCommandValueInt);
                }
                else{
                    //printi("Invalid mode number. Number entered: ", serialCommandValueInt);
                }
            break;
            case 'd':
                setLedDuty(serialCommandValueInt);
            break;
            case 'r':
                setSingleDuty(serialCommandValueInt, c);
            break;
            case 'g':
                setSingleDuty(serialCommandValueInt, c);
            break;
            case 'b':
                setSingleDuty(serialCommandValueInt, c);
            break;
            case 's':
                if(serialCommandValueFloat >= 0 && serialCommandValueFloat <= 360){
                    hsv_start.h = serialCommandValueFloat;
                    hsvTohsvSynchronization(number_of_increments);
                    //printi("hue_start: ", hsv_start.h);
                }
                else{
                    //printi("Valid range 0-360: Hue entered: ", serialCommandValueFloat); 
                }
            break;
            case 'e':
                if(serialCommandValueFloat >= 0 && serialCommandValueFloat <= 360){
                    hsv_end.h = serialCommandValueFloat;
                    hsvTohsvSynchronization(number_of_increments);
                    //printi("hue_end: ", hsv_end.h);
                }
                else{
                    //printi("Valid range 0-360: Hue entered: ", serialCommandValueFloat); 
                }
            break;
            case 'i'://hue increment
            if(serialCommandValueFloat >= -360 && serialCommandValueFloat <= 360){
                circle_hue_increment = serialCommandValueFloat;
                //printi("Circle Hue increment: ", circle_hue_increment); 
            }
            else{
                //printi("Valid range -360-360: Hue entered: ", serialCommandValueFloat); 
            }
            break;    
            case 'h':
                hsv.h = boundFloat(serialCommandValueFloat, 0, 360);
                setMode(18);
                //printi("Hue: ", serialCommandValueFloat); 
            break;
            case 'S':
                hsv.s = boundFloat(serialCommandValueFloat, 0, 1);
                setMode(18);
                //printi("Saturation: ", serialCommandValueFloat); 
            break;          
            case 'v':
                hsv.v = boundFloat(serialCommandValueFloat,0, 1);
                setMode(18);
                //printi("Value: ", serialCommandValueFloat); 
            break;
            case 'I':
                flag_random_hue = !flag_random_hue;
                //printi("flag_random_hue: ", flag_random_hue);
            break;
            case 'C':
                flag_random_saturation = !flag_random_saturation;
                //printi("flag_random_saturation: ", flag_random_saturation); 
            break;
            case 'V':
                flag_random_value = !flag_random_value;
                //printi("flag_random_value: ", flag_random_value);
            break;
            case 'a':
                if(serialCommandValueFloat >= 0 && serialCommandValueFloat <= 1){
                    hsv_start.s = serialCommandValueFloat;
                    hsvTohsvSynchronization(number_of_increments);
                    //printi("Saturation start value: ", hsv_start.s ); 
                }
                else{
                    //printi("Valid range 0-1: Value entered: ", serialCommandValueFloat); 
                }
            break;
            case 'A':
                if(serialCommandValueFloat >= 0 && serialCommandValueFloat <= 1){
                    hsv_end.s = serialCommandValueFloat;
                    hsvTohsvSynchronization(number_of_increments);
                    //printi("Saturation end value: ", hsv_end.s ); 
                }
                else{
                    //printi("Valid range 0-1: Value entered: ", serialCommandValueFloat); 
                }
            break;            
            case 'j':
                if(serialCommandValueFloat >= 0 && serialCommandValueFloat <= 1){
                    hsv_start.v = serialCommandValueFloat;
                    hsvTohsvSynchronization(number_of_increments);
                    //printi("Value start value: ", hsv_start.v ); 
                }
                else{
                    //printi("Valid range 0-1: Value entered: ", serialCommandValueFloat); 
                }
            break;            
            case 'J':
                if(serialCommandValueFloat >= 0 && serialCommandValueFloat <= 1){
                    hsv_end.v = serialCommandValueFloat;
                    hsvTohsvSynchronization(number_of_increments);
                    //printi("Value end value: ", hsv_end.v); 
                }
                else{
                    //printi("Valid range 0-1: Value entered: ", serialCommandValueFloat); 
                }
            break;
            case 'n':
                hsvTohsvSynchronization(serialCommandValueInt);
            break;
            case 'N':
                intruderAlert();
            break;
            case 'q':
                printi("____Status____\n");
                printi("Base loop count: ", base_loop_count, "\n\n"); 
                printi("Red loop count: ", red_max_count);
                printi("Red duty: ", red_duty, 1, "\n\n");
                printi("Green loop count: ", green_max_count);
                printi("Green duty: ", green_duty, 1, "\n\n");
                printi("Blue loop count: ", blue_max_count);
                printi("Blue duty: ", blue_duty, 1, "\n\n");
                printi("Mode: ", mode_status, "\n\n");
                printi("Circle Hue increment: ", circle_hue_increment);
                printi("Hue start: ", hsv_start.h); 
                printi("Hue end: ", hsv_end.h); 
                printi("Saturation start: ", hsv_start.s); 
                printi("Saturation end: ", hsv_end.s); 
                printi("Value start: ", hsv_start.v); 
                printi("Value end: ", hsv_end.v); 
                printi("Hue: ", hsv.h);
                printi("Saturation: ", hsv.s);
                printi("Value: ", hsv.v);
                printi("Random hue flag: ", flag_random_hue);
                printi("Random saturation flag: ", flag_random_saturation);
                printi("Random value flag: ", flag_random_value);
            break;
            case 'Q'://TODO: correct all values and ensure all commands are included they are correct
                printi("\n");
                printi("loop count: f\n");
                printi("Mode: m\n");
                printi("All LED duty cycles: d\n");
                printi("Red duty: r\n");
                printi("Green duty: g\n");
                printi("Blue duty: b\n");
                printi("Display current settings: H\n");
                printi("Hue increment: i\n");
                printi("Saturation: S\n");
                printi("Value: v\n");
                printi("Start hue value: s\n");
                printi("End hue value: e\n");
                printi("Random hue flag: I\n");
                printi("Random saturation flag: C\n");
                printi("Random value flag: V\n");
                printi("ISR frequency: F\n");
                printi("Saturation start: a\n");
                printi("Saturation end: A\n");
                printi("Value start: j\n");
                printi("Value end: J\n");
                printi("Fade increments: n\n");
            break;
            case 'M':
                printi("\n");
                printi("Modes:\n");
                printi("1-7: Single colours\n");
                printi("8-13: Colour flows\n");
                printi("14: Fade between colours\n");
                printi("15: Random HSV\n");
                printi("16: Circle hue\n");
                printi("17: Flash between colours\n");
                printi("18: User HSV\n");
                printi("19: Flash Random HSV\n");
            break;
        }
    }

    if(red_count < red_duty_value){//Switch LEDs on/off for the set duty/frequency
        PORTD |= _BV(RED_PIN);  //write port HIGH
    }
    else{
        PORTD &= ~_BV(RED_PIN);  //write port LOW
    }
    
    if(green_count < green_duty_value){
        PORTD |= _BV(GREEN_PIN);  //write port HIGH
    }
    else {
        PORTD &= ~_BV(GREEN_PIN);  //write port LOW
    }
    
    if(blue_count < blue_duty_value){
        PORTD |= _BV(BLUE_PIN);  //write port B5 HIGH
    }
    else {
        PORTD &= ~_BV(BLUE_PIN);  //write port B5 LOW
    }

    base_count++;//Increment counts  
    red_count++;
    green_count++;
    blue_count++;

    if(base_count >= base_loop_count){
        if(mode_status == 14 && flag_ISR){
            //fadeBetweenHues(hue_start, hue_end);//depricated
            fadeBetweenHSV();
        }
        else if(mode_status == 15 && flag_ISR){
            randomHSV(flag_random_hue, flag_random_saturation, flag_random_value);
        }
        else if(mode_status == 16 && flag_ISR){
            circleHue();
        }
        else if(mode_status == 17 && flag_ISR){
            flashColours();
        }
        else if(mode_status == 19 && flag_ISR){
            randomFlashingHSV(flag_random_hue, flag_random_saturation, flag_random_value);
        }
        base_count = 0;
    }
    if(red_count >= red_max_count){
        red_count = 0;
    }
    if(green_count >= green_max_count){
        green_count = 0;
    }
    if(blue_count >= blue_max_count){
        blue_count = 0;
    }  
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------*/
