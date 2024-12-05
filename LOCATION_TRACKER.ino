#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include <Thread.h>
#include <AltSoftSerial.h>

// SIM module pins
#define SIM_TX 7
#define SIM_RX 6

// GPS module pins
#define GPS_TX 9
#define GPS_RX 8

// Button pins
#define CALL_BUTTON 3
#define SMS_BUTTON 4

#define STAT_LED 2

// SoftwareSerial instances for SIM and GPS modules
SoftwareSerial simSerial(SIM_TX, SIM_RX);
AltSoftSerial gpsSerial;

// TinyGPS instance
TinyGPSPlus gps;
Thread smsthread = Thread();

volatile int smsTimer = 0;
String recv_number = "+2348023568576";
String loc = "https://maps.google.com/maps?q=loc:0.000000,0.000000";

bool call_button_state = false;
bool sms_button_state = false;

// Updates GPS location
void gps_update() {
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isValid()) {
    loc = "https://maps.google.com/maps?q=loc:"
          + String(gps.location.lat(), 6) + ","
          + String(gps.location.lng(), 6);
    // Serial.println(loc);
  }
}

// Sends AT commands to initialize the SIM module
void setup_sim() {
  simSerial.println("AT");
  updateSerial();
  simSerial.println("AT+CSQ");
  updateSerial();
}

// Sends an SMS message
void send_sms(const String& msg) {
  digitalWrite(STAT_LED, HIGH);
  Serial.println("Sending SMS...");

  simSerial.println("AT+CMGF=1");
  updateSerial();
  simSerial.println("AT+CMGS=\"" + recv_number + "\"");
  updateSerial();
  simSerial.print(msg);
  simSerial.write(26);  // End of message
  updateSerial();

  digitalWrite(STAT_LED, LOW);
}

// Makes a call to the specified number
void call_number() {
  digitalWrite(STAT_LED, HIGH);
  Serial.println("Calling...");

  simSerial.println("ATD" + recv_number + ";");
  updateSerial();
  delay(20000);              // Wait for 10 seconds
  simSerial.println("ATH");  // Hang up
  updateSerial();

  digitalWrite(STAT_LED, LOW);
}

// Clears serial buffer
void updateSerial() {
  delay(500);
  while (simSerial.available()) {
    simSerial.read();
  }
}

// SMS callback function for the thread
void smsCallback() {
  Serial.println("SMS Callback Triggered");
  send_sms(loc);
}

// Processes incoming SMS commands
void sms_data() {

  while (simSerial.available()) {
    String textMessage = simSerial.readString();
    delay(10);
    textMessage.trim();
    textMessage.toUpperCase();

    Serial.println("Received SMS: " + textMessage);

    if (textMessage.indexOf("GETSMS") >= 0) {
      send_sms(loc);
    } else if (textMessage.indexOf("GETCALL") >= 0) {
      call_number();
    }
  }
}

// Arduino setup function
void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600);
  simSerial.begin(2400);

  pinMode(CALL_BUTTON, INPUT);
  pinMode(SMS_BUTTON, INPUT);
  pinMode(STAT_LED, OUTPUT);

  smsthread.onRun(smsCallback);
  smsthread.setInterval(60000);  // Run every 60 seconds

  setup_sim();
}

// Arduino loop function
void loop() {
  gps_update();
  sms_data();

   if (smsthread.shouldRun())
    smsthread.run();

  // Check for call button press
  if (digitalRead(CALL_BUTTON) == HIGH) {
    if (!call_button_state) {
      Serial.println("Call button pressed");
      call_number();
      call_button_state = true;
    }
  } else {
    call_button_state = false;
  }

  // Check for SMS button press
  if (digitalRead(SMS_BUTTON) == HIGH) {
    if (!sms_button_state) {
      Serial.println("SMS button pressed");
      send_sms("Please, I need help. Below is my current location:\r\n" + loc);
      sms_button_state = true;
    }
  } else {
    sms_button_state = false;
  }
}
