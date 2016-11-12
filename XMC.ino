/*
* XMC.ino
*
* Project: XMC - An ESP8266 and E1.31 based LED strips driver
*  Copyright (c) 2016 Christoph Rafetzeder 
*  Special thanks to Dieter Reinhardt and Markus Felinger 
*  for the inspiration and their support.
* 
*  Some parts of this project are forked from:
*   1.) http://www.esp8266.com/viewtopic.php?f=152&t=8814 from RichardS 
*   2.) https://github.com/forkineye/ESPixelStick from http://www.forkineye.com
*
*  This program is provided free for you to use in any way that you wish,
*  subject to the laws and regulations where you are using it. Due diligence
*  is strongly suggested before using this code.  Please give credit where due.
*
*  The Author makes no warranty of any kind, express or implied, with regard
*  to this program or the documentation contained in this document.  The
*  Author shall not be liable in any event for incidental or consequential
*  damages in connection with, or arising out of, the furnishing, performance
*  or use of these programs.
*
*/

/*****************************************/
/*        BEGIN - Configuration          */
/*****************************************/

/* Fallback configuration if config.json is empty or fails */
//const char ssid[] = "ENTER_SSID_HERE";
//const char passphrase[] = "ENTER_PASSPHRASE_HERE";

const char ssid[] = "Pirello";
const char passphrase[] = "AAAABBBBCCCCD";

//const char ssid[]       = "DMX";             /* Replace with your SSID */
//const char passphrase[] = "Christmas";       /* Replace with your WPA2 passphrase */

/*****************************************/
/*         END - Configuration           */
/*****************************************/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <Hash.h>
#include "XMC.h"
#include "_E131.h"
#include "helpers.h"

/* Output Drivers */
#include "ChannelDriver.h"
ChannelDriver       channels;           /* Channel object */

/* Public declarations */
uint8_t             *seqTracker;        /* Current sequence numbers for each Universe */
uint32_t            lastUpdate;         /* Update timeout tracker */
AsyncWebServer      web(HTTP_PORT);     /* Web Server */

/* Web pages */
#include "page_config_channel.h"
#include "page_config_channel_mapping.h"
#include "page_demo_channel.h"

/* Common Web pages and handlers */
#include "page_root.h"
#include "page_admin.h"
#include "page_config_net.h"
#include "page_status_net.h"
#include "page_status_e131.h"

void setup() {
    /* Generate and set hostname */
    char chipId[7] = { 0 };
    snprintf(chipId, sizeof(chipId), "%06x", ESP.getChipId());
    String hostname = "XMC_" + String(chipId);
    WiFi.hostname(hostname);

    /* Initial pin states */
    // disable built-in led if in client mode
    pinMode(BUILT_IN_LED, OUTPUT); digitalWrite(BUILT_IN_LED, HIGH); 

    /* Setup serial log port */
    LOG_PORT.begin(115200);
    delay(10);

    /* Enable SPIFFS */
    SPIFFS.begin();

    LOG_PORT.println("");
    LOG_PORT.print(F("XMC v"));
    for (uint8_t i = 0; i < strlen_P(VERSION); i++)
        LOG_PORT.print((char)(pgm_read_byte(VERSION + i)));
    LOG_PORT.println("");

    /* Load configuration from SPIFFS */
    loadConfig();

    /* Fallback to default SSID and passphrase if we fail to connect */
    int status = initWifi();
    if (status != WL_CONNECTED) {
        LOG_PORT.println(F("*** Timeout - Reverting to default SSID ***"));
        config.ssid = ssid;
        config.passphrase = passphrase;
        status = initWifi();
    }

    /* If we fail again, go SoftAP or reboot */
    if (status != WL_CONNECTED) {
        if (config.ap_fallback) {
            LOG_PORT.println(F("**** FAILED TO ASSOCIATE WITH AP, GOING SOFTAP ****"));
            WiFi.mode(WIFI_AP);
            String ssid = "XMC_" + String(chipId);
            WiFi.softAP(ssid.c_str());
            Serial.println(WiFi.softAPIP()); 
        } else {
            LOG_PORT.println(F("**** FAILED TO ASSOCIATE WITH AP, REBOOTING ****"));
            ESP.restart();
        }
    }

    /* Configure and start the web server */
    initWeb();

    /* Setup mDNS / DNS-SD */
    //TODO: Reboot or restart mdns when config.id is changed?
    MDNS.setInstanceName(config.id + " (" + String(chipId) + ")");
    if (MDNS.begin(hostname.c_str())) {
        MDNS.addService("http", "tcp", HTTP_PORT);
        MDNS.addService("e131", "udp", E131_DEFAULT_PORT);
        MDNS.addServiceTxt("e131", "udp", "TxtVers", String(RDMNET_DNSSD_TXTVERS));
        MDNS.addServiceTxt("e131", "udp", "ConfScope", RDMNET_DEFAULT_SCOPE);
        MDNS.addServiceTxt("e131", "udp", "E133Vers", String(RDMNET_DNSSD_E133VERS));
        MDNS.addServiceTxt("e131", "udp", "CID", String(chipId));
        MDNS.addServiceTxt("e131", "udp", "Model", "XMC");
        MDNS.addServiceTxt("e131", "udp", "Manuf", "Forkineye");
    } else {
        LOG_PORT.println(F("*** Error setting up mDNS responder ***"));
    }

    /* Update Config and PCA9685 */
    updateConfig();
}

int initWifi() {
    /* Switch to station mode and disconnect just in case */
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    LOG_PORT.println("");
    LOG_PORT.print(F("Connecting to "));
    LOG_PORT.println(config.ssid);

    WiFi.begin(config.ssid.c_str(), config.passphrase.c_str());
    if (config.dhcp) {
        LOG_PORT.print(F("Connecting with DHCP"));
    } else {
        /* We don't use DNS, so just set it to our gateway */
        WiFi.config(IPAddress(config.ip[0], config.ip[1], config.ip[2], config.ip[3]),
                    IPAddress(config.gateway[0], config.gateway[1], config.gateway[2], config.gateway[3]),
                    IPAddress(config.netmask[0], config.netmask[1], config.netmask[2], config.netmask[3]),
                    IPAddress(config.gateway[0], config.gateway[1], config.gateway[2], config.gateway[3])
        );
        LOG_PORT.print(F("Connecting with Static IP"));
    }

    uint32_t timeout = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        LOG_PORT.print(".");
        if (millis() - timeout > CONNECT_TIMEOUT) {
            LOG_PORT.println("");
            LOG_PORT.println(F("*** Failed to connect ***"));
            break;
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        LOG_PORT.println("");
        LOG_PORT.print(F("Connected with IP: "));
        LOG_PORT.println(WiFi.localIP());

        if (config.multicast)
            e131.begin(E131_MULTICAST, config.universe);
        else
            e131.begin(E131_UNICAST);
    }

    return WiFi.status();
}

/* Configure and start the web server */
void initWeb() {
    /* Handle OTA update from asynchronous callbacks */
    Update.runAsync(true);

    /* Heap status handler */
    web.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });

    /* Config file handler for testing */
    web.serveStatic("/configfile", SPIFFS, "/config.json");

    /* JSON Config Handler */
    web.on("/conf", HTTP_GET, [](AsyncWebServerRequest *request) {
        String jsonString;
        serializeConfig(jsonString);
        request->send(200, "text/json", jsonString);
    });

    /* AJAX Handlers */
    web.on("/rootvals", HTTP_GET, send_root_vals);
    web.on("/adminvals", HTTP_GET, send_admin_vals);
    web.on("/config/netvals", HTTP_GET, send_config_net_vals);
    web.on("/config/survey", HTTP_GET, send_survey_vals);
    web.on("/status/netvals", HTTP_GET, send_status_net_vals);
    web.on("/status/e131vals", HTTP_GET, send_status_e131_vals);
    web.on("/config/channelvals", HTTP_GET, send_config_channel_vals);
    web.on("/config/channelmappingvals", HTTP_GET, send_config_channel_mapping_vals);
    web.on("/demo/getbuttons", HTTP_GET, send_demo_button_vals);
        
    /* POST Handlers */
    web.on("/admin.html", HTTP_POST, send_admin_html, handle_fw_upload);
    web.on("/config_net.html", HTTP_POST, send_config_net_html);
    web.on("/config/chmapdef", HTTP_POST, receive_config_channel_mapping_default);
    web.on("/demo/receivevals", HTTP_POST, send_demo_receive_vals);

    /* Static handler */
    web.on("/config_channel.html", HTTP_POST, send_config_channel_html);
    web.on("/config_ch_map.html", HTTP_POST, send_config_channel_mapping_html);
    web.on("/demo_channel.html", HTTP_POST, send_demo_channel_html);
    web.serveStatic("/", SPIFFS, "/www/").setDefaultFile("channel.html");

    web.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Page '" + String(request->url()) + "' not found :( ");
    });

    web.begin();

    LOG_PORT.print(F("- Web Server started on port "));
    LOG_PORT.println(HTTP_PORT);
}

/* Configuration Validations */
void validateConfig() {
    /* E1.31 Limits */
    if (config.universe < 1)
        config.universe = 1;

    if (config.channel_start < 1)
        config.channel_start = 1;
    else if (config.channel_start > UNIVERSE_LIMIT)
        config.channel_start = UNIVERSE_LIMIT;

    /* Generic channel limits for channels */
    if (config.channel_count > CHANNEL_LIMIT)
        config.channel_count = CHANNEL_LIMIT;
    else if (config.channel_count < 1)
        config.channel_count = 1;

}

void updateConfig() {
    /* Validate first */
    validateConfig();

    /* Find the last universe we should listen for */
    uint16_t span = config.channel_start + config.channel_count - 1;
    if (span % UNIVERSE_LIMIT)
        uniLast = config.universe + span / UNIVERSE_LIMIT;
    else
        uniLast = config.universe + span / UNIVERSE_LIMIT - 1;

    /* Setup the sequence error tracker */
    uint8_t uniTotal = (uniLast + 1) - config.universe;

    if (seqTracker) free(seqTracker);
    if (seqTracker = static_cast<uint8_t *>(malloc(uniTotal)))
        memset(seqTracker, 0x00, uniTotal);

    if (seqError) free(seqError);
    if (seqError = static_cast<uint32_t *>(malloc(uniTotal * 4)))
        memset(seqError, 0x00, uniTotal * 4);
  
    /* Zero out packet stats */
    e131.stats.num_packets = 0;

    /* Initialize pca9685 */
    channels.begin(config.channel_count); 
    channels.setGamma(config.channel_gamma);
    channels.setupPCA9685(config.zero, config.channel_start + config.channel_count);

    LOG_PORT.print(F("- Listening for "));
    LOG_PORT.print(config.channel_count);
    LOG_PORT.print(F(" channels, from Universe "));
    LOG_PORT.print(config.universe);
    LOG_PORT.print(F(" to "));
    LOG_PORT.println(uniLast);
}

/* Load configugration JSON file */
void loadConfig() {
    /* Zeroize Config struct */
    memset(&config, 0, sizeof(config));

    /* Load CONFIG_FILE json. Create and init with defaults if not found */
    File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) {
        LOG_PORT.println(F("- No configuration file found."));
        config.ssid = ssid;
        config.passphrase = passphrase;
        saveConfig();
    } else if (reset) {
        LOG_PORT.println(F("- Reset configuration."));
        config.ssid = ssid;
        config.passphrase = passphrase;
        saveConfig();
    } else {
        /* Parse CONFIG_FILE json */
        size_t size = file.size();
        if (size > CONFIG_MAX_SIZE) {
            LOG_PORT.println(F("*** Configuration File too large ***"));
            return;
        }

        std::unique_ptr<char[]> buf(new char[size]);
        file.readBytes(buf.get(), size);

        DynamicJsonBuffer jsonBuffer;
        JsonObject &json = jsonBuffer.parseObject(buf.get());
        if (!json.success()) {
            LOG_PORT.println(F("*** Configuration File Format Error ***"));
            return;
        }

        /* Device */
        config.id = json["device"]["id"].as<String>();

        /* Fallback to embedded ssid and passphrase if null in config */
        if (strlen(json["network"]["ssid"]))
            config.ssid = json["network"]["ssid"].as<String>();
        else
            config.ssid = ssid;

        if (strlen(json["network"]["passphrase"]))
            config.passphrase = json["network"]["passphrase"].as<String>();
        else
            config.passphrase = passphrase;

        /* Network */
        for (int i = 0; i < 4; i++) {
            config.ip[i] = json["network"]["ip"][i];
            config.netmask[i] = json["network"]["netmask"][i];
            config.gateway[i] = json["network"]["gateway"][i];
        }
        config.dhcp = json["network"]["dhcp"];
        config.ap_fallback = json["network"]["ap_fallback"];

        /* E131 */
        config.universe = json["e131"]["universe"];
        config.channel_start = json["e131"]["channel_start"];
        config.channel_count = json["e131"]["channel_count"];
        config.multicast = json["e131"]["multicast"];

        /* Channel */
        config.channel_gamma = json["channel"]["gamma"];
        config.toggle = json["channel"]["toggle"];
        config.zero = json["channel"]["zero"];
        config.mapping = json["channel"]["mapping"].as<String>();

        if (mapping) free(mapping);
        mapping = static_cast<uint16_t *>(malloc(config.channel_count*2));
        
        String value = String(config.mapping);
        uint16_t idx = 0;
        for ( uint16_t i = 0; i < config.channel_count; i++){
          idx = value.indexOf('|');
          if(idx > 0) {
            mapping[i] = value.substring(0,idx).toInt();
            value = value.substring(idx+1);
          }else if(value.length()){
            mapping[i] = value.toInt();
            value = "";
          }else{
            mapping[i] = i + 1;  
          }
        }
        
        LOG_PORT.println(F("- Configuration loaded."));
    }

    /* Validate it */
    validateConfig();
}

/* Serialize the current config into a JSON string */
void serializeConfig(String &jsonString, bool pretty, bool creds) {
    /* Create buffer and root object */
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();

    /* Device */
    JsonObject &device = json.createNestedObject("device");
    device["id"] = config.id.c_str();

    /* Network */
    JsonObject &network = json.createNestedObject("network");
    network["ssid"] = config.ssid.c_str();
    if (creds)
        network["passphrase"] = config.passphrase.c_str();
    JsonArray &ip = network.createNestedArray("ip");
    JsonArray &netmask = network.createNestedArray("netmask");
    JsonArray &gateway = network.createNestedArray("gateway");
    for (int i = 0; i < 4; i++) {
        ip.add(config.ip[i]);
        netmask.add(config.netmask[i]);
        gateway.add(config.gateway[i]);
    }
    network["dhcp"] = config.dhcp;
    network["ap_fallback"] = config.ap_fallback;

    /* E131 */
    JsonObject &e131 = json.createNestedObject("e131");
    e131["universe"] = config.universe;
    e131["channel_start"] = config.channel_start;
    e131["channel_count"] = config.channel_count;
    e131["multicast"] = config.multicast;

    /* Channel */
    JsonObject &channel = json.createNestedObject("channel");
    channel["gamma"] = config.channel_gamma;
    channel["toggle"] = config.toggle;
    channel["zero"] = config.zero;

    /* Mapping */
    String mappingStr = "";
    for (uint16_t i = 0; i < config.channel_count-1 ; i++)
       mappingStr += String(mapping[i]) + "|";
    mappingStr += String(mapping[config.channel_count-1]);
    channel["mapping"] = mappingStr.c_str();

    if (pretty)
        json.prettyPrintTo(jsonString);
    else
        json.printTo(jsonString);
}

/* Save configuration JSON file */
void saveConfig() {
    /* Update Config */
    updateConfig();

    /* Serialize Config */
    String jsonString;
    serializeConfig(jsonString, true, true);

    /* Save Config */
    File file = SPIFFS.open(CONFIG_FILE, "w");
    if (!file) {
        LOG_PORT.println(F("*** Error creating configuration file ***"));
        return;
    } else {
        file.println(jsonString);
        LOG_PORT.println(F("* Configuration saved."));
    }
}

/* Main Loop */
void loop() {

    /* Reboot handler */
    if (reset) {
        /* Initialize config structure, if reset=true */
        loadConfig();
        reboot = true;
    }
  
    /* Reboot handler */
    if (reboot) {
        delay(REBOOT_DELAY);
        ESP.restart();
    }

    /* DEMO handler */
    if (demo) {
        switch (demo) {
            case 1: /* ALL */
                for (int i = 0; i < config.channel_count; i++) {
                    channels.setValue(i, demoChannelValue);
                }
                channels.show();
                demo = 0;
                lWaitMillis = 0;
                break;
                
            case 2: /* ON-OFF */
                if( (long)( millis() - lWaitMillis ) >= 0){
                  lWaitMillis = millis() + demoChannelValue;                  
                  for (int i = 0; i < config.channel_count; i++) {
                      if(demoCounter)
                        channels.setValue(i, 255);
                      else
                        channels.setValue(i, 0);
                  }
                  channels.show();
                  if(demoCounter)
                    demoCounter = 0;
                  else
                    demoCounter = 1;
                }

                break;
                
            case 3: /* HOPPING */
                if( (long)( millis() - lWaitMillis ) >= 0){
                  lWaitMillis = millis() + demoChannelValue;                  
                  for (int i = 0; i < config.channel_count; i++) {
                      if(i == demoCounter)
                        channels.setValue(i, 255);
                      else
                        channels.setValue(i, 0);
                  }
                  channels.show();
                  demoCounter = ++demoCounter % config.channel_count;
                }
                break;
                
            case 4: /* FLIPPING */
                if( (long)( millis() - lWaitMillis ) >= 0){
                  lWaitMillis = millis() + demoChannelValue;                  
                  for (int i = 0; i < config.channel_count; i++) {
                      if(i%2 == demoCounter)
                        channels.setValue(i, 255);
                      else
                        channels.setValue(i, 0);
                  }
                  channels.show();
                  demoCounter = ++demoCounter % 2;
                }
                break;
                
            case 5: /* RANDOM */
                if( (long)( millis() - lWaitMillis ) >= 0){
                  lWaitMillis = millis() + demoChannelValue;                  
                  for (int i = 0; i < config.channel_count; i++) {
                    channels.setValue(i, (uint8_t) random(0, 256));
                  }
                  channels.show();
                }
                break;
            case 6: /* single set */
                channels.show();
                demo = 0;
                lWaitMillis = 0;
                break;

            case 0:
                Serial.println(F("No DEMO should not bring you to this line!"));
        }

        yield();
        return;
    }

      
    /* Parse a packet and update channels */
    if (e131.parsePacket()) {
        if ((e131.universe >= config.universe) && (e131.universe <= uniLast)) {
            /* Toggle Built-In LED */
            toggleBuiltInLED();
            
            /* Universe offset and sequence tracking */
            uint8_t uniOffset = (e131.universe - config.universe);
            if (e131.packet->sequence_number != seqTracker[uniOffset]++) {
                seqError[uniOffset]++;
                seqTracker[uniOffset] = e131.packet->sequence_number + 1;
            }

            /* Offset the channel if required for the first universe */
            uint16_t offset = 0;
            if (e131.universe == config.universe)
                offset = config.channel_start - 1;

            /* Find start of data based off the Universe */
            uint16_t dataStart = uniOffset * UNIVERSE_LIMIT;

            /* Calcuate how much data we need for this buffer */
            uint16_t dataStop = config.channel_count;
            if ((dataStart + UNIVERSE_LIMIT) < dataStop)
                dataStop = dataStart + UNIVERSE_LIMIT;

            /* Set the data */
            uint16_t buffloc = 0;
            for (int i = dataStart; i < dataStop; i++) {
                if(mapping[i]-1 < config.channel_count) channels.setValue(mapping[i]-1, e131.data[buffloc + offset]);
                
                buffloc++;
            }

            channels.show();

        }
    }

// perform background stuff
    yield();
    
}









