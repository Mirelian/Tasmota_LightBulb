#ifdef USE_THINGSBOARD
#ifdef USE_TB_LIGHTBULB

bool RPCThingsBoardDeviceProcess(const char *method, JsonParserObject root)
{
    char command[32];

    if (strcmp(method, "setCT") == 0)
    {
        snprintf_P(command, sizeof(command),
                   PSTR("CT %u"),
                   root[PSTR("params")].getUInt());
    }
    else if (strcmp(method, "setHue") == 0)
    {
        snprintf_P(command, sizeof(command),
                   PSTR("HsbColor1 %u"),
                   root[PSTR("params")].getUInt());
    }
    else if (strcmp(method, "setSaturation") == 0)
    {
        snprintf_P(command, sizeof(command),
                   PSTR("HsbColor2 %u"),
                   root[PSTR("params")].getUInt());
    }
    else if (strcmp(method, "setDimmer") == 0)
    {
        snprintf_P(command, sizeof(command),
                   PSTR("Dimmer %u"),
                   root[PSTR("params")].getUInt());
    }
    else if (strcmp(method, "setPower") == 0)
    {
        snprintf_P(command, sizeof(command),
                   PSTR("Power %u"),
                   root[PSTR("params")].getBool());
    }
    else
    {
        return false;
    }

    ExecuteCommand(command, SRC_WEBGUI);

    return true;
}

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

        ThingsBoardInit("Bulb");

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