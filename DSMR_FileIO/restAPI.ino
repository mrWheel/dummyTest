

//===========================GLOBAL VAR'S======================================
//DynamicJsonDocument jsonDoc(4000);  // generic doc to return, clear() before use!
  JsonObject          jsonObj;
  char                apiName[20] = "";

struct buildJson {

    JsonArray root = jsonDoc.createNestedArray(apiName);
    
    template<typename Item>
    void apply(Item &i) {
      String Name = Item::name;

      JsonObject nested = root.createNestedObject();

      nested["name"]  = Name;
      if (i.present()) 
      {
        String Unit = Item::unit();
        nested["value"] = value_to_json(i.val());
        
        if (Unit.length() > 0)
        {
          nested["unit"]  = Unit;
        }
      }
      else
      {
        nested["value"] = "-";
      }
  }
  
template<typename Item>
Item& value_to_json(Item& i) {
  return i;
}

String value_to_json(TimestampedFixedValue i) {
  return String(i);
}
  
float value_to_json(FixedValue i) {
  return i;
}

}; // build_json{} 



//=======================================================================
void makeJson() 
{
  String toReturn;

  jsonDoc.clear();

  strcpy(apiName, "fields");
  DSMRdata.applyEach(buildJson());
  DebugTln();

  jsonObj = jsonDoc.as<JsonObject>();

  //serializeJson(jsonObj, toReturn);         // for production
  serializeJsonPretty(jsonObj, toReturn); // for human readable testing
  DebugTf("JSON String is %d chars\r\n", toReturn.length());
  DebugTln(toReturn);

} // makeJson()
