#ifdef USE_THINGSBOARD
#ifdef USE_TB_LIGHTBULB

bool Xdrv100(uint32_t function)
{
    switch (function)
    {
    case FUNC_INIT:
    {
        Tele = new Telementary_data[3]{
            {"POWER", "", false},
            {"Color", "", false},
            {"CT", "", false}};
        TeleSize = 3;

        snprintf_P(tb_host, sizeof(tb_host), PSTR("%s"), SettingsText(SET_MEM15));
        snprintf_P(tb_token, sizeof(tb_token), PSTR("%s"), SettingsText(SET_MEM16));
        if (!tb_host[0] || !tb_token[0])
        {
            AddLog(LOG_LEVEL_INFO, PSTR("TB : ThingsBoard HTTP Initialized"));
        }

        uint8_t mac[6];
        WiFi.macAddress(mac);

        char host_name[20];
        snprintf(host_name, sizeof(host_name), "Bulb-%02X%02X%02X", mac[3], mac[4], mac[5]);

        if (!strstr(SettingsText(SET_HOSTNAME), "Bulb"))
        {
            SettingsUpdateText(SET_HOSTNAME, host_name);
        }

        if (!Settings->flag3.mdns_enabled)
        {
            ExecuteCommand("SetOption55 1", SRC_IGNORE);
        }

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

    case FUNC_EVERY_50_MSECOND:
    {
        RPCThingsBoardFetch();
        break;
    }

    case FUNC_EVERY_SECOND:
    {
        char color_str[20];

        LightGetColor(color_str, sizeof(color_str));

        if (strcmp(Tele[1].value, color_str) != 0)
        {
            snprintf_P(Tele[1].value, sizeof(Tele[1].value), PSTR("%s"), color_str);
            Tele[1].change = true;
            if (LightGetColorTemp() != 0)
            {
                snprintf_P(Tele[2].value, sizeof(Tele[2].value), PSTR("%d"), LightGetColorTemp());
                Tele[2].change = true;
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

#endif // USE_TB_LIGHTBULB
#endif // USE_THINGSBOARD