#ifdef USE_THINGSBOARD

#define XDRV_100 100

struct Telementary_data
{
    const char *key;
    char value[20];
    bool change;
};

Telementary_data *Tele = nullptr;
uint8_t TeleSize = 0;

bool RPCThingsBoardDeviceProcess(const char *method, JsonParserObject root);

static char tb_host[50] = "";
static char tb_token[50] = "";

void TelementaryThingsBoardSend()
{
    if (!WiFi.isConnected())
        return;

    if (!tb_host[0] || !tb_token[0])
    {
        return;
    }

    String payload = "{";

    for (uint8_t i = 0; i < TeleSize; i++)
    {
        if (Tele[i].change == true)
        {
            if (payload != "{")
                payload += ",";

            payload += "\"" + String(Tele[i].key) + "\":\"" + String(Tele[i].value) + "\"";
        }
    }

    if (payload == "{")
        return;

    payload += "}";

    WiFiClient client;
    HTTPClient http;

    char url[200];
    snprintf_P(url, sizeof(url),
               PSTR("http://%s/api/v1/%s/telemetry"),
               tb_host, tb_token);

    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");

    uint16_t httpCode = http.POST(payload);

    if (httpCode >= 200 && httpCode < 300)
    {
        for (uint8_t i = 0; i < TeleSize; i++)
        {
            Tele[i].change = false;
        }

        AddLog(LOG_LEVEL_INFO,
               PSTR("TB : HTTP %d - Sent Telementary %s"),
               httpCode, payload.c_str());
    }
    else
    {
        AddLog(LOG_LEVEL_ERROR,
               PSTR("TB : HTTP failed (%d)"),
               httpCode);
    }

    http.end();
}

void RPCThingsBoardProcess(char *payload)
{
    if (!payload || !payload[0])
        return;
    if (!strstr(payload, "\"method\""))
        return;

    JsonParser parser(payload);
    JsonParserObject root = parser.getRootObject();

    const char *method = root[PSTR("method")].getStr();

    if (!method || !method[0])
        return;

    AddLog(LOG_LEVEL_INFO, PSTR("TB : RPC %s"), method);

    RPCThingsBoardDeviceProcess(method, root);

    //    if (strcmp(method, "setFanSpeed") == 0)
    // { // params: 0-100
    //     char speed = root[PSTR("params")].getStr()[0];

    //     switch (speed)
    //     {
    //     case 'L':
    //         LightSetDimmer(60);
    //         break;
    //     case 'M':
    //         LightSetDimmer(80);
    //         break;
    //     case 'H':
    //         LightSetDimmer(100);
    //         break;
    //     default:
    //         LightSetDimmer(20);
    //         break;
    //     }
    //     LightPreparePower(2);
    // }
}

#endif // USE_THINGSBOARD