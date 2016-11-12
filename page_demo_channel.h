#ifndef PAGE_DEMO_CHANNEL_H_
#define PAGE_DEMO_CHANNEL_H_

void send_demo_channel_html(AsyncWebServerRequest *request) {
    if (request->params()) {
        AsyncWebServerResponse *response = request->beginResponse(303);
        response->addHeader("Location", request->url());
        request->send(response);
    } else {
        request->send(400);
    }
}

void send_demo_receive_vals(AsyncWebServerRequest *request){
    if (request->params()) {
        if (request->hasParam("stopDemo", true)) {
          demo = 0;
        } else {
          for (uint8_t i = 0; i < request->params(); i++) {
            AsyncWebParameter *p = request->getParam(i);
            if (p->name() == "rangeValue1") { /* ALL */ demoChannelValue = p->value().toInt();     demo = 1; }
            if (p->name() == "rangeValue2") { /* ON-OFF */ demoChannelValue = p->value().toInt();  demo = 2; }
            if (p->name() == "rangeValue3") { /* HOPPING */ demoChannelValue = p->value().toInt(); demo = 3; }
            if (p->name() == "rangeValue4") { /* FLIPPING */ demoChannelValue = p->value().toInt(); demo = 4; }
            if (p->name() == "rangeValue5") { /* RANDOM */ demoChannelValue = p->value().toInt();   demo = 5; }
            if (p->name() == "on")  { /* ON  */ channels.setValue(p->value().toInt() - 1, 255);   demo = 6; }
            if (p->name() == "off") { /* OFF */ channels.setValue(p->value().toInt() - 1, 0  );   demo = 6; }
          }
        }
    }
    AsyncWebServerResponse *response = request->beginResponse(303);
    response->addHeader("Location", request->url());
    request->send(response);
}


void send_demo_button_vals(AsyncWebServerRequest *request) {

    String buttons = "<table border='0' cellspacing='1' cellpadding='3' style='width:310px'><tbody>";
    for (int i = 1; i <= config.channel_count; i++) {
        buttons += "<tr><td>#" + String(i) + ":[" + String(channels.getValue(i-1)) + "]</td><td><button onclick=\"onoff('on'," + String(i) + ")\" class='";
        if(channels.getValue(i-1)>0)  buttons += " btn_on";
          else                        buttons += " btn_off";
        buttons += "'>ON</button></td><td><button onclick=\"onoff('off'," + String(i) + ")\" class='";
        if(channels.getValue(i-1)==0) buttons += " btn_on";
          else                        buttons += " btn_off"; 
        buttons += "'>OFF</button></td></tr>";
    }
    buttons += "</tbody></table>";
    buttons = "buttons|div|" + buttons + "\n";

    // Convert String to character array
    char charArr[buttons.length()+1];
    buttons.toCharArray(charArr,buttons.length()+1); 

    // send response with chunks
    request->send_P(200, "text/plain", charArr);
}

#endif /* PAGE_DEMO_CHANNEL_H_ */
