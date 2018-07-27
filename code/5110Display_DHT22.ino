//5110 Screen 84x48 Pixels - DHT-Sensor
//Wilhelm Zeuschner, Started on 31.03.2018
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <Fonts\FreeSans9pt7b.h>
#include <Fonts\Picopixel.h>

#include "DHT.h"
#define DHTTYPE DHT22

#include <EEPROM.h>



//Pins
const byte dht_pin = 6;
const byte light_pin = A0;
const byte backlight_pin = 9;
const byte button_pin_1 = 2;
const byte button_pin_2 = 3;

const byte display_reset = 20;
const byte display_cs = 4;
const byte display_dc = 5;
//Otherwise Hardware SPI-Pins!

//Variables
int eeprom_bl = 0;    //EEPROM Address
int eeprom_ui = 2;

//extern float temp;
//extern float humid;
float temp;
float humid;

int light_value;

int light_trigger = 800;

volatile bool button_1_pressed = 0;
volatile bool button_2_pressed = 0;
byte backlight_state = 1;
byte graph_state = 1;

//Array holds values for the last 60 Seconds
int temp_seconds[60];
//Array holds values of the average Temp for the last 60 Minutes
int temp_minutes[60];
//Last 60 Hours, these values will be displayed
int temp_hours[60];

int seconds_counter = 0;
int minutes_counter = 0;
unsigned long last_millis = 0;
unsigned long last_millis_2 = 0;
volatile unsigned long last_millis_isr = 0;

Adafruit_PCD8544 d = Adafruit_PCD8544(display_dc, display_cs, display_reset);

DHT dht(dht_pin, DHTTYPE);

void setup() {
	
	pinMode(button_pin_1, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(button_pin_1), button_interrupt, FALLING);
	pinMode(button_pin_2, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(button_pin_2), button_interrupt, FALLING);
	pinMode(light_pin, INPUT);
	pinMode(backlight_pin, OUTPUT);
	digitalWrite(backlight_pin, 1);

	Serial.begin(115200);
	Serial.println("Gebaut von Wilhelm Zeuschner. Projekt begonnen am 31.03.2018. Uploaded: 27.07.2018");
	dht.begin();
  Serial.println(dht.readTemperature());
	d.begin();
	d.setFont(NULL);
	d.setContrast(60);
	d.setRotation(2);
	d.setTextSize(1);
	d.clearDisplay();
	d.setTextColor(BLACK);
	d.setCursor(0, 10);
	d.println("Temperatur- und Luftfeuchtigkeitsmessgeraet");
	d.display();
	//delay(1000);
	
	//Read Data from EEPROM
	backlight_state = EEPROM.read(eeprom_bl);
	graph_state = EEPROM.read(eeprom_ui);

	Serial.println(backlight_state);
	if (backlight_state == 1) {
		digitalWrite(backlight_pin, 1);
	}
	else if (backlight_state == 2) {
		digitalWrite(backlight_pin, 0);
	}


  //Fill Hour Array with readings to get better axis scaling
  for (int i = 0; i < 60; i ++) {
    temp_hours[i] = dht.readTemperature() * 10;
    delay(50);
  }  
}

void loop() {

	//Detect Button Press
	if (button_1_pressed) {
		if (backlight_state < 3) {
			backlight_state = backlight_state + 1;
		}
		else {
			backlight_state = 1;
		}
		EEPROM.update(eeprom_bl, backlight_state);
		button_1_action();		
	}
	if (button_2_pressed) {
		button_2_pressed = 0;
		Serial.println("Button2");
		if (graph_state < 3) {
			graph_state = graph_state + 1;
		}
		else {
			graph_state = 1;
		}
		EEPROM.update(eeprom_ui, graph_state);
	}

	//Timing of Displaying Values
	if (millis() - last_millis >= 1000) {
		last_millis = millis();
		measure();
		average();
		display_text();
		switch (graph_state) {
		case 1:
			draw_graph(temp_hours);
			break;
		
		case 2:
			draw_graph(temp_minutes);
			break;
	
		case 3:
			draw_graph(temp_seconds);
			break;
		}
		
	}
	//Dim Backlight
	if (backlight_state == 3) {
		if (millis() - last_millis_2 >= 100) {
			last_millis_2 = millis();
			dim_backlight();
		}
	}
}

void average() {

	//First off, shift Array for Seconds by one Place to the left
	for (int i = 0; i < 59; i++) {
		temp_seconds[i] = temp_seconds[i + 1];
	}
	temp_seconds[59] = round(temp * 10);
	//Increment Seconds
	if (seconds_counter < 60) {
		seconds_counter++;
	}	
	//One Minute has passed
	else {
		//Increment Minutes
		minutes_counter++;
		seconds_counter = 0;
		
		//Calculate Average for last 60 Seconds
		float temp_avg_seconds;
		temp_avg_seconds = 0;
		for (int i = 0; i < 60; i++) {
			temp_avg_seconds = temp_avg_seconds + temp_seconds[i];
		}
		temp_avg_seconds = temp_avg_seconds / 60;

		//Shift Array for Minutes by on Place to the left
		for (int i = 0; i < 59; i++) {
			temp_minutes[i] = temp_minutes[i + 1];
		}
		//Save Average
		temp_minutes[59] = round(temp_avg_seconds);

		//One Hour has passed
		if (minutes_counter >= 60) {
			minutes_counter = 0;

			//Calculate Average for last 60 Minutes
			float temp_avg_minutes;
			temp_avg_minutes = 0;
			for (int i = 0; i < 60; i++) {
				temp_avg_minutes = temp_avg_minutes + temp_minutes[i];
			}
			temp_avg_minutes = temp_avg_minutes / 60;

			//Shift Array for Hours by on Place to the left
			for (int i = 0; i < 59; i++) {
				temp_hours[i] = temp_hours[i + 1];
			}
			//Save Average
			temp_hours[59] = round(temp_avg_minutes);
		}
	}
}

void print_arrays() {
	Serial.println("Array for Seconds");
	for (int i = 0; i < 60; i++) {
		Serial.print(temp_seconds[i]);
		Serial.print(",");
	}
	Serial.println(" ");

	Serial.println("Array for Minutes");
	for (int i = 0; i < 60; i++) {
		Serial.print(temp_minutes[i]);
		Serial.print(",");
	}
	Serial.println(" ");

	Serial.println("Array for Hours");
	for (int i = 0; i < 60; i++) {
		Serial.print(temp_hours[i]);
		Serial.print(",");
	}
	Serial.println(" ");
}




void display_text() {
	//Display Readings
	d.clearDisplay();
	d.setFont(&FreeSans9pt7b);
	d.setCursor(0, 12);
	d.setTextSize(1);
	d.print("T.: ");
	d.setCursor(20, 12);
	if (temp >= 10) {
		d.print(String(temp, 1));
	}
	else {
		d.print(String(temp, 2));
	}
	d.setCursor(62, 12);
	d.print("C");
	d.drawCircle(59, 2, 2, BLACK);
	d.setFont(NULL);
	d.setCursor(0, 14);
	d.print("Lf.: ");
	d.setCursor(25, 14);
	if (humid < 100) {
		d.print(String(humid, 1));
	}
	else {
		d.print(String(humid, 0));
	}
	d.setCursor(50, 14);
	d.print("%");

	d.drawFastHLine(0, 22, 84, BLACK);

	//Show which Data is shown as Graph
	d.setFont(&Picopixel);
	d.setCursor(0, 37);
	switch (graph_state) {
	case 1:		
		d.print("H.:");
		break;

	case 2:
		d.print("M.:");
		break;

	case 3:
		d.print("S.:");
		break;
	}

	
	d.display();
	//delay(100);
	//print_arrays();
}

//This draws a Graph (Diagram) into the bottom Section of the Screen
void draw_graph(int array_of_data[60]) {
	int current_x = d.width() - 60;
	int current_y = 0;
	int current_height = 0;

	//Determine max. and min. Temperature
	//Set "unrealistic" Values which have to be overwriten
	float max_temperature = -500.1;		
	float min_temperature = 1000.1;

	for (int i = 0; i < 60; i++) {
		if (array_of_data[i] > max_temperature) {
			max_temperature = array_of_data[i];
		}
		if (array_of_data[i] < min_temperature) {
			min_temperature = array_of_data[i];
		}
	}
	//Better Visuals -> this should "raise" the Graph
	//int subtract = round(max_temperature * 1.2f);
	//if (subtract > 25) {
	//	subtract = 25;
	//}
	//min_temperature = min_temperature - subtract;
	//Avoid Crashes
	if (min_temperature == max_temperature) {
		min_temperature = min_temperature - 1;
	}
	
	//Serial.print("max:");
	//Serial.println(max_temperature);
	//Serial.print("min:");
	//Serial.println(min_temperature);

	
	//Draw Graph
	for (int i = 0; i < 60; i++) {
		current_height = map(array_of_data[i], min_temperature, max_temperature, 1, 24);		//Determine Height for this Temperature
		current_y = (48 - 24 + (24 - current_height));										//Determine corresponding y-Position
		d.drawFastVLine(current_x, current_y, current_height, BLACK);						//Draw Line
		current_x = current_x + 1;															//Increment x-Position
	}

	max_temperature = max_temperature / 10;
	min_temperature = min_temperature / 10;

	//Print Text
	d.setFont(&Picopixel);
	d.setCursor(0, 28);
	d.print(String(max_temperature, 1));
	d.setCursor(0, 47);
	d.print(String(min_temperature, 1));

	//Draw Arrows
	d.drawFastVLine(17, 24, 24, BLACK);
	d.fillTriangle(15, 26, 17, 24, 19, 26, BLACK);
	d.fillTriangle(15, 45, 17, 47, 19, 45, BLACK);

	d.display();
}

void button_1_action() {
	d.clearDisplay();
	d.setFont(NULL);
	d.setCursor(0, 0);
	d.print("Licht-Modus:");
	d.setFont(&FreeSans9pt7b);
	d.setCursor(0, 20);
	switch (backlight_state) {
	case 1:
		d.print("Immer\nAN");
		digitalWrite(backlight_pin, 1);
		break;
	case 2:
		d.print("Immer\nAUS");
		digitalWrite(backlight_pin, 0);
		break;
	case 3:
		d.print("Ange-\npasst");
		dim_backlight();
		break;
	}
	Serial.println(backlight_state);
	d.display();
	delay(1100);
	button_1_pressed = 0;
}

void dim_backlight() {
	//Backlight
	light_value = analogRead(light_pin);
	//Serial.println(light_value);
	//Tune Backlight only when the right Mode is selected
	if (backlight_state == 3) {
		if (light_value > light_trigger) {
			//Dark
			analogWrite(backlight_pin, 25);
		}
		else if (light_value <= light_trigger) {
			//Bright
			digitalWrite(backlight_pin, 1);
		}
	}
}

void button_interrupt() {
	if (millis() - last_millis_isr >= 400) {
		last_millis_isr = millis();
		if (digitalRead(button_pin_1) == 0) {
			button_1_pressed = 1;
		}
		else if (digitalRead(button_pin_2) == 0) {
			button_2_pressed = 1;
		}
	}
}

//Zero Padding function
//String zero_padder(String z_p) {
//	String zero_padded = z_p; //Variable to hold result
//	float current = z_p.toFloat(); //Integer for comparison
//	if (10 > current) {
//		zero_padded = "0" + z_p;
//	}
//	return zero_padded;
//}
