#ifdef USE_THINGSBOARD

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