/*******************************************************************************
 Stephen Lee HW8
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2013-2014 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/
// DOM-IGNORE-END


// *****************************************************************************
// *****************************************************************************
// Section: Included Files 
// *****************************************************************************
// *****************************************************************************

#include "system_definitions.h"
#include "app.h"
#include "i2c_master_int.h"
#include "i2c_display.h"
#include "accel.h"
// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

/* Recieve data buffer */
uint8_t receiveDataBuffer[64] APP_MAKE_BUFFER_DMA_READY;

/* Transmit data buffer */
uint8_t  transmitDataBuffer[64] APP_MAKE_BUFFER_DMA_READY;

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.
    
    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************



/*int makebinary(int num,int move) {  //num is binary representation of element of the acii array, move will range from 0-7
    int i,a;
    //for (i=0;i<8;i++)
    num=num>>move;
    if (num &1)
        a=1;
    else a=0;
    return a;

}
*/
USB_DEVICE_HID_EVENT_RESPONSE APP_USBDeviceHIDEventHandler
(
    USB_DEVICE_HID_INDEX iHID,
    USB_DEVICE_HID_EVENT event,
    void * eventData,
    uintptr_t userData
)
{
    USB_DEVICE_HID_EVENT_DATA_REPORT_SENT * reportSent;
    USB_DEVICE_HID_EVENT_DATA_REPORT_RECEIVED * reportReceived;

    /* Check type of event */
    switch (event)
    {
        case USB_DEVICE_HID_EVENT_REPORT_SENT:

            /* The eventData parameter will be USB_DEVICE_HID_EVENT_REPORT_SENT
             * pointer type containing details about the report that was
             * sent. */
            reportSent = (USB_DEVICE_HID_EVENT_DATA_REPORT_SENT *) eventData;
            if(reportSent->handle == appData.txTransferHandle )
            {
                // Transfer progressed.
                appData.hidDataTransmitted = true;
            }
            
            break;

        case USB_DEVICE_HID_EVENT_REPORT_RECEIVED:

            /* The eventData parameter will be USB_DEVICE_HID_EVENT_REPORT_RECEIVED
             * pointer type containing details about the report that was
             * received. */

            reportReceived = (USB_DEVICE_HID_EVENT_DATA_REPORT_RECEIVED *) eventData;
            if(reportReceived->handle == appData.rxTransferHandle )
            {
                // Transfer progressed.
                appData.hidDataReceived = true;
            }
          
            break;

        case USB_DEVICE_HID_EVENT_SET_IDLE:

            /* For now we just accept this request as is. We acknowledge
             * this request using the USB_DEVICE_HID_ControlStatus()
             * function with a USB_DEVICE_CONTROL_STATUS_OK flag */

            USB_DEVICE_ControlStatus(appData.usbDevHandle, USB_DEVICE_CONTROL_STATUS_OK);

            /* Save Idle rate recieved from Host */
            appData.idleRate = ((USB_DEVICE_HID_EVENT_DATA_SET_IDLE*)eventData)->duration;
            break;

        case USB_DEVICE_HID_EVENT_GET_IDLE:

            /* Host is requesting for Idle rate. Now send the Idle rate */
            USB_DEVICE_ControlSend(appData.usbDevHandle, & (appData.idleRate),1);

            /* On successfully reciveing Idle rate, the Host would acknowledge back with a
               Zero Length packet. The HID function drvier returns an event
               USB_DEVICE_HID_EVENT_CONTROL_TRANSFER_DATA_SENT to the application upon
               receiving this Zero Length packet from Host.
               USB_DEVICE_HID_EVENT_CONTROL_TRANSFER_DATA_SENT event indicates this control transfer
               event is complete */

            break;
        default:
            // Nothing to do.
            break;
    }
    return USB_DEVICE_HID_EVENT_RESPONSE_NONE;
}

void APP_USBDeviceEventHandler(USB_DEVICE_EVENT event, void * eventData, uintptr_t context)
{
    switch(event)
    {
        case USB_DEVICE_EVENT_RESET:
        case USB_DEVICE_EVENT_DECONFIGURED:

            /* Host has de configured the device or a bus reset has happened.
             * Device layer is going to de-initialize all function drivers.
             * Hence close handles to all function drivers (Only if they are
             * opened previously. */

            BSP_LEDOn  (APP_USB_LED_1);
            BSP_LEDOn  (APP_USB_LED_2);
            BSP_LEDOff (APP_USB_LED_3);
            appData.deviceConfigured = false;
            appData.state = APP_STATE_WAIT_FOR_CONFIGURATION;
            break;

        case USB_DEVICE_EVENT_CONFIGURED:
            /* Set the flag indicating device is configured. */
            appData.deviceConfigured = true;

            /* Save the other details for later use. */
            appData.configurationValue = ((USB_DEVICE_EVENT_DATA_CONFIGURED*)eventData)->configurationValue;

            /* Register application HID event handler */
            USB_DEVICE_HID_EventHandlerSet(USB_DEVICE_HID_INDEX_0, APP_USBDeviceHIDEventHandler, (uintptr_t)&appData);

            /* Update the LEDs */
            BSP_LEDOff (APP_USB_LED_1);
            BSP_LEDOff (APP_USB_LED_2);
            BSP_LEDOn  (APP_USB_LED_3);





            break;

        case USB_DEVICE_EVENT_SUSPENDED:

            /* Switch on green and orange, switch off red */
            BSP_LEDOff (APP_USB_LED_1);
            BSP_LEDOn  (APP_USB_LED_2);
            BSP_LEDOn  (APP_USB_LED_3);
            break;

        case USB_DEVICE_EVENT_POWER_DETECTED:

            /* VBUS was detected. We can attach the device */

            USB_DEVICE_Attach (appData.usbDevHandle);
            break;

        case USB_DEVICE_EVENT_POWER_REMOVED:

            /* VBUS is not available */
            USB_DEVICE_Detach(appData.usbDevHandle);
            break;

        /* These events are not used in this demo */
        case USB_DEVICE_EVENT_RESUMED:
        case USB_DEVICE_EVENT_ERROR:
        default:
            break;
    }
}

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary local functions.
*/


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;
    
    appData.usbDevHandle = USB_DEVICE_HANDLE_INVALID;
    appData.deviceConfigured = false;
    appData.txTransferHandle = USB_DEVICE_HID_TRANSFER_HANDLE_INVALID;
    appData.rxTransferHandle = USB_DEVICE_HID_TRANSFER_HANDLE_INVALID;
    appData.hidDataReceived = false;
    appData.hidDataTransmitted = true;
    appData.receiveDataBuffer = &receiveDataBuffer[0];
    appData.transmitDataBuffer = &transmitDataBuffer[0];
}


/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks (void )
{



  
 




    /* Check if device is configured.  See if it is configured with correct
     * configuration value  */

    switch(appData.state)
    {
        case APP_STATE_INIT:

            /* Open the device layer */
            appData.usbDevHandle = USB_DEVICE_Open( USB_DEVICE_INDEX_0, DRV_IO_INTENT_READWRITE );

            if(appData.usbDevHandle != USB_DEVICE_HANDLE_INVALID)
            {
                /* Register a callback with device layer to get event notification (for end point 0) */
                USB_DEVICE_EventHandlerSet(appData.usbDevHandle, APP_USBDeviceEventHandler, 0);

                appData.state = APP_STATE_WAIT_FOR_CONFIGURATION;
            }
            else
            {
                /* The Device Layer is not ready to be opened. We should try
                 * again later. */
            }

            break;

        case APP_STATE_WAIT_FOR_CONFIGURATION:

            if(appData.deviceConfigured == true)
            {
                /* Device is ready to run the main task */
                appData.hidDataReceived = false;
                appData.hidDataTransmitted = true;
                appData.state = APP_STATE_MAIN_TASK;

                /* Place a new read request. */
                USB_DEVICE_HID_ReportReceive (USB_DEVICE_HID_INDEX_0,
                        &appData.rxTransferHandle, appData.receiveDataBuffer, 64);
            }
            break;

        case APP_STATE_MAIN_TASK:

            if(!appData.deviceConfigured)
            {
                /* Device is not configured */
                appData.state = APP_STATE_WAIT_FOR_CONFIGURATION;
            }
            else if( appData.hidDataReceived )
            {
                /* Look at the data the host sent, to see what
                 * kind of application specific command it sent. */

                 char message[200],binary[20];
    //scanf("%s", message);

                int startrow,len,rowlen, startcol=32, element,i;
                switch(appData.receiveDataBuffer[0])
                {
                    case 0x80:
                       //display_init;
                       display_clear();
                       //display_draw();
                        memset(message,0,200);
                        /* Toggle on board LED1 to LED2. */
                        BSP_LEDToggle( APP_USB_LED_1 );
                        BSP_LEDToggle( APP_USB_LED_2 );

                        


                       i=0;
                       while((appData.receiveDataBuffer[i+1]>='0')&&(appData.receiveDataBuffer[i+1]<='9')){
                            message[i]=appData.receiveDataBuffer[i+1];
                            i++;
                        }

                       startrow=atoi(message);
                       rowlen=strlen(message);
                     
                        //sprintf(message,"hello");


                

                         
                           len=appData.receiveDataBuffer[i+1];
                        for(i=0; i<len; i++){
                            message[i]=appData.receiveDataBuffer[rowlen+2+i];
                        }


                           display_message(message,startrow,startcol);
                      /*   while(message[u]){
                         row=message[u]-0x20;
                        for (x = 0; x < 5; x++) {
                                element = ASCII[row][x];
                                for (j = 0; j < 8; j++) {
                                    x = makebinary(element, j);
                                    display_pixel_set(startrow + j, startcol + x + u * 5, x);
                                }
                            }
                            u++;
                        }*/

                        display_draw();

                           appData.hidDataReceived = false;

                        /* Place a new read request. */
                        USB_DEVICE_HID_ReportReceive (USB_DEVICE_HID_INDEX_0,
                                &appData.rxTransferHandle, appData.receiveDataBuffer, 64 );

                        break;

                    case 0x81:

                        if(appData.hidDataTransmitted)
                        {
                            /* Echo back to the host PC the command we are fulfilling in
                             * the first byte.  In this case, the Get Push-button State
                             * command. */

                            appData.transmitDataBuffer[0] = 0x81;

                            short accels[3]; // accelerations for the 3 axes
                            char row=2, col= 5;
                            acc_read_register(OUT_X_L_A, (unsigned char *) accels, 6);
                            sprintf(message,"x: %d y:%d, z:%d",accels[0],accels[1],accels[2]);
                            //sprintf(message,"x: %.1f  y: %.1f   z: %.1f  ",x,y,z);
                            display_message(message, row, col);
                            display_draw();
                            if(_CP0_GET_COUNT()>200000){
                                appData.transmitDataBuffer[1]=1;
                                appData.transmitDataBuffer[2]=accels[0]>>8;
                                appData.transmitDataBuffer[3]=accels[0]&0xFF;
                                appData.transmitDataBuffer[4]=accels[1]>>8;
                                appData.transmitDataBuffer[5]=accels[1]&0xFF;
                                appData.transmitDataBuffer[6]=accels[2]>>8;
                                appData.transmitDataBuffer[7]=accels[2]&0xFF;
                                _CP0_SET_COUNT(0);

                            }
                            else{
                                appData.transmitDataBuffer[1]=0;
                            }
                       
                           /* if( BSP_SwitchStateGet(APP_USB_SWITCH_1) == BSP_SWITCH_STATE_PRESSED )
                            {
                                appData.transmitDataBuffer[1] = 0x00;
                            }
                            else
                            {
                                appData.transmitDataBuffer[1] = 0x01;
                            }*/




                            appData.hidDataTransmitted = false;

                            /* Prepare the USB module to send the data packet to the host */
                            USB_DEVICE_HID_ReportSend (USB_DEVICE_HID_INDEX_0,
                                    &appData.txTransferHandle, appData.transmitDataBuffer, 64 );

                            appData.hidDataReceived = false;

                            /* Place a new read request. */
                            USB_DEVICE_HID_ReportReceive (USB_DEVICE_HID_INDEX_0,
                                    &appData.rxTransferHandle, appData.receiveDataBuffer, 64 );
                        }
                        break;

                    default:

                        appData.hidDataReceived = false;

                        /* Place a new read request. */
                        USB_DEVICE_HID_ReportReceive (USB_DEVICE_HID_INDEX_0,
                                &appData.rxTransferHandle, appData.receiveDataBuffer, 64 );
                        break;
                }
            }
        case APP_STATE_ERROR:
            break;
        default:
            break;
    }
}
 

/*******************************************************************************
 End of File
 */

