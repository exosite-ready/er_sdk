/*
 * configurator_server_res.c
 *
 *  Created on: Dec 15, 2015
 *      Author: Zoltan Ribi
 */

char configurator_html_opener[] = "\
<html>\
<body>\
<h1>Network settings</h1>\
SN: \
";

char configurator_wifi_setting_form[] = "\
<hr>\
<form action=\"network_settings\">\
Security: <select name=\"security\">\
<option value=\"open\">Open</option>\
<option value=\"wpa2\" selected>WPA2</option>\
</select><br>\
SSID: <input type=\"text\" name=\"ssid\"><br>\
Passphrase: <input type=\"text\" name=\"passphrase\"><br>\
<input type=\"submit\" value=\"Submit\">\
</form>\
<hr>\
";

char configurator_error_notification[] = "<h2>Error</h2>";

char configurator_success_notification[] = "<h2>Success</h2>";

char configurator_html_closer[] = "\
</body>\
</html>\
";
