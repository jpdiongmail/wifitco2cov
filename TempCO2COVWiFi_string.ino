/*
   Code for communication between a module measuring
   temperature, CO2 and COV values
   nd an Android smartphone which displays these values
*/

#include <Arduino.h>
#include <WiFi.h>
// #include "WiFiAP.h"
#include <Adafruit_CCS811.h>
// #include <SoftwareSerial.h>
#include <Ethernet.h>
#include "deep_sleep_example.h"

#define rx 8   // pin for receiving datas
#define tx 9   // pin for sending datas
// #define state 9

#define BUTTON_PIN_BITMASK 0x200000000   // 2^33 in hex

RTC_DATA_ATTR int bootcount = 0;

// initialize sensor Adafruit CCS811 (eCO2)
Adafruit_CCS811 ccs;
bool ccsavail;           // test if ccs data available
bool ccserr;             // try read ccs data
unsigned int eco2 = 0;  // eCO2 value
String eco2str;          // string from eco2 value
unsigned long tvoc = 0;  // TVOC value
String tvocstr;          // string from tvoc value
// unsigned long co2av[32];  // cumulative rate calculation
unsigned long ico2 = 0;   // row in tab co2av
double temp = 0;       // temperature
String tempstr;       // string to display temperature
uint8_t humidity;     // humidity to calibrate the sensor
float offset = 20;    // temperature offset setting
String co2avcalcstr = "NA";
unsigned long co2avcalc = 0;  // var for CO2 average calculation
unsigned long co2avcalctmp = 0;  // var for CO2 average calculation

// initialize  sensor Adafruit MiCS524 (CO, alcohol...)
unsigned int a0val = 0;   // value read in analog pin A0
String a0str;             // char string from a0val
float alcohol = 0;        // float : takes too much memory, to convert a0val to alcohol concentration in blood
String alcoholstr;        // to display alcohol value
int readnbb = 0;   // readnbb bytes written to LCD
int colpos = 0;    // set column position to LCD
bool firstloop = true;          // to test if first loop
unsigned long nbloops = 1;      // number of loops of the program
String cmd = "";
String readval = "";
String answer = "";

String ssid = "SFR_18D0";  // "ESP_8439D";  // "SFR_18D0";
const char* password = "filou1105"; // NULL;  // "filou1105";
int status = WL_IDLE_STATUS;
int thisNet;
byte numSsid;
String apssid = "ESP_8439CD";
const char *appassword = NULL;
byte gateway[] = {10, 0, 0, 1};
byte subnet[] = {255, 255, 0, 0};

WiFiServer server(14551);
// EthernetServer ethserver(14551);
byte macaddr[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
byte ipaddr[] = {192, 168, 4, 1};
IPAddress ipandroid(192, 168, 1, 81);
IPAddress ip;
WiFiClient client;
byte clientip[] = {192, 168, 4, 2};
char *androidjpap = "Androidjp";
char *androidjppwd = "filou1105";
byte esp32macaddr[] = {0x84, 0xFC, 0xE6, 0x84, 0x39, 0xCC};
byte esp32ipaddr[] = {192, 168, 43, 190};

// TCP
const char *address = "192.168.4.1";
const int port = 14551;

//Are we currently connected?
boolean connected = false;
boolean first = true;

boolean flipflop;     // for test
int minusone = -1;    // for test
boolean test = true;  // for test
String bufrec = "?????";

const int ext_wakeup_pin_0 = D0;

/*
void deep_sleep_register_ext0_wakeup(void)
{
    if (test) Serial.print("Enabling EXT0 wakeup on pin GPIO");
    if (test) Serial.println(ext_wakeup_pin_0);

    ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(ext_wakeup_pin_0, 1));

    // Configure pullup/downs via RTCIO to tie wakeup pins to inactive level during deepsleep.
    // EXT0 resides in the same power domain (RTC_PERIPH) as the RTC IO pullup/downs.
    // No need to keep that power domain explicitly, unlike EXT1.
    ESP_ERROR_CHECK(rtc_gpio_pullup_dis(ext_wakeup_pin_0));
    ESP_ERROR_CHECK(rtc_gpio_pulldown_en(ext_wakeup_pin_0));
}
*/

/*
  Method to print the reason by which ESP32
  has been awaken from sleep
*/
/*
void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

static void deep_sleep_register_rtc_timer_wakeup(void)
{
  const int wakeup_time_sec = 20;
  if (test) Serial.print("Enabling timer wakeup = ");
  if (test) Serial.println(wakeup_time_sec);
  ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));
}
*/

// CO2 average calculation ******************************************

/*
  int co2_average(unsigned long co2) {
  // CO2 max and cumulative rate calculation :
  //     * 1 000 ppm instant value max before undesirable health effects
  //     * 15 000 ppm during 10 mn max before undesirable health effects : 30 000 pendant 15 mn
  //     * 40 000 ppm is the threshold for irreversible health effects
  int j = 0;
  int speakerpin = 9;   // to activate the speaker
  int freqtest = 800;   // 800 Hz
  int duration = 1000;  // 3 seconds

  if (test) Serial.println("co2 ico2");
  if (test) Serial.println(co2);
  if (test) Serial.println(ico2);

  co2avcalctmp = co2avcalctmp + eco2;
  delay(300);
  return(0);

  if (ico2 > 29)
    ico2 = 0;

  co2av[ico2] = co2;
  ico2++;

  // calculate average
  co2avcalc = 0;

  if (nbloops < 29)
  {
    co2avcalcstr = ("NA");
    return(0);
  }

  for (j = 0; j < 30; j++)
    co2avcalc += co2av[j];
  co2avcalc = co2avcalc / 30;

  co2avcalcstr = String(co2avcalc);
  if (test) Serial.print("eco2 moyen ppm = ");
  if (test) Serial.println(co2avcalc);
  if (test) Serial.println(co2avcalcstr);
  }
*/

// wifi event handler ***********************************

void WiFiEvent(WiFiEvent_t event) {

  if (test) Serial.print("Event received = ");
  if (test) Serial.println(event);

  switch (event) {
    case IP_EVENT_STA_GOT_IP :
      //When connected set
      if (test) Serial.print("WiFi connected ! IP address: ");
      if (test) Serial.println(WiFi.localIP());
      connected = true;
      break;
    case WIFI_EVENT_STA_DISCONNECTED :
      if (test) Serial.println("WiFi lost connection");
      connected = false;
      break;
    case WIFI_EVENT_STA_STOP :
      if (test) Serial.println("WiFi AP stoppped. Restarting...");
      server.begin();
      connected = false;
      break;
    case WIFI_EVENT_STA_WPS_ER_FAILED :
      if (test) Serial.println("WiFi AP fails in enrollee mode. Restarting...");
      server.begin();
      connected = false;
      break;
    case 13 :
      if (test) Serial.println("WiFi AP starts. Restarting...");
      ip = WiFi.softAPIP();
      server.begin();
      if (test) Serial.print("Adresse IP : ");
      if (test) Serial.println(ip);
      connected = false;
      break;
    case 14 :
      if (test) Serial.println("WiFi AP stops. Restarting...");
      WiFi.mode(WIFI_AP);  // WIFI_STA   WIFI_AP   WIFI_AP_STA : ESP_8439CD
      WiFi.softAP(apssid, appassword);
      ip = WiFi.softAPIP();
      server.begin();
      // Ethernet.begin(macaddr, ipaddr);
      if (test) Serial.print("Adresse IP : ");
      if (test) Serial.println(ip);
      connected = false;
      break;
    default :
      if (test) Serial.print("Other wifi event : ");
      if (test) Serial.println(event);
      break;
  }
}

// Test function ************************************************

void testfunction()
{
  tempstr = "21";        // String(temp);
  // eco2str = "407";       // String(eco2);
  co2avcalcstr = "418";  // String(co2avcal);

  if (flipflop == false)
  {
    tvocstr = "17";        // String(tvoc);
    eco2str = "407";
    flipflop = true;
  }
  else
  {
    tvocstr = "28";     // String(tvoc);
    eco2str = "432";
    flipflop = false;
  }
  // tvocstr = "26";        // String(tvoc);
}

// Setup ******************************************

void setup() {
  // put your setup code here, to run once:
  int j;
  int i;

  //Increment boot number and print it every reboot
  ++bootcount;
  if (test) Serial.println("Boot number: " + String(bootcount));

  //Print the wakeup reason for ESP32
  // print_wakeup_reason();
  
  // pinMode(ext_wakeup_pin_0, INPUT_PULLUP);
  // esp_deep_sleep_enable_gpio_wakeup(BIT(D1), ESP_GPIO_WAKEUP_GPIO_LOW);

  // initialize sensor Adafruit CCS811
  // uint8_t addr = (90);
  // TwoWire *theWire = Wire;
  if (!ccs.begin()); // addr, &Wire);
  delay(1000);

  // calibrate the sensor CCS811 for temperature and humidity
  // ccs.setEnvironmentalData(40, 20);
  // for (j = 0; j < 60; j++)
  // co2av[j] = 0;

  // Serial line
  if (test) Serial.begin(115200);
  if (test) Serial.println("=======================");
  if (test) Serial.println("Serial line TCO2COV OK 115200");

  // scan for nearby networks:
  if (test) Serial.println("** Scan Networks **");
  numSsid = WiFi.scanNetworks();

  // print the list of networks seen:
  if (test) Serial.print("SSID List:");
  if (test) Serial.println(numSsid);
  // print the network number and name for each network found:
  for (thisNet = 0; thisNet < numSsid; thisNet++)
  {
    if (test) Serial.print(thisNet);
    if (test) Serial.print(") Network: ");
    if (test) Serial.println(WiFi.SSID(thisNet));
  }

  if (test) Serial.println("Starting AP...");
  WiFi.mode(WIFI_AP);  // WIFI_STA   WIFI_AP   WIFI_AP_STA : ESP_8439CD
  WiFi.setMinSecurity(WIFI_AUTH_WPA_PSK);

  WiFi.onEvent(WiFiEvent);
  if (test) Serial.println("Working as an AP for smartphone to connect...");
  // if (test) Serial.println("Working as a station to connect to smartphone...");
  WiFi.softAP(apssid, appassword);
  ip = WiFi.softAPIP();
  if (test) Serial.print("Adresse IP : ");
  if (test) Serial.println(ip);
  if (test) Serial.print("AP ssid : ");
  if (test) Serial.println(apssid);
  if (test) Serial.print("AP password : ");
  if (test) Serial.println(appassword);

  server.begin();
  if (test) Serial.println("Server running");
  // if (test) flipflop = false;
  client = server.available();

  if (!client)
  {
    if (test) Serial.print("No client connected");
    if (test) Serial.println(client);
  }
  else
  {
    if (test) Serial.print("Client connected : ");
    if (test) Serial.println(client);
  }
  if (test) Serial.println("-----");
  
  /* Enable wakeup from deep sleep by rtc timer */
  // deep_sleep_register_rtc_timer_wakeup();
  // deep_sleep_register_ext0_wakeup();
}

// Loop ******************************************

void loop() {
  // put your main code here, to run repeatedly:
  int err;
  unsigned long time = 0;         // time duration of the program
  unsigned long timeloop = 120000; // time of one loop of the program in ms
  long delayloop = 0;    // delay time to make loop duration 1 s
  int nbbw = 0;
  int nbbr = 0;
  // char readbuf[100];
  int i;
  int nbbytes = 0;

  if (test) Serial.print("===================== Entering loop nb ");
  if (test) Serial.println(nbloops);

  // test if any data available on sensor Adafruit CCS811 (eCO2)
  if (ccsavail = ccs.available()) {
    if (!(ccserr = ccs.readData())) {
      if (test) Serial.println("Getting eCO2 and TVOC...");
      eco2 = ccs.geteCO2();  // eCO2 value
      tvoc = ccs.getTVOC();  // TVOC value
      // err = co2_average(eco2);
      if (test) Serial.print("eco2 = ");
      if (test) Serial.println(eco2);
    }
  }

  co2avcalctmp = co2avcalctmp + eco2;

  if ((nbloops % 10)  == 0)
  {
    co2avcalc = co2avcalctmp / 10;
    co2avcalctmp = 0;
    if (test) Serial.print("co2avcalc = ");
    
    if (test) Serial.println(co2avcalc);

    // delay(2000);
    // Go to sleep now
    // if (test) Serial.println("Going to sleep now");
    // esp_deep_sleep_start();
  }

  if (nbloops > 9)
    co2avcalcstr = String(co2avcalc);
  else
    co2avcalcstr = ("NA");

  if (test) Serial.print("eco2 moyen ppm = ");
  if (test) Serial.println(co2avcalc);
  if (test) Serial.println(co2avcalcstr);

  eco2str = String(eco2);
  tvocstr = String(tvoc);

  // read value of MiCS524
  // a0val = analogRead(A0);
  // If alcohol, conversio is 131 ppm = 0,5 g/l of blood
  // alcohol = ((float)a0val / 131) * 0.5;
  // alcoholstr = String(alcohol, 1);

  // temperature
  if (firstloop == true) {
    temp = ccs.calculateTemperature();
    ccs.setTempOffset(temp - offset);
    firstloop = false;
  } else {
    temp = ccs.calculateTemperature();
    delay(1000);
  }
  tempstr = String(temp, 1);

  time = millis();
  delayloop = (long)((nbloops * timeloop) - time);
  if (delayloop > 0) {
    delay(delayloop);
  }
  nbloops += 1;

  // Send datas from ESP_32C3 to smartphone via wifi
  connected = true;

  if (test) Serial.println("Sending datas from ESP_32C3 to smartphone via TCP and wifi...");
  // if (test) testfunction();

  // Send Temp-CO2-CO2AV-COV to ESP32
  if (test) Serial.println("Datas :");

  cmd = "";
  cmd.concat(tempstr);
  cmd.concat("-");
  cmd.concat(eco2str);
  cmd.concat("-");
  cmd.concat(co2avcalcstr);
  cmd.concat("-");
  cmd.concat(tvocstr);
  cmd.concat("-");
  
  if (test) Serial.print("cmd : ");
  if (test) Serial.println(cmd);
  if (test) Serial.println("-----");
  if (test) Serial.println("Sending datas to client");
  
  client = server.available();
  nbbytes = client.println(cmd);
  // nbbytes = server.println(cmd);
  if (test) Serial.print("Nb bytes sent to client : ");
  if (test) Serial.println(nbbytes);
}
