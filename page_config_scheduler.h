#ifndef PAGE_CONFIG_SCHEDULER_H_
#define PAGE_CONFIG_SCHEDULER_H_

void send_config_schedule_html(AsyncWebServerRequest *request) {
    locked = true;
    if (request->params()) {
        config.schedule = false;
        for (uint8_t i = 0; i < request->params(); i++) {
            AsyncWebParameter *p = request->getParam(i);
            if (p->name() == "hh01a") config.scheduler_vals[0][0][0] = p->value().toInt();
            if (p->name() == "mm01a") config.scheduler_vals[0][0][1] = p->value().toInt();
            if (p->name() == "hh01b") config.scheduler_vals[0][1][0] = p->value().toInt();
            if (p->name() == "mm01b") config.scheduler_vals[0][1][1] = p->value().toInt();
            if (p->name() == "hh02a") config.scheduler_vals[1][0][0] = p->value().toInt();
            if (p->name() == "mm02a") config.scheduler_vals[1][0][1] = p->value().toInt();
            if (p->name() == "hh02b") config.scheduler_vals[1][1][0] = p->value().toInt();
            if (p->name() == "mm02b") config.scheduler_vals[1][1][1] = p->value().toInt();
            if (p->name() == "hh03a") config.scheduler_vals[2][0][0] = p->value().toInt();
            if (p->name() == "mm03a") config.scheduler_vals[2][0][1] = p->value().toInt();
            if (p->name() == "hh03b") config.scheduler_vals[2][1][0] = p->value().toInt();
            if (p->name() == "mm03b") config.scheduler_vals[2][1][1] = p->value().toInt();
            if (p->name() == "hh04a") config.scheduler_vals[3][0][0] = p->value().toInt();
            if (p->name() == "mm04a") config.scheduler_vals[3][0][1] = p->value().toInt();
            if (p->name() == "hh04b") config.scheduler_vals[3][1][0] = p->value().toInt();
            if (p->name() == "mm04b") config.scheduler_vals[3][1][1] = p->value().toInt();
            if (p->name() == "timeout") config.dmxTimeout = p->value().toInt();
            if (p->name() == "schchk") config.schedule = true;
        }

        saveConfig();

        AsyncWebServerResponse *response = request->beginResponse(303);
        response->addHeader("Location", request->url());
        request->send(response);
    } else {
        request->send(400);
    }
    locked = false;
}

void send_scheduler_receive_vals(AsyncWebServerRequest *request){
    locked = true;  
    if (request->params()) {
        if (request->hasParam("syncNTP", true)) {
          ntp_update = true;
        } else {
          uint16_t second=0, minute=0, hour=0;
          for (uint8_t i = 0; i < request->params(); i++) {
            AsyncWebParameter *p = request->getParam(i);
            if (p->name() == "hh_clock") { hour   = p->value().toInt(); }            
            if (p->name() == "mm_clock") { minute = p->value().toInt(); }
            if (p->name() == "ss_clock") { second = p->value().toInt(); }
          }

          Serial.println("h:" + String(hour) + " min:" + String(minute) + " sec:" + String(second) );
          updateClockMan(hour, minute, second);
        }
    }
    AsyncWebServerResponse *response = request->beginResponse(303);
    response->addHeader("Location", request->url());
    request->send(response);
    locked = false;    
}

void send_config_scheduler_curTime_vals(AsyncWebServerRequest *request) {
    locked = true;  
    time_t now = time(nullptr);

    String values = "";
    values += "NTPTime|div|" + getNTPTimestampString() + "\n";
    values += "hh_clock|input|" + getHH_ntpString() + "\n";
    values += "mm_clock|input|" + getMM_ntpString() + "\n";
    values += "ss_clock|input|" + getSS_ntpString() + "\n";
    request->send(200, "text/plain", values);
    locked = false;
}

void send_config_scheduler_vals(AsyncWebServerRequest *request) {
    locked = true;
    time_t now = time(nullptr);

    String values = "";
    values += "NTPTime|div|" + getNTPTimestampString() + "\n";
    values += "hh_clock|input|" + getHH_manString() + "\n";
    values += "mm_clock|input|" + getMM_manString() + "\n";
    values += "ss_clock|input|" + getSS_manString() + "\n";
    values += "hh01a|input|" + (String)config.scheduler_vals[0][0][0] + "\n";
    values += "mm01a|input|" + (String)config.scheduler_vals[0][0][1] + "\n";
    values += "hh01b|input|" + (String)config.scheduler_vals[0][1][0] + "\n";
    values += "mm01b|input|" + (String)config.scheduler_vals[0][1][1] + "\n";
    values += "hh02a|input|" + (String)config.scheduler_vals[1][0][0] + "\n";
    values += "mm02a|input|" + (String)config.scheduler_vals[1][0][1] + "\n";
    values += "hh02b|input|" + (String)config.scheduler_vals[1][1][0] + "\n";
    values += "mm02b|input|" + (String)config.scheduler_vals[1][1][1] + "\n";
    values += "hh03a|input|" + (String)config.scheduler_vals[2][0][0] + "\n";
    values += "mm03a|input|" + (String)config.scheduler_vals[2][0][1] + "\n";
    values += "hh03b|input|" + (String)config.scheduler_vals[2][1][0] + "\n";
    values += "mm03b|input|" + (String)config.scheduler_vals[2][1][1] + "\n";
    values += "hh04a|input|" + (String)config.scheduler_vals[3][0][0] + "\n";
    values += "mm04a|input|" + (String)config.scheduler_vals[3][0][1] + "\n";
    values += "hh04b|input|" + (String)config.scheduler_vals[3][1][0] + "\n";
    values += "mm04b|input|" + (String)config.scheduler_vals[3][1][1] + "\n";
    values += "timeout|input|" + (String)(config.dmxTimeout) + "\n";
    values += "schchk|chk|" + (String)(config.schedule ? "checked" : "") + "\n";
    values += "title|div|" + config.id + " - Scheduler Config\n";
    request->send(200, "text/plain", values);
    locked = false;
}

void send_scheduler_current_dmx_timout(AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "curDMXtimout|div|currently " + String((millis() - lastDMXPacket) / 1000) + " seconds<br>without DMX message\n");
}

#endif /* PAGE_CONFIG_SCHEDULER_H_ */
