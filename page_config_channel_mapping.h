#ifndef PAGE_CONFIG_CHANNEL_MAPPING_H_
#define PAGE_CONFIG_CHANNEL_MAPPING_H_

void send_config_channel_mapping_html(AsyncWebServerRequest *request) {
    if (request->params()) {
        for (uint8_t i = 0; i < request->params(); i++) {
          AsyncWebParameter* p = request->getParam(i);

          if(String(p->name().c_str()).startsWith("map")){
            uint16_t idx = String(p->name().c_str()).substring(3).toInt();
            mapping[idx-1] = String(p->value().c_str()).toInt();      
          }
          
        }
        saveConfig();
        
        AsyncWebServerResponse *response = request->beginResponse(303);
        response->addHeader("Location", request->url());
        request->send(response);
    } else {
        request->send(400);
    }
}

void receive_config_channel_mapping_default(AsyncWebServerRequest *request){
    if (request->params()) {
        if (request->hasParam("defaultAssignment", true)) {
          for (uint16_t i = 0; i < config.channel_count ; i++)
            mapping[i] = i+1;
          saveConfig();
        } 
    }
    AsyncWebServerResponse *response = request->beginResponse(204);
    response->addHeader("Location", request->url());
    request->send(response);
}

void send_config_channel_mapping_vals(AsyncWebServerRequest *request) {
  // TODO: Switch to next universe if more than 512 channels are required
    String values = "";
    values += "title|div|" + config.id + " - Channel Mapping\n";
    values += "mapping|div|<table border='0' cellspacing='0' cellpadding='3'><tr bgcolor='#DDDDDD' ><td><strong>Uni</strong></td><td><strong>Ch</strong></td><td><strong>Output port #</strong></td></tr>";
    for (int i = 0; i < config.channel_count; i++) {
        values += "<tr><td>" + String(config.universe) + "</td><td>" + String(config.channel_start + i) + "</td><td>-> <input type='text' name='map" + String(i+1) + "' value='" + String(mapping[i]) + "'></td></tr>";
    }
    values += "</table>\n";

    // Convert String to character array
    char charArr[values.length()+1];
    values.toCharArray(charArr,values.length()+1); 
    request->send_P(200, "text/plain", charArr);
}


#endif /* PAGE_CONFIG_CHANNEL_MAPPING_H_ */
