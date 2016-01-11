SDK Porting Guide {#sdk_porting_guide}
====================

This readme describes the ways the Exosite Ready SDK can be ported to a platform

The Exosite Ready SDK is designed to be able to work Platform, OS and Network Stack
independently. The core of the SDK uses the OS and network services through wrapper
layers that provide uniform interfaces. Porting of the SDK means implementing of
these wrapper layers basically. 

@image html ER_SDK_porting.jpg

Required interfaces
---------------------
There are 3 interface groups that  you have to do to have all the features of the SDK
running on your system:

 - [System Interface](@ref system_port_interface)

 
   Your platform has to provide some generic system services for the SDK. The System
   interface abstracts various OS/platform dependent features like
     * entering/exiting critical_section 
     * get the system time
     * etc.
   The SDK uses these services through the interface declared in the system_port.h
   file. All of the declared functions have to be implemented.
  
 - [Network Interface](@ref net_port_interface)
  
   Network link layer has to be provide by the platform code. The SDK accesses to 
   this layer through the interface declared in the net_port.h header file. All of 
   the declared functions have to be implemented in the platform specific code.

 - [Thread Interface](@ref thread_port_interface)
  
   The Thread interface abstracts thread/mutext creation and usage The SDK is able to
   use threads to make its operation more efficient however is is able to work in 
   single-thread mode as well. thread_port.h contains the interface specification.
   
   If you want to use the SDK in single-thread mode, you need to provide just dummy 
   implementation as thread related functions. In this case you have to return with 
   ERR_NOT_IMPLEMENTED error code from these implementations. 
   
   Even If, you want to use the SDK in single-thread mode, you have to implement 
   the mutex related functions!



Porting the SDK to an unsupported OS / Network stack
------------------------------------------------------
   
   Since there is no support to your system you have to provide all 3 interface implementations:
   
   * You have to provide a system_port.h implementation.
   * You have to provide a thread_port.h implementation
   * You have to provide a net_port.h implementation
   
Porting the SDK to a supported OS /  Network stack
------------------------------------------------------

   * If you use a supported OS, then you can use an existing implementation of
     system_port and thread_port. You can find existing implementations in the porting/system folder
     These are:
       * No operating system, called EXOS
       * FreeRTOS
       * MQX
       * posix
       
   * If you use a supported Network stack then you can use and existing implementation of the net_port
     These are:
       * UIP
       * LWIP
       * posix
       


