/**
 *  @file exosite_send_http.c
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
 *  @example exosite_send_http.c
 *  This example demonstrates how to update a data resource on the server
 *  over HTTP.
 *  It increments a software counter then sends this value to the "Counter"
 *  data resource. After a while, the value of the resource is read back
 *  in order to be sure that the data resource has been updated correctly.
 *
 *  @note
 *- Make sure that the "count" data resource is added to your
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

static char counter_str[16];
static int counter;

/**
 *  This function is called when the read operation is finished.
 *
 *  @param[in] status   Status of the read operation. It is ERR_SUCCESS
 *                      in case of success. Otherwise it is <0.
 *  @param[in] alias    Alias of the data resource.
 *  @param[in] value    Value of the data resource
 */
static void on_read(int status, const char *alias, const char *value)
{
    if (status == ERR_SUCCESS)
        printf("Read \"%s\": %s\n", alias, value);
}

/**
 *  This function is called when the write operation is finished.
 *
 *  @param[in] status   Status of the wrire operation. It is ERR_SUCCESS
 *                      in case of success. Otherwise it is <0.
 *  @param[in] alias    Alias of the data resource.
 *  @param[in] value    Value of the data resource
 */
static void on_write(int status, const char *alias)
{
    if (status == ERR_SUCCESS)
        printf("Write \"%s\"\n", alias);
}

/**
 * Entry point of the example
 */
int main(int argc, char *argv[])
{
    struct exosite_class *exo;
    enum exosite_device_status status;
    int error;
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

    info_log(DEBUG_APPLICATION, ("Send http example\n"));

    /** Create new instance */
    error = exosite_new(&exo, vendor, model, sn, APP_PROTO_HTTP);
    if (error)
        return ERR_FAILURE;

    /** Wait until the activation is finished */
    do {
        status = exosite_get_status(exo);
        exosite_delay_and_poll(exo, 200);
    } while (status != DEVICE_STATUS_ACTIVATED);

    while (1) {

        /** Write new value into the "Counter" data resource */
        sprintf(counter_str, "%d", counter++);
        exosite_write(exo, "count", counter_str, on_write);
        exosite_delay_and_poll(exo, 2000);

        /** Read the current value of "Counter" data resource */
        exosite_read(exo, "count", on_read);
        exosite_delay_and_poll(exo, 2000);
    }

    return ERR_SUCCESS;
}

