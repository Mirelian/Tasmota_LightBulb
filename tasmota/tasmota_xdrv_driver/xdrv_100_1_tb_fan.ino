#ifdef USE_THINGSBOARD
#ifdef USE_TB_FAN

bool Xdrv100(uint32_t function)
{
    switch (function)
    {
    case FUNC_INIT:
    {
        Tele = new Telementary_data[4]{
            {"POWER", "", false},
            {"Speed", "", false},
            {"Temperature (°C)", "", false},
            {"Humidity (%)", "", false}};
        TeleSize = 4;

        snprintf_P(tb_host, sizeof(tb_host), PSTR("%s"), SettingsText(SET_MEM15));
        snprintf_P(tb_token, sizeof(tb_token), PSTR("%s"), SettingsText(SET_MEM16));
        if (!tb_host[0] || !tb_token[0])
        {
            AddLog(LOG_LEVEL_INFO, PSTR("TB : ThingsBoard HTTP Initialized"));
        }

        uint8_t mac[6];
        WiFi.macAddress(mac);

        char host_name[20];
        snprintf(host_name, sizeof(host_name), "Fan-%02X%02X%02X", mac[3], mac[4], mac[5]);

        if (!strstr(SettingsText(SET_HOSTNAME), "Fan"))
        {
            SettingsUpdateText(SET_HOSTNAME, host_name);
        }

        if (!Settings->flag3.mdns_enabled)
        {
            ExecuteCommand("SetOption55 1", SRC_IGNORE);
        }

        break;
    }

    case FUNC_EVERY_50_MSECOND:
    {
        RPCThingsBoardFetch();
        break;
    }

    case FUNC_SET_POWER:
    {
        if (!Tele)
        {
            break;
        }
        snprintf_P(Tele[0].value, sizeof(Tele[0].value), PSTR("%s"), (XdrvMailbox.index == 1) ? "ON" : "OFF");
        Tele[0].change = true;
        break;
    }

    case FUNC_EVERY_SECOND:
    {
        uint8_t speed = LightGetDimmer(0);

        switch (speed)
        {
        case 60:
            if (Tele[1].value[0] != 'L')
            {
                Tele[1].value[0] = 'L';
                Tele[1].change = true;
            }
            break;
        case 80:
            if (Tele[1].value[0] != 'M')
            {
                Tele[1].value[0] = 'M';
                Tele[1].change = true;
            }
            break;
        case 100:
            if (Tele[1].value[0] != 'H')
            {
                Tele[1].value[0] = 'H';
                Tele[1].change = true;
            }
            break;
        default:
            break;
        }

        static uint32_t last_update = 0;

        if (TasmotaGlobal.global_update != last_update)
        {
            last_update = TasmotaGlobal.global_update;

            char temp_str[10];
            char hum_str[10];

            snprintf_P(temp_str, sizeof(temp_str), PSTR("%.1f"), TasmotaGlobal.temperature_celsius);
            if (strcmp(Tele[2].value, temp_str) != 0)
            {
                snprintf_P(Tele[2].value, sizeof(Tele[2].value), PSTR("%s"), temp_str);
                Tele[2].change = true;
            }

            snprintf_P(hum_str, sizeof(hum_str), PSTR("%.1f"), TasmotaGlobal.humidity);
            if (strcmp(Tele[3].value, hum_str) != 0)
            {
                snprintf_P(Tele[3].value, sizeof(Tele[3].value), PSTR("%s"), hum_str);
                Tele[3].change = true;
            }
        }

        TelementaryThingsBoardSend();

        break;
    }
#ifdef USE_WEBSERVER
    case FUNC_WEB_ADD_BUTTON:
        WSContentSend_P(HTTP_FORM_BUTTON, PSTR("tb"), PSTR("ThingsBoard"));
        break;

    case FUNC_WEB_ADD_HANDLER:
        WebServer_on(PSTR("/"
                          "tb"),
                     HandleThingsBoardConfiguration);
        break;
#endif
    }
    return false;
}

#endif // USE_TB_FAN
#endif // USE_THINGSBOARD