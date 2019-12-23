/*
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * Example that shows how to parse a P1 message and automatically print
 * the result.
*/

#include <TimeLib.h>
#include "dsmr.h"
#include <ESP8266WiFi.h>        // version 1.0.0 - part of ESP8266 Core https://github.com/esp8266/Arduino
#include <ArduinoJson.h>
#include <FS.h>
#include "DebugS.h"

//#define READSLIMMEMETER
//#define DTR_ENABLE 12   // GPIO-pin to use for DTR


//-------------------------.........1....1....2....2....3....3....4....4....5....5....6....6....7....7
//-------------------------1...5....0....5....0....5....0....5....0....5....0....5....0....5....0....5
#define DATA_FORMAT       "%-8.8s;%10.3f;%10.3f;%10.3f;%10.3f;%10.3f;\n"
#define DATA_CSV_HEADER   "YYMMDDHH;      EDT1;      EDT2;      ERT1;      ERT2;       GDT;"
#define DATA_RECLEN       75
#define KEEP_DAYS_HOURS   3                     // number of days to keep!
#define HOURS_FILE        "/RINGhours.csv"
#define _NO_HOUR_SLOTS_   (KEEP_DAYS_HOURS * 24)
#define DAYS_FILE         "/RINGdays.csv"
#define KEEP_WEEK_DAYS    3                     // number of weeks to keep!  
#define _NO_DAY_SLOTS_    (KEEP_WEEK_DAYS * 7)
#define MONTHS_FILE       "/RINGmonths.csv"
#define KEEP_YEAR_MONTHS  1                     // number of years to keep!
#define _NO_MONTH_SLOTS_  (KEEP_YEAR_MONTHS * 12)

enum    { PERIOD_UNKNOWN, HOURS, DAYS, MONTHS, YEARS };


// Data to parse
const char msg[] =
  "/XMX5LGBBLB2410065887\r\n"
  "\r\n"
  "1-3:0.2.8(50)\r\n"
  "0-0:1.0.0(200408063501S)\r\n"
  "0-0:96.1.1(4530303336303000000000000000000040)\r\n"
  "1-0:1.8.1(000234.191*kWh)\r\n"
  "1-0:1.8.2(000402.930*kWh)\r\n"
  "1-0:2.8.1(000119.045*kWh)\r\n"
  "1-0:2.8.2(000079.460*kWh)\r\n"
  "0-0:96.14.0(0001)\r\n"
  "1-0:1.7.0(001.22*kW)\r\n"
  "1-0:2.7.0(001.11*kW)\r\n"
  "0-0:96.7.21(00010)\r\n"
  "0-0:96.7.9(00000)\r\n"
  "1-0:99.97.0(0)(0-0:96.7.19)\r\n"
  "1-0:32.32.0(00002)\r\n"
  "1-0:52.32.0(00003)\r\n"
  "1-0:72.32.0(00003)\r\n"
  "1-0:32.36.0(00000)\r\n"
  "1-0:52.36.0(00000)\r\n"
  "1-0:72.36.0(00000)\r\n"
  "0-0:96.13.0()\r\n"
  "1-0:32.7.0(241.0*V)\r\n"
  "1-0:52.7.0(237.0*V)\r\n"
  "1-0:72.7.0(235.0*V)\r\n"
  "1-0:31.7.0(000*A)\r\n"
  "1-0:51.7.0(000*A)\r\n"
  "1-0:71.7.0(000*A)\r\n"
  "1-0:21.7.0(00.536*kW)\r\n"
  "1-0:41.7.0(00.194*kW)\r\n"
  "1-0:61.7.0(00.487*kW)\r\n"
  "1-0:22.7.0(00.013*kW)\r\n"
  "1-0:42.7.0(00.611*kW)\r\n"
  "1-0:62.7.0(00.486*kW)\r\n"
  "0-1:24.1.0(003)\r\n"
  "0-1:96.1.0(4730303339303031363532303530323136)\r\n"
  "0-1:24.2.1(200408063501S)(00169.156*m3)\r\n"
  "!0876\r\n";

/**
 * Define the data we're interested in, as well as the datastructure to
 * hold the parsed data.
 * Each template argument below results in a field of the same name.
 */
using MyData = ParsedData<
  /* String */         identification
  /* String */        ,p1_version
  /* String */        ,timestamp
  /* String */        ,equipment_id
  /* FixedValue */    ,energy_delivered_tariff1
  /* FixedValue */    ,energy_delivered_tariff2
  /* FixedValue */    ,energy_returned_tariff1
  /* FixedValue */    ,energy_returned_tariff2
  /* String */        ,electricity_tariff
  /* FixedValue */    ,power_delivered
  /* FixedValue */    ,power_returned
  /* FixedValue */    ,electricity_threshold
  /* uint8_t */       ,electricity_switch_position
  /* uint32_t */      ,electricity_failures
  /* uint32_t */      ,electricity_long_failures
  /* String */        ,electricity_failure_log
  /* uint32_t */      ,electricity_sags_l1
  /* uint32_t */      ,electricity_sags_l2
  /* uint32_t */      ,electricity_sags_l3
  /* uint32_t */      ,electricity_swells_l1
  /* uint32_t */      ,electricity_swells_l2
  /* uint32_t */      ,electricity_swells_l3
  /* String */        ,message_short
  /* String */        ,message_long
  /* FixedValue */    ,voltage_l1
  /* FixedValue */    ,voltage_l2
  /* FixedValue */    ,voltage_l3
  /* FixedValue */    ,current_l1
  /* FixedValue */    ,current_l2
  /* FixedValue */    ,current_l3
  /* FixedValue */    ,power_delivered_l1
  /* FixedValue */    ,power_delivered_l2
  /* FixedValue */    ,power_delivered_l3
  /* FixedValue */    ,power_returned_l1
  /* FixedValue */    ,power_returned_l2
  /* FixedValue */    ,power_returned_l3
  /* uint16_t */      ,gas_device_type
  /* String */        ,gas_equipment_id
  /* uint8_t */       ,gas_valve_position
  /* TimestampedFixedValue */ ,gas_delivered
  /* uint16_t */      ,thermal_device_type
  /* String */        ,thermal_equipment_id
  /* uint8_t */       ,thermal_valve_position
  /* TimestampedFixedValue */ ,thermal_delivered
  /* uint16_t */      ,water_device_type
  /* String */        ,water_equipment_id
  /* uint8_t */       ,water_valve_position
  /* TimestampedFixedValue */ ,water_delivered
  /* uint16_t */      ,slave_device_type
  /* String */        ,slave_equipment_id
  /* uint8_t */       ,slave_valve_position
  /* TimestampedFixedValue */ ,slave_delivered
>;

#if defined(READSLIMMEMETER)
  #ifdef DTR_ENABLE
    P1Reader    slimmeMeter(&Serial, DTR_ENABLE);
  #else
    P1Reader    slimmeMeter(&Serial, 0);
  #endif
#endif


//===========================GLOBAL VAR'S======================================
  MyData      DSMRdata;
  DynamicJsonDocument jsonDoc(4000);  // generic doc to return, clear() before use!
  uint32_t    readTimer;
  char        actTimestamp[20] = "";
  char        newTimestamp[20] = "";
  char        cMsg[100]        = "";
  int         hr = 1;
  uint32_t    nrReboots  = 0;
  uint32_t    slotErrors = 0;
  uint64_t    upTimeSeconds;
  uint8_t     Verbose1, Verbose2;
  bool        spiffsNotPopulated;
  bool        updateStatus;

void setup() 
{
  Serial.begin(115200);
  DebugTln("\n\nAnd now it begins ...\n");

  DebugTln(ESP.getResetReason());
  if (   ESP.getResetReason() == "Exception" 
      || ESP.getResetReason() == "Software Watchdog"
      || ESP.getResetReason() == "Soft WDT reset"
      ) 
  {
    while (1) 
    {
      delay(500);
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
  }

  SPIFFS.begin();
  listSPIFFS();
  
  //strcpy(actTimestamp, "060504030201X");
  //writeLastStatus();  // only for firsttime initialization
  readLastStatus(); // place it in actTimestamp
  DebugTf("===>actTimestamp[%s]-> nrReboots[%u]-> Errors[%u]<======\r\n\n", actTimestamp
                                                                    , nrReboots++
                                                                    , slotErrors);

#if defined(READSLIMMEMETER)
  slimmeMeter.enable(true);
#else
  ParseResult<void> res = P1Parser::parse(&DSMRdata, msg, lengthof(msg));
  if (res.err) {
    // Parsing error, show it
    DebugTln(res.fullError(msg, msg + lengthof(msg)));
  } else if (!DSMRdata.all_present()) {
    DebugTln("DSMR: Some fields are missing");
  } 
  // Succesfully parsed, make JSON:
  //makeJson();
#endif
  DebugTln("=============================");
  DebugTf("DSMRdata.timestamp[%s]\r\n", DSMRdata.timestamp.c_str());
  strcpy(newTimestamp, actTimestamp); 

  //--- only first time --
  //strcpy(newTimestamp, DSMRdata.timestamp.c_str()); 
  
  DebugTf("newTimestamp[%s]\r\n", newTimestamp);
  time_t t = epoch(newTimestamp, strlen(newTimestamp), false);
  setTime(t);    
  //-- make newTimestamp the actTimestamp
  strncpy(actTimestamp, newTimestamp, sizeof(actTimestamp));
  writeLastStatus();

/**
  DebugTln("strCopy() test ======");
  char bufin[20] = "0123456789ABCDEFGHI";
  DebugTf(" in[%s]\r\n", bufin);
  char bufout[15] = "abcdef";
  DebugTf("out[%s]\r\n", bufout);
  strCopy(bufout, sizeof(bufout), bufin, 5, 10);
  DebugTf("=> bufOut[%s]\r\n", bufout);
  strCopy(bufout, sizeof(bufout), bufin, 0, 8);
  DebugTf("=> bufOut[%s]\r\n", bufout);
  strCopy(bufout, sizeof(bufout), bufin, 15, 25);
  DebugTf("=> bufOut[%s]\r\n", bufout);
  strCopy(bufout, sizeof(bufout), bufin, 0, 20);
  DebugTf("=> bufOut[%s]\r\n", bufout);

  bufin[8] = '\0';
  strCopy(bufout, sizeof(bufout), bufin, 5, 10);
  DebugTf("=> bufOut[%s]\r\n", bufout);
  strCopy(bufout, sizeof(bufout), bufin, 0, 8);
  DebugTf("=> bufOut[%s]\r\n", bufout);
  strCopy(bufout, sizeof(bufout), bufin, 15, 25);
  DebugTf("=> bufOut[%s]\r\n", bufout);
  strCopy(bufout, sizeof(bufout), bufin, 0, 20);
  DebugTf("=> bufOut[%s]\r\n", bufout);
  DebugTln("strCopy() test ======");
**/
} // setup()


void loop () 
{
  DebugTf("===>actTimestamp[%s]-> nrReboots[%u]-> Errors[%u]<======\r\n\n", actTimestamp
                                                                    , nrReboots
                                                                    , slotErrors);
  DebugTln(buildDateTimeString(actTimestamp, strlen(actTimestamp)) );
  hr = nextHour(actTimestamp, strlen(actTimestamp));
  time_t actT = epoch(actTimestamp, strlen(actTimestamp), false);
  time_t newT = epoch(newTimestamp, strlen(newTimestamp), false);
  if (hour(actT) != hour(newT) )
  {
    uint16_t recSlot = timestampToHourSlot(newTimestamp, strlen(newTimestamp));
    if (recSlot > _NO_HOUR_SLOTS_) slotErrors++;
    updateStatus = true;
  }
  if (day(actT) != day(newT) )
  {
    uint16_t recSlot = timestampToDaySlot(newTimestamp, strlen(newTimestamp));
    readDataFromFile(DAYS, DAYS_FILE, actTimestamp, false, "") ;
    readDataFromFile(HOURS, HOURS_FILE, actTimestamp, false, "") ;
    if (recSlot > _NO_DAY_SLOTS_) slotErrors++;
    updateStatus = true;
  }
  if (month(actT) != month(newT) )
  {
    readDataFromFile(MONTHS, MONTHS_FILE, actTimestamp, false, "") ;
    uint16_t recSlot = timestampToMonthSlot(newTimestamp, strlen(newTimestamp));
    if (recSlot > _NO_MONTH_SLOTS_) slotErrors++;
    updateStatus = true;
  }
  //strncpy(actTimestamp, newTimestamp, 16);
  sprintf(actTimestamp, "%sX", newTimestamp);
  if (updateStatus)
  {
    writeDataToFiles();
    writeLastStatus();
    updateStatus = false;
  }

  setTime(actT);  // <<< after all processing and file writing
  
  delay(500);
  
#if defined(READSLIMMEMETER)
  slimmeMeter.loop();
  slimmeMeter.enable(true);
  if (millis() - readTimer > 10000)
  {
    readTimer = millis();
    if (slimmeMeter.available()) 
    {
      DSMRdata = {};
      String DSMRerror;
      
      if (slimmeMeter.parse(&DSMRdata, &DSMRerror))   // Parse succesful, print result
      {
        //processData();
        makeJson();
      }
    }
  }
#endif
} // loop()
