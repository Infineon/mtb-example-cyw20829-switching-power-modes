/*******************************************************************************
* File Name:   main.c

* Description: This code example demonstrates how to manage the AIROC&trade;
* CYW20829 power system and modes. CYW20829 has following power configurations.
*
* - Power System - Low Power System/Ultra Low Power System
* - Power modes - Active/Sleep/DeepSleep/DeepSleep-RAM/Hibernate
*
* Related Document: See README.md
*
*
********************************************************************************
* Copyright 2022-2023, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "app_dsram.h"


/*******************************************************************************
* Macros
*******************************************************************************/
#define GPIO_INTERRUPT_PRIORITY   (2UL)


/*******************************************************************************
* Global Variables
*******************************************************************************/
/* System power modes */
static volatile uint8_t power_modes = 0;

/* System power state */
static bool syspm_state = false;

/* User Button Callbacks */
static cyhal_gpio_callback_data_t btn1_cb;
static cyhal_gpio_callback_data_t btn2_cb;

/* Enum For changing the Power modes */
enum PowerModes{
    SYSPM_NORMAL = 0U,   /* Active mode */
    SYSPM_SLEEP,         /* Sleep mode */
    SYSPM_DEEPSLEEP,     /* DeepSleep mode */
    SYSPM_DEEPSLEEP_RAM, /* DeepSleep-RAM mode */
    SYSPM_HIBERNATE,     /* Hibernate mode */
};


/*******************************************************************************
* Function Prototypes
*******************************************************************************/
static inline void handle_error(uint32_t status);
static void switch_power_mode(void);
static void gpio1_interrupt_handler(void *handler_arg,cyhal_gpio_event_t event);
static void gpio2_interrupt_handler(void *handler_arg,cyhal_gpio_event_t event);


/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
*  The main function initializes the board, retarget-io, LED and GPIO
*
* Parameters:
*  none
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;
    uint8_t hibernate_boot = (CY_SYSLIB_RESET_HIB_WAKEUP &
                                Cy_SysLib_GetResetReason()) ? 1 : 0;

    /* Check if device reboots due to DSRAM */
    if(!warm_boot)
    {
        /* Initialize the device and board peripherals */
        result = cybsp_init();

        /* Board initialization failed. Stop program execution */
        handle_error(result);
    }
    else
    {
        /* Free the required peripherals after waking-up from DS-RAM */
        cy_retarget_io_deinit();
        cyhal_gpio_free(CYBSP_USER_LED);
        cyhal_gpio_free(CYBSP_USER_BTN2);
        cyhal_gpio_free(CYBSP_USER_BTN);
    }

    /* Initialize the User LED */
    result = cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT,
                             CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);
    /* GPIO initialization failed. Stop program execution */
    handle_error(result);

    /* Initialize the User button 1 */
    result = cyhal_gpio_init(CYBSP_USER_BTN, CYHAL_GPIO_DIR_INPUT,
                CYBSP_USER_BTN_DRIVE, CYBSP_BTN_OFF);
    /* GPIO initialization failed. Stop program execution */
    handle_error(result);

    /* Initialize the User button 2 */
    result = cyhal_gpio_init(CYBSP_USER_BTN2, CYHAL_GPIO_DIR_INPUT,
                CYBSP_USER_BTN_DRIVE, CYBSP_BTN_OFF);
    /* GPIO initialization failed. Stop program execution */
    handle_error(result);

    /* Configure Button interrupts */
    btn1_cb.callback = &gpio1_interrupt_handler;
    cyhal_gpio_register_callback(CYBSP_USER_BTN, &btn1_cb);
    cyhal_gpio_enable_event(CYBSP_USER_BTN, CYHAL_GPIO_IRQ_FALL,
                            GPIO_INTERRUPT_PRIORITY, true);

    btn2_cb.callback = &gpio2_interrupt_handler;
    cyhal_gpio_register_callback(CYBSP_USER_BTN2, &btn2_cb);
    cyhal_gpio_enable_event(CYBSP_USER_BTN2, CYHAL_GPIO_IRQ_FALL,
                            GPIO_INTERRUPT_PRIORITY, true);

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                                 CY_RETARGET_IO_BAUDRATE);
    /* retarget-io initialization failed. Stop program execution */
    handle_error(result);

    /* Turn ON User LED after waking-up from DS-RAM */
    if(warm_boot)
    {
        warm_boot = 0;
        cyhal_gpio_write(CYBSP_USER_LED,CYBSP_LED_STATE_ON);

        printf("\n Warmboot(Wake from DS-RAM) : Running "
                "CYW20829 : Switching Power Modes Application\r\n");

        printf("\n Entering to Hibernate press User Button 1 to come out "
                "the system from Hibernate mode\r\n");
    }
    /* Do the following routine after coming out from Hibernate */
    else if(hibernate_boot)
    {
        if(Cy_SysPm_GetIoFreezeStatus())
        {
            /* Unfreeze the system */
            Cy_SysPm_IoUnfreeze();
        }
        printf("\n Coldboot(Wake from Hibernate) : Running"
                " CYW20829 : Switching Power Modes Application\r\n");
    }
    else
    {
        /* Print a message on UART */
        /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
        printf("\x1b[2J\x1b[;H");
        printf("**********CYW20829 : Switching Power Modes*************\r\n\n");

        printf("=========================================================\r\n");
        printf("| Press the User Button:                                |\r\n");
        printf("|   Press User Button 1 to Change Power mode            |\r\n");
        printf("|   Press User Button 2 to change power system          |\r\n");
        printf("=========================================================\r\n");
    }

    /* DS-RAM Initialization */
    result = cybsp_syspm_dsram_init();

    /* Stop the Execution if DS-RAM Initialization failed */
    handle_error(result);

    /* Register the DS-RAM Call Back */
    cy_app_register_syspm_dsram_callback();

    cy_app_register_syspm_ds_callback();

    for (;;)
    {
        switch_power_mode();
    }
}


/*******************************************************************************
* Function Name: gpio1_interrupt_handler
********************************************************************************
* Summary:
*  GPIO interrupt handler will change the power modes from
*  Active to Sleep
*  Active to DeepSleep
*  Active to DeepSleep-RAM
*  Active to Hibernate
*
* Parameters:
*  void *handler_arg
*  cyhal_gpio_irq_event_t
*
* Return:
*  static void
*
*******************************************************************************/
static void gpio1_interrupt_handler(void *handler_arg, cyhal_gpio_event_t event)
{
    (void)handler_arg;
    (void)event;
    power_modes++;
}


/*******************************************************************************
* Function Name: gpio2_interrupt_handler
********************************************************************************
* Summary:
*  GPIO interrupt handler will change the power system from Low Power system to
*  Ultra Low Power system and vise versa
*
* Parameters:
*  void *handler_arg
*  cyhal_gpio_irq_event_t
*
* Return:
*  static void
*
*******************************************************************************/
static void gpio2_interrupt_handler(void *handler_arg, cyhal_gpio_event_t event)
{
    (void)handler_arg;
    (void)event;
    syspm_state = true;
    power_modes = SYSPM_NORMAL;
}


/*******************************************************************************
* Function Name: handle_error
********************************************************************************
* Summary:
*  User defined error handling function
*
* Parameters:
*  uint32_t status - status indicates success or failure
*
* Return:
*  void
*
*******************************************************************************/
static inline void handle_error(uint32_t status)
{
    if (status != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }
}


/*******************************************************************************
* Function Name: switch_power_mode
********************************************************************************
* Summary:
*  To enter device to Low-Power and Ultra Low-Power system, and transition
*  from Active,Sleep,DeepSleep,DeepSleep-RAM and Hibernate power modes
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void switch_power_mode(void)
{
    cy_rslt_t result;

    if(syspm_state == true)
    {
        /* Check if the device is in System Ultra Low Power state */
        if (cyhal_syspm_get_system_state() == CYHAL_SYSPM_SYSTEM_LOW)
        {
            /* Switch to System Low Power state */
            cyhal_syspm_set_system_state(CYHAL_SYSPM_SYSTEM_NORMAL);
        }
        else
        {
            /* Switch to System Ultra Low Power state */
            cyhal_syspm_set_system_state(CYHAL_SYSPM_SYSTEM_LOW);
        }

        printf("\n Switching the System Power system : %s \r\n",
                cyhal_syspm_get_system_state() ? "ULP": "LP");
        syspm_state=false;

    }
    /* Switching the power modes */
    switch(power_modes)
    {
        case SYSPM_SLEEP:
            printf("\n Going to Sleep : Running"
                    " CYW20829 : Switching Power Modes Application\r\n");
            result = cyhal_syspm_sleep();
            if(result == CY_SYSPM_SUCCESS)
            {
                printf("\n Wake from Sleep : Running"
                        " CYW20829 : Switching Power Modes Application\r\n");
            }
            break;
        case SYSPM_DEEPSLEEP:
            Cy_SysPm_SetDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP);
            result = cyhal_syspm_deepsleep();
            if(result == CY_SYSPM_SUCCESS)
            {
                printf("\n Wake from Deep Sleep : Running"
                        " CYW20829 : Switching Power Modes Application\r\n");
            }
            break;
        case SYSPM_DEEPSLEEP_RAM:
            Cy_SysPm_SetDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP_RAM);
            cyhal_syspm_deepsleep();
            break;
        case SYSPM_HIBERNATE:
            Cy_SysLib_Delay(1000/* msec */);
            cyhal_syspm_hibernate(CYHAL_SYSPM_HIBERNATE_PINA_LOW);
            break;
        default:
            break;
    }
}


/* [] END OF FILE */
