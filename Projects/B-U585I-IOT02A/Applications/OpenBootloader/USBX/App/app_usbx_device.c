/**
  ******************************************************************************
  * @file    app_usbx_device.c
  * @author  MCD Application Team
  * @brief   USBX Device applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "app_usbx_device.h"

/* Private includes ----------------------------------------------------------*/
#include "main.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_device_stack.h"
#include "ux_dcd_stm32.h"
#include "ux_device_descriptors.h"
#include "ux_device_dfu_media.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Thread priority */
#define DEFAULT_THREAD_PRIO                   10U
/* Thread preemption priority */
#define DEFAULT_PREEMPTION_THRESHOLD          DEFAULT_THREAD_PRIO

/* USB App Stack Size */
#define USBX_APP_STACK_SIZE                   (2U * 1024U)

/* Usb Memory Size */
#define USBX_MEMORY_SIZE                      (32U * 1024U)

#if defined ( __ICCARM__ ) /* IAR Compiler */
#pragma data_alignment=4
#endif /* defined ( __ICCARM__ ) */
__ALIGN_BEGIN ux_dfu_downloadInfotypeDef  ux_dfu_download  __ALIGN_END;

/* Private macro -------------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
UX_SYSTEM_SLAVE              *ux_system;
TX_QUEUE                     ux_app_MsgQueue;
TX_THREAD                    ux_app_thread;
TX_THREAD                    usbx_dfu_download_thread;
UX_SLAVE_CLASS_DFU_PARAMETER dfu_parameter;

/* Private variables ---------------------------------------------------------*/
static TX_QUEUE ux_usbpd_app_MsgQueue;

/* External variables --------------------------------------------------------*/
extern PCD_HandleTypeDef hpcd_USB_OTG_FS;

/* Private function prototypes -----------------------------------------------*/
void usbx_app_thread_entry(ULONG arg);
void MX_USB_Device_Init(void);

/**
  * @brief  Application USBX Device Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT MX_USBX_Device_Init(VOID *memory_ptr)
{
  UINT ret;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL *)memory_ptr;

  CHAR *pointer;
  ULONG device_framework_hs_length;
  ULONG device_framework_fs_length;
  ULONG string_framework_length;
  ULONG languge_id_framework_length;
  UCHAR *device_framework_high_speed;
  UCHAR *device_framework_full_speed;
  UCHAR *string_framework;
  UCHAR *language_id_framework;
  UINT tx_status;

  /* Allocate USBX_MEMORY_SIZE. */
  tx_status = tx_byte_allocate(byte_pool, (VOID **) &pointer, USBX_MEMORY_SIZE, TX_NO_WAIT);

  /* Check memory allocation */
  if (UX_SUCCESS != tx_status)
  {
    Error_Handler();
    return tx_status;
  }

  /* Initialize USBX Memory */
  ux_system_initialize(pointer, USBX_MEMORY_SIZE, UX_NULL, 0U);

  /* Get_Device_Framework_High_Speed and get the length */
  device_framework_high_speed = USBD_Get_Device_Framework_Speed(USBD_HIGH_SPEED, &device_framework_hs_length);

  /* Get_Device_Framework_Full_Speed and get the length */
  device_framework_full_speed = USBD_Get_Device_Framework_Speed(USBD_FULL_SPEED, &device_framework_fs_length);

  /* Get_String_Framework and get the length */
  string_framework = USBD_Get_String_Framework(&string_framework_length);

  /* Get_Language_Id_Framework and get the length */
  language_id_framework = USBD_Get_Language_Id_Framework(&languge_id_framework_length);

  /* The code below is required for installing the device portion of USBX.
     In this application */
  ret =  _ux_device_stack_initialize(device_framework_high_speed,
                                     device_framework_hs_length,
                                     device_framework_full_speed,
                                     device_framework_fs_length,
                                     string_framework,
                                     string_framework_length,
                                     language_id_framework,
                                     languge_id_framework_length,
                                     DFU_Device_ConnectionCallback);

  /* Check the device stack class status */
  if (ret != UX_SUCCESS)
  {
    Error_Handler();
    return ret;
  }

  /* Initialize the DFU parameters for the device. */
  dfu_parameter.ux_slave_class_dfu_parameter_instance_activate = DFU_Init;

  /* DeInitialize the DFU parameters for the device. */
  dfu_parameter.ux_slave_class_dfu_parameter_instance_deactivate = DFU_DeInit;

  /* Set the callback function "DFU_GetStatus" to get status parameter */
  dfu_parameter.ux_slave_class_dfu_parameter_get_status = DFU_GetStatus;

  /* Set the callback function "DFU_Read" to read parameter */
  dfu_parameter.ux_slave_class_dfu_parameter_read = DFU_Read;

  /* Set the callback function "DFU_Write" to write parameter */
  dfu_parameter.ux_slave_class_dfu_parameter_write = DFU_Write;

  /* Set the callback function "DFU_Notify" to notify parameter */
  dfu_parameter.ux_slave_class_dfu_parameter_notify = DFU_Notify;

  /* Manage the leave DFU requests */
  dfu_parameter.ux_device_class_dfu_parameter_custom_request = DFU_Leave;

  /* Set device framework parameter */
  dfu_parameter.ux_slave_class_dfu_parameter_framework = device_framework_full_speed;

  /* Set device framework length parameter */
  dfu_parameter.ux_slave_class_dfu_parameter_framework_length = device_framework_fs_length;

  /* Initialize the device dfu class. The class is connected with interface 0 */
  ret = ux_device_stack_class_register(_ux_system_slave_class_dfu_name,
                                       ux_device_class_dfu_entry, 1U, 0U,
                                       (VOID *)&dfu_parameter);

  /* Check the device stack class status */
  if (ret != UX_SUCCESS)
  {
    Error_Handler();
    return ret;
  }

  /* Allocate the stack for main_usbx_app_thread_entry */
  tx_byte_allocate(byte_pool, (VOID **) &pointer, USBX_APP_STACK_SIZE, TX_NO_WAIT);

  /* Create the usbx_app_thread_entry */
  ret = tx_thread_create(&ux_app_thread, "main_usbx_app_thread_entry",
                         usbx_app_thread_entry, 0,
                         pointer, USBX_APP_STACK_SIZE,
                         DEFAULT_THREAD_PRIO, DEFAULT_PREEMPTION_THRESHOLD,
                         TX_NO_TIME_SLICE, TX_AUTO_START);

  /* Check memory allocation */
  if (UX_SUCCESS != ret)
  {
    Error_Handler();
    return ret;
  }

  /* Allocate the stack for usbx_dfu_download_thread_entry. */
  tx_status = tx_byte_allocate(byte_pool, (VOID **) &pointer, USBX_APP_STACK_SIZE, TX_NO_WAIT);

  /* Check usbx_dfu_download_thread_entry creation */
  if (UX_SUCCESS != tx_status)
  {
    return tx_status;
  }

  /* Create the usbx_dfu_download_thread_entry thread. */
  tx_status = tx_thread_create(&usbx_dfu_download_thread,
                               "usbx_dfu_download_thread_entry",
                               usbx_dfu_download_thread_entry, 0U,
                               pointer, USBX_APP_STACK_SIZE, 20U, 20U,
                               TX_NO_TIME_SLICE,
                               TX_AUTO_START);

  /* Check usbx_cdc_acm_read_thread_entry creation */
  if (UX_SUCCESS != tx_status)
  {
    return tx_status;
  }
  /* Allocate Memory for the Queue */
  ret = tx_byte_allocate(byte_pool, (VOID **) &pointer,
                         APP_QUEUE_SIZE * sizeof(ux_dfu_downloadInfotypeDef),
                         TX_NO_WAIT);

  if (ret != TX_SUCCESS)
  {
    ret = TX_POOL_ERROR;
  }

  /* Create the MsgQueue */
  ret = tx_queue_create(&ux_app_MsgQueue, "Message Queue app",
                        sizeof(ux_dfu_downloadInfotypeDef),
                        pointer,
                        APP_QUEUE_SIZE * sizeof(ux_dfu_downloadInfotypeDef)) ;

  if (ret != TX_SUCCESS)
  {
    ret = TX_QUEUE_ERROR;
  }

  /* Allocate Memory for the Queue */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer,
                       sizeof(APP_QUEUE_SIZE * sizeof(ULONG)),
                       TX_NO_WAIT) != TX_SUCCESS)
  {
    ret = TX_POOL_ERROR;
  }

  /* Create the MsgQueue */
  if (tx_queue_create(&ux_usbpd_app_MsgQueue, "Usbpd Message Queue app",
                      TX_1_ULONG, pointer,
                      APP_QUEUE_SIZE * sizeof(ULONG)) != TX_SUCCESS)
  {
    ret = TX_QUEUE_ERROR;
  }

  return ret;
}

/**
  * @brief  Function implementing usbx_app_thread_entry.
  * @param Arg: Not used
  * @retval None
  */
void usbx_app_thread_entry(ULONG Arg)
{
  /* Initialization of USB device */
  MX_USB_Device_Init();
}

/**
  * @brief  Initialization of USB device.
  * @retval None
  */
void MX_USB_Device_Init(void)
{
  /* Set Rx FIFO */
  HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_FS, 0x100U);
  /* Set Tx FIFO 0 */
  HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 0, 0x40U);

  /* initialize the device controller driver*/
  _ux_dcd_stm32_initialize((ULONG)USB_OTG_FS, (ULONG)&hpcd_USB_OTG_FS);
}
