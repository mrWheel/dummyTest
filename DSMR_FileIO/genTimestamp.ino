int nextHour(const char *tmpTimestamp, int8_t len)
{
  //DebugTf("nextHour(%s)\r\n", tmpTimestamp);
  //DebugFlush();
  time_t t1 = epoch((char*)tmpTimestamp, len, false);
  //DebugTf("Na epoch(String(tmpTimestamp)) t1[%u] hour[%d]..", t1, hour(t1)); Serial.flush();
  t1 += SECS_PER_HOUR;
  epochToTimestamp(t1, newTimestamp, strlen(newTimestamp));
  //DebugTf("newTimestamp[%s] newHour[%d]\r\n", newTimestamp, hour(t1)); Serial.flush();
  //DebugTf(" ---> [%d]\r\n", hour(t1));
  return hour(t1);

} // nextHour()  
