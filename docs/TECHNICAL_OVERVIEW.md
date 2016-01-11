 Technical Overview
===================================================================

The Exosite Ready SDK is a collection of software modules that allows 
to easily compile modular Exosite Client software for various HW platforms 
supported by Exosite. It has built-in support for various network protocols
and to provide flexible access to the Exosite services.

The Exosite Ready SDK is ideal for the embedded environments.

Features
---------
 * Cross-platform; the source code is written in C
 * The SDK core is std99 compliant C sources
 * Flexible and Extendible architecture
 * Built-in support for FreeRTOS, QMX, OS X and Linux
 * ExOS mode (Single-Threaded mode without Operating System )
 * Easy-to-Use, non-blocking API
 * Exosite Provisioning support
 * Highly Configurable to match various HW constraints
 * Built-in protocols:
   - HTTP
   - Co-AP
 * Security
   - TLS
   - DTLS
 * Tiny static and run-time footprint

Definitons
-------------

- **ExOS**: The non-threading lightweight OS emulator
- **Exosite Client (software)**: Software that can send/receive data to/from the Exosite servers

Exosite Ready SDK™ with RTOS
===================================
This section provides an overview of the Exosite Ready SDK with a RTOS, 
including a block diagram and a description of both the internal and external
modules involved in the system. 

## System Block diagram

  The following figure provides a block diagram of a system in which the SDK 
  with RTOS has been implemented.
  - Items in red identify software components provided as part of the SDK.
  - Items in orange represent wrapper modules that has to be implemented by the
    user during the SDK porting.
  - Items in grey are project-specific and are not provided as part of the SDK;
    they must be provided by the user, a third-party vendor, or by Exosite  
    through a professional services contract. 
    
  **NOTE:** The block diagram depicts a typical system architecture; actual  
            project implementations may differ slightly. 
  
  **NOTE:** The SDK contains ports for a couple of operating systems and  
            network stacks. These implementations can be used as examples and 
            located in the er_sdk/porting/ folder.
  
  @image html ER_SDK_with_RTOS.jpg

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
## Modules of the Exosite Activator SDK

 This section provides information about the modules included as part of the
 SDK. These modules are shown in red in the block diagram above. 
 
  _______________________________
  ### Exosite Activator SDK core

  The core module implements the core functionality of the SDK and allows the 
  user application to access Exosite services. Operation of the SDK is based 
  on a state machine. In ExOS mode, the user application has to call the 
  exosite_poll function periodically to make the state machine work. Otherwise 
  (If one of the supported OSs is available), the SDK creates internal threads 
  for the state machine.
  
  **Interface definition:**
  er_sdk/include/exosite_api.h
  
  **Source code:**
  er_sdk/src/*.c
  
  **WARNING:**
  Any modifications to this component is not recommended.

  ___________________
  ### Network wrapper

  This module is a thin wrapper for the network stack. Its purpose is to cover 
  distinctions between various network stack implementations. The interface is 
  defined in net_port.h should be implemented by this module. 

  **Interface definition:**
  er_sdk/porting/net_port.h

  **Source code:**
  User defined. Recommended location is er_sdk/porting/net/<network_stack>/

  **NOTE:**
  Currently uIP, lwip, Microchip and Qualcomm network stacks are supported by 
  Exosite. er_sdk/porting/net/ folder contains the implementations for these 
  network stacks.
  
  *See [Network Port Interface] for more details*
  
  __________________
  ### System wrapper
  
  The system wrapper module provides a uniform interface for the core module 
  that covers the distinctions between various operating systems (OSs) (or the 
  ExOS module). It provides general system functions, including system timers, 
  system reset, and watchdog handling. It is important to note that the system 
  wrapper module must be implemented for each OS.

  
  **Interface definition:**
  er_sdk/porting/system_port.h

  **Source code:**
  User defined. Recommended location is er_sdk/porting/system/<os>/system.c
  
  **NOTE:**
  Currently, POSIX (Linux, OS X), MQX and FreeRTOS OS implementations are
  available. er_sdk/porting/system/ folder contains the implementations 
  for these operating systems.

  *See [System Port Interface] for more details*
  
  __________________
  ### Thread wrapper

  This module provides a uniform interface for the core module. It covers 
  the distinctions between various operating systems (or the ExOS module).
  It provides a toolkit for concurrent programming: threads and mutexes.
  
  **Interface definition:**
  er_sdk/porting/thread_port.h

  **Source code:**
  User defined. Recommended location is er_sdk/porting/net/<os>/thread.c and
  er_sdk/porting/net/<os>/mutex.c
  
  **NOTE:**
  At the moment we have implementation for POSX (Linux, OS X), MQX and FreeRTOS 
  Operating Systems. er_sdk/porting/system/ folder contains the implementations 
  for these operating systems.
  
  *See [Thread Port Interface] for more details*
  
  ______________
  ### SDK Config

  This module provides provides settings that are able to tune the Exosite  
  Ready SDK to be suitable to the application and the environment.
  
  **Implementation:**
  er_sdk/porting/config_port.h
  
  *See [SDK Configuration] for more details*
  
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
## External Modules

  This section provides information about the external modules that are not 
  included as part of the Exosite Ready SDK™. These modules are shown in grey 
  in the block diagram above. 
  
  ___________________________
  ### Platform-dependent Code

  The platform-dependent code module implements the platform-dependent 
  initialisations, functions, routines, and algorithms. It typically contains 
  the startup code and initialization of the hardware peripherals. It can also 
  provides several functions for the SDK, like platform_reset, if these
  functions are not accessible through the OS layer. 
  If an OS is used, additional OS-specific functions must be implemented that are 
  OS specific.
  
  ______
  ### OS
  
  If an OS will be used, it must be ported to the corresponding platform. The 
  services of the OS are accessible directly from the application layer, but 
  the Exosite Ready SDK™ uses it as well. The SDK accesses the OS services  
  through the system wrapper and thread wrapper modules that provides an 
  OS-independent interface. See System Wrapper and Thread Wrapper chapters 
  above for additional  information.
  
  _________________
  ### Network Stack
  Typically, a third-party network stack module works with the selected OS.
  The services of the network stack are accessible directly from the 
  application layer; however, this usage is not recommended. The SDK accesses
  the services through the Network Wrapper module that provides a generic 
  interface. See Network Wrapper section above for additional information. 

  ____________________
  ### User Application
  The user application can access the OS and the network stack directly through 
  the corresponding interfaces. The SDK services are accessible through the 
  Exosite API. It is defined in exosite_api.h.
  
  **NOTE:** The SDK contains example applications in the /examples folder.

Exosite Ready SDK™ without RTOS
================================
This section provides an overview of the Exosite Ready SDK without a RTOS, 
including a block diagram and a description of both the internal and external
modules involved in the system.

  ## System Block diagram
  
  The figure provides a block diagram of a system in which the SDK has been 
  implemented. In this system, the ExOS non-threading lightweight OS emulator 
  is used in place of RTOS. 
    - Items in red identify software components provided as part of the SDK.
    - Items in orange represent wrapper modules that has to be implemented by the
      user during the SDK porting.
    - Items in grey are project-specific and are not provided as part of the SDK;
      they must be provided by the user, a third-party vendor, or by Exosite  
      through a professional services contract. 
    
  **NOTE:** The block diagram depicts a typical system architecture; actual  
            project implementations may differ slightly.

  @image html ER_SDK_with_ExOS.jpg
  
  ## ExOS
  In embedded systems where system resources are limited, ExOS can be used. ExOS
  is an OS emulator layer that provides the necessary interfaces for the upper 
  layers with limited functionality.  The Exosite Ready SDK™ currently defaults to 
  the uIP network stack. When ExOS is used, the network communication is served 
  by this module.
  
  ### Advantages
  * Low system resource usage.
  * In most cases, porting ExOS requires less effort than porting a RTOS.
  * The system wrapper and the network wrapper are ready for ExOS.
  
  ### Disadvantages
  * ExOS-specific platform code must be implemented.
  * The application code cannot use OS services.
  * The application code cannot access the network stack directly.



[Network Port Interface]: @ref net_port_interface
[System Port Interface]: @ref system_port_interface
[Thread Port Interface]: @ref thread_port_interface
[SDK Configuration]: @ref config_port

