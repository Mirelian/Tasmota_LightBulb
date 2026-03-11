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
            Tele[i].change = false;
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

    if (httpCode > 0)
    {
        AddLog(LOG_LEVEL_INFO, PSTR("TB : HTTP %d - Sent Telementary %s"), httpCode, payload.c_str());
    }
    else
    {
        AddLog(LOG_LEVEL_ERROR, PSTR("TB : HTTP failed (%d)"), httpCode);
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

    if (strcmp(method, "setCT") == 0)
    { // params: 153-500
        LightSetColorTemp(root[PSTR("params")].getUInt());
        LightPreparePower(2);
    }
    else if (strcmp(method, "setHue") == 0)
    { // params: 0-359
        uint16_t hue = root[PSTR("params")].getUInt();
        char cmd[30];
        snprintf(cmd, sizeof(cmd), "HsbColor1 %u", hue);
        ExecuteCommand(cmd, SRC_WEBGUI); // updates light & telemetry
    }
    else if (strcmp(method, "setSaturation") == 0)
    { // params: 0-100
        uint8_t sat = root[PSTR("params")].getUInt();
        char cmd[30];
        snprintf(cmd, sizeof(cmd), "HsbColor2 %u", sat);
        ExecuteCommand(cmd, SRC_WEBGUI);
    }
    else if (strcmp(method, "setDimmer") == 0)
    { // params: 0-100
        uint8_t dimm = root[PSTR("params")].getUInt();
        LightSetDimmer(dimm);
        LightPreparePower(2);
    }
    else if (strcmp(method, "setPower") == 0)
    { // params: true/false or 1/0
        bool on = root[PSTR("params")].getBool();
        char cmd[30];
        snprintf(cmd, sizeof(cmd), "Power %u", on);
        ExecuteCommand(cmd, SRC_WEBGUI);
    }
    else if (strcmp(method, "setFanSpeed") == 0)
    { // params: 0-100
        char speed = root[PSTR("params")].getStr()[0];

        switch (speed)
        {
        case 'L':
            LightSetDimmer(60);
            break;
        case 'M':
            LightSetDimmer(80);
            break;
        case 'H':
            LightSetDimmer(100);
            break;
        default:
            LightSetDimmer(20);
            break;
        }
        LightPreparePower(2);
    }
}

/*********************************************************************************************
 * ThingsBoard RPC Transport (Non-Blocking WiFiClient)
 * Runs from FUNC_EVERY_50_MSECOND
 *********************************************************************************************/

struct ThingsBoard_RPC
{
    WiFiClient client;
    char data[1024];
    uint16_t data_len;
    int content_length;
    bool header_done;
    bool request_active;
};

static ThingsBoard_RPC tb_rpc;

void RPCThingsBoardReset()
{
    tb_rpc.client.stop();
    tb_rpc.data[0] = '\0';
    tb_rpc.data_len = 0;
    tb_rpc.content_length = -1;
    tb_rpc.header_done = false;
    tb_rpc.request_active = false;
}

int RPCThingsBoardContentLength(const char *header)
{
    const char *cl = strstr(header, "Content-Length:");
    if (!cl)
        return -1;

    cl += strlen("Content-Length:");
    while (*cl == ' ' || *cl == '\t')
    {
        cl++;
    }

    return atoi(cl);
}

void RPCThingsBoardFinalize()
{
    if (!tb_rpc.header_done || tb_rpc.data_len == 0)
    {
        RPCThingsBoardReset();
        return;
    }
    if (tb_rpc.content_length >= 0 && tb_rpc.data_len > (uint16_t)tb_rpc.content_length)
    {
        tb_rpc.data_len = tb_rpc.content_length;
        tb_rpc.data[tb_rpc.data_len] = '\0';
    }

    char *json_start = strchr(tb_rpc.data, '{');
    if (json_start)
    {
        RPCThingsBoardProcess(json_start);
    }

    RPCThingsBoardReset();
}

bool RPCThingsBoardStart()
{
    RPCThingsBoardReset();

    if (!tb_rpc.client.connect(tb_host, 80))
    {
        AddLog(LOG_LEVEL_DEBUG, PSTR("TB: RPC connect failed"));
        return false;
    }

    char request[256];
    snprintf_P(request, sizeof(request),
               PSTR("GET /api/v1/%s/rpc?timeout=5000&limit=1 HTTP/1.0\r\n"
                    "Host: %s\r\n"
                    "Connection: close\r\n"
                    "\r\n"),
               tb_token, tb_host);

    tb_rpc.client.print(request);
    tb_rpc.request_active = true;

    return true;
}

void RPCThingsBoardFetch()
{
    if (!WiFi.isConnected())
        return;

    if (!tb_host[0] || !tb_token[0])
    {
        return;
    }

    if (!tb_rpc.request_active)
    {
        RPCThingsBoardStart();
        return;
    }

    while (tb_rpc.client.available() > 0)
    {
        int data_space_left = sizeof(tb_rpc.data) - 1 - tb_rpc.data_len;
        if (data_space_left <= 0)
        {
            AddLog(LOG_LEVEL_ERROR, PSTR("TB: RPC buffer overflow"));
            RPCThingsBoardReset();
            return;
        }

        int chunk = tb_rpc.client.available();
        if (chunk > data_space_left)
            chunk = data_space_left;

        int read = tb_rpc.client.read((uint8_t *)tb_rpc.data + tb_rpc.data_len, chunk);
        if (read <= 0)
        {
            break;
        }

        tb_rpc.data_len += read;
        tb_rpc.data[tb_rpc.data_len] = '\0';

        if (!tb_rpc.header_done)
        {
            char *header_end = strstr(tb_rpc.data, "\r\n\r\n");
            if (header_end)
            {
                *header_end = '\0';
                tb_rpc.content_length = RPCThingsBoardContentLength(tb_rpc.data);
                *header_end = '\r';

                uint16_t header_size = (header_end - tb_rpc.data) + 4;
                uint16_t body_len = tb_rpc.data_len - header_size;

                // Remove header from data buffer
                memmove(tb_rpc.data, tb_rpc.data + header_size, body_len);
                tb_rpc.data_len = body_len;
                tb_rpc.data[tb_rpc.data_len] = '\0';

                tb_rpc.header_done = true;
            }
        }

        if (tb_rpc.header_done && tb_rpc.content_length >= 0)
        {
            if (tb_rpc.data_len >= (uint16_t)tb_rpc.content_length)
            {
                RPCThingsBoardFinalize();
                return;
            }
        }
    }

    // Server closed connection: fallback
    if (tb_rpc.request_active && !tb_rpc.client.connected() && tb_rpc.client.available() == 0)
    {
        RPCThingsBoardFinalize();
    }
}

#endif // USE_THINGSBOARD