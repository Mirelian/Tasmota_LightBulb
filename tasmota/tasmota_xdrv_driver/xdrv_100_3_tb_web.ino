#ifdef USE_THINGSBOARD
#ifdef USE_WEBSERVER

const char HTTP_FORM_THINGSBOARD[] PROGMEM =
    "<p><b>Host</b><br>"
    "<input id='tb_host' placeholder='demo.thingsboard.io' value='%s'></p>"
    "<p><label><b>Token</b><input type='checkbox' onclick='sp(\"tb_token\")'></label><br>"
    "<input id='tb_token' type='password' placeholder='Device token' value='%s'></p>";

void ThingsBoardSaveSettings()
{
    char host[sizeof(tb_host)];
    char token[sizeof(tb_token)];

    WebGetArg(PSTR("tb_host"), host, sizeof(host));
    WebGetArg(PSTR("tb_token"), token, sizeof(token));

    SettingsUpdateText(SET_MEM15, host);
    SettingsUpdateText(SET_MEM16, token);

    snprintf_P(tb_host, sizeof(tb_host), PSTR("%s"), SettingsText(SET_MEM15));
    snprintf_P(tb_token, sizeof(tb_token), PSTR("%s"), SettingsText(SET_MEM16));

    AddLog(LOG_LEVEL_INFO, PSTR("TB: ThingsBoard credentials saved"));
}

void HandleThingsBoardConfiguration(void)
{
    if (!HttpCheckPriviledgedAccess())
    {
        return;
    }

    if (Webserver->hasArg(F("save")))
    {
        ThingsBoardSaveSettings();
        WebRestart(1);
        return;
    }

    WSContentStart_P(PSTR("Configure ThingsBoard"));
    WSContentSendStyle();
    WSContentSend_P(HTTP_FORM_GET_ACTION, PSTR("tb"));
    WSContentSend_P(HTTP_FIELDSET_LEGEND, PSTR("ThingsBoard parameters"));
    WSContentSend_P(
        HTTP_FORM_THINGSBOARD,
        SettingsTextEscaped(SET_MEM15).c_str(),
        "****");
    WSContentSend_P(HTTP_FORM_END);
    WSContentSpaceButton(BUTTON_CONFIGURATION);
    WSContentStop();
}

#endif // USE_WEBSERVER
#endif // USE_THINGSBOARD