/**
 *  @file exosite_subscribe_http.c
 *
 *  @copyright
 *  Please read the Exosite Copyright: @ref copyright
 *
 *  @if Authors
 *  @authors
 *    - Szilveszter Balogh (szilveszterbalogh@exosite.com)
 *    - Zoltan Ribi (zoltanribi@exosite.com)
 *  @endif
 *
 *  @example exosite_subscribe_http.c
 *  This example demonstrates how to subscribe to a data resource on the
 *  server over HTTP.
 *  It subscribes to the "Dev1" data resource, and show a message when its
 *  value is updated. (For example a new value is set on the Exosite
 *  portal)
 *
 *  @note
 *- Make sure that the "Dev1" data resource is added to your
 *  device on the Exosite OpenTether portal.
 *
 *  @note
 *- If your device has not been activated, the activation process
 *  will be performed when this example runs first time. The given CIK will
 *  be stored in the device's permanent memory. Later this stored CIK
 *  will be used for the communication.
 *
 *  @note
 *- Make sure that your device does not store an invalid CIK in
 *  its permanent storage. (For example the device was activated before,
 *  but the assigned device on the OpenTeter portal has been deleted).
 *  If it does, you have to clear it to enforce a new activation process.
 */

#include <stdio.h>
#include <lib/type.h>
#include <lib/error.h>
#include <lib/debug.h>
#include <exosite_api.h>

/**
 *  This function is called when the data resource changed on the server.
 *
 *  @param[in] status   Status of the read operation. It is ERR_SUCCESS
 *                      in case of success. Otherwise it is <0.
 *  @param[in] alias    Alias of the data resource.
 *  @param[in] value    Value of the data resource
 */
static void on_change(int status, const char *alias, const char *value)
{
    static BOOL notify_state = FALSE;

    if (status == ERR_SUCCESS) {
        printf("Value changed on server \"%s\" to %s\n", alias, value);
        notify_state = !notify_state;
    }
}

/**
 * Entry point of the example
 */
int main(int argc, char *argv[])
{
    struct exosite_class *exo;
    int error;
    enum exosite_device_status status;
    char *vendor, *model, *sn;

    if (argc < 4) {
        printf("Please specify the VENDOR, MODEL, SERIAL_NUMBER of your device and run again\n");
        printf("Without VENDOR, MODEL, SERIAL_NUMBER the test cannot connect connect to the Exosite cloud\n");
        return ERR_FAILURE;
    }
    vendor = argv[1];
    model = argv[2];
    sn = argv[3];

    /** SDK initialization */
    error = exosite_sdk_init(printf, NULL);
    if (error)
        return ERR_FAILURE;

    info_log(DEBUG_APPLICATION, ("Subscribe http example\n"));

    /** Create new instance */
    error = exosite_new(&exo, vendor, model, sn, APP_PROTO_HTTP);
    if (error)
        return ERR_FAILURE;

    /** Wait until the activation is finished */
    do {
        status = exosite_get_status(exo);
        exosite_delay_and_poll(exo, 200);
    } while (status != DEVICE_STATUS_ACTIVATED);

    /** Subscribe to the Dev1 data source */
    do {
        error = exosite_subscribe(exo, "led", 0, on_change);
    } while (error);

    while (1)
        exosite_poll(exo);

    return ERR_SUCCESS;
}

