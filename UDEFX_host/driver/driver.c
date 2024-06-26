/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Driver.c

Abstract:

    Main module.

    This driver is for the UDEFX2 Virtual Device.
	
	It is based on the OSR USB Sample for the FX-2 board, 
	and the device interface is similar - but not identical
	 - to the FX2-2.
	 
	The UDEFX2 interface has three endpoints:
	- a bulk out endpoint, for outgoing "missions"
	- a bulk in endpoint, for incoming "response to missions"
	- an interrupt in endpoint, which always brings in an arbitrary
	  4-byte data blob (in the UDE sample, currently 
	  a little endian DWORD with value 0x11 or 0x10 ).
	
	- the way the device is meant to be used is      
	  - send a "mission" via bulk out 
	  - wait for an interrupt to arrive on interrupt/in with value 0x10
	  - get the "mission response" via bulk in

Environment:

    Kernel mode only

--*/

#include <hostude.h>

#include "driver.tmh"

PFN_IO_SET_DEVICE_INTERFACE_PROPERTY_DATA g_pIoSetDeviceInterfacePropertyData;


NTSTATUS
DriverEntry(
    PDRIVER_OBJECT  DriverObject,
    PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL or another NTSTATUS error code otherwise.

--*/
{
    WDF_DRIVER_CONFIG       config;
    NTSTATUS                status;
    WDF_OBJECT_ATTRIBUTES   attributes;
    UNICODE_STRING          funcName;

    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING( DriverObject, RegistryPath );

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT,
                       "OSRUSBFX2 Driver Sample - Driver Framework Edition.\n");
    //
    // The Device interface property set is available on some version, save it
    // into globals (or NULL if not available)
    //
    RtlInitUnicodeString(&funcName, L"IoSetDeviceInterfacePropertyData");
    g_pIoSetDeviceInterfacePropertyData = (PFN_IO_SET_DEVICE_INTERFACE_PROPERTY_DATA) (ULONG_PTR)
        MmGetSystemRoutineAddress(&funcName);


    //
    // Initiialize driver config to control the attributes that
    // are global to the driver. Note that framework by default
    // provides a driver unload routine. If you create any resources
    // in the DriverEntry and want to be cleaned in driver unload,
    // you can override that by manually setting the EvtDriverUnload in the
    // config structure. In general xxx_CONFIG_INIT macros are provided to
    // initialize most commonly used members.
    //

    WDF_DRIVER_CONFIG_INIT(
        &config,
        OsrFxEvtDeviceAdd
        );

    //
    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = OsrFxEvtDriverContextCleanup;

    //
    // Create a framework driver object to represent our driver.
    //
    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        &attributes, // Driver Object Attributes
        &config,          // Driver Config Info
        WDF_NO_HANDLE // hDriver
        );

    if (!NT_SUCCESS(status)) {

        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT,
                "WdfDriverCreate failed with status 0x%x\n", status);
        //
        // Cleanup tracing here because DriverContextCleanup will not be called
        // as we have failed to create WDFDRIVER object itself.
        // Please note that if your return failure from DriverEntry after the
        // WDFDRIVER object is created successfully, you don't have to
        // call WPP cleanup because in those cases DriverContextCleanup
        // will be executed when the framework deletes the DriverObject.
        //
        WPP_CLEANUP(DriverObject);
    }

    return status;
}

VOID
OsrFxEvtDriverContextCleanup(
    WDFOBJECT Driver
    )
/*++
Routine Description:

    Free resources allocated in DriverEntry that are not automatically
    cleaned up by the framework.

Arguments:

    Driver - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
{
    //
    // EvtCleanupCallback for WDFDRIVER is always called at PASSIVE_LEVEL
    //
    _IRQL_limited_to_(PASSIVE_LEVEL);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT,
                    "--> OsrFxEvtDriverContextCleanup\n");

    WPP_CLEANUP( WdfDriverWdmGetDriverObject( (WDFDRIVER)Driver ));

    UNREFERENCED_PARAMETER(Driver); // For the case when WPP is not being used.
}





