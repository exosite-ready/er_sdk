User API
==========

./include/ folder contains the User APIs that provide access to
the Exosite SDK services.

![Block diagram](../resources/ER_SDK_user_if.jpg)

@warning
Only these headers are necessary to use the Exosite Ready SDK.
If you include any other header from the internal layers, you may
not use the SDK the right way.

@warning
DO NOT FORGET - Operation of the SDK is based on a state machine.
In ExOS (none-threading) mode, the user application has to call
the exosite_poll or exosite_delay_and_poll function periodically
to make the state machine work. Otherwise (If one of the
supported OSs is available), it is not needed.

Supported APIs
----------------

| API                                 | Definition         | Description                                              |
| ----------------------------------- |:------------------ |:-------------------------------------------------------- |
| [Non-blocking API]                  | exosite_api.h      | Non-blocking API. The caller thread will not be blocked. |
| [Configurator API]                  | configurator_api.h | Provide access to the WiFi setting configurator server   |


[Non-blocking API]: @ref Exosite_API
[Configurator API]: @ref Configurator_API

