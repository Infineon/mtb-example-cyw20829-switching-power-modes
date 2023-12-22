/*******************************************************************************
* File Name:   app_dsram.c

* Description: Provides initialization code for handling deepsleep ram.
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


/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Indicate to sysetm wake-up from DS-RAM */
uint8_t warm_boot = 0;

/* DS-RAM Warmboot Re-entry */
cy_stc_syspm_warmboot_entrypoint_t syspm_app_dsram_entry_point =
{(uint32_t *)&__INITIAL_SP, (uint32_t *)&cy_app_warmboot_handler};


/*******************************************************************************
* Function Prototypes
*******************************************************************************/
extern int main(void);


/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: cy_app_warmboot_handler
********************************************************************************
* Summary:
*  DeepSleep-RAM Warm boot entry function execute after wake-up DS-RAM
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
CY_SECTION_RAMFUNC_BEGIN
int  cy_app_warmboot_handler(void)
{
    warm_boot=1;
    SystemInit_Warmboot_CAT1B_CM33();

    cybsp_smif_enable();
    cybsp_smif_init();

    init_cycfg_all();

    System_Restore_NVIC_Reg();

    /* Suppress a compiler warning about unused return value */
    (void)Cy_SysPm_ExecuteCallback(((cy_en_syspm_callback_type_t)
                            CY_SYSPM_DEEPSLEEP_RAM), CY_SYSPM_AFTER_TRANSITION);

    /* Desired Resume point of the application */
    main();
    return 0;
}
CY_SECTION_RAMFUNC_END

/*******************************************************************************
* Function Name: cy_app_ds_callback
********************************************************************************
* Summary:
*  DeepSleep Callback Function
*
* Parameters:
*  callbackParams Pointer to cy_stc_syspm_callback_params_t
*  mode cy_en_syspm_callback_mode_t
*
* Return:
*  cy_en_syspm_status_t: CY_SYSPM_SUCCESS or CY_SYSPM_FAIL
*
*******************************************************************************/
cy_en_syspm_status_t cy_app_ds_callback(cy_stc_syspm_callback_params_t*
callbackParams,cy_en_syspm_callback_mode_t mode)
{
    cy_en_syspm_status_t retVal = CY_SYSPM_FAIL;

    CY_UNUSED_PARAMETER(callbackParams);

    switch (mode)
    {
        case CY_SYSPM_CHECK_READY:
        {
            retVal = CY_SYSPM_SUCCESS;
            break;
        }

        case CY_SYSPM_CHECK_FAIL:
        {
            retVal = CY_SYSPM_SUCCESS;
            break;
        }

        case CY_SYSPM_BEFORE_TRANSITION:
        {
            /* CTS and RTS pins are configured as analog hign-z as it is
             * connected to kitprog3 and also drawing more current */
            cyhal_gpio_configure(CYBSP_BT_UART_RTS,
                                CYHAL_GPIO_DIR_INPUT,
                                CYHAL_GPIO_DRIVE_ANALOG);
            cyhal_gpio_configure(CYBSP_BT_UART_CTS,
                                 CYHAL_GPIO_DIR_INPUT,
                                 CYHAL_GPIO_DRIVE_ANALOG);

            retVal = CY_SYSPM_SUCCESS;
        break;
        }


        case CY_SYSPM_AFTER_DS_WFI_TRANSITION:
        {
            retVal = CY_SYSPM_SUCCESS;
            break;
        }

        case CY_SYSPM_AFTER_TRANSITION:
        {
            /* CTS and RTS pins are configured to it's default configuration */
            cyhal_gpio_configure(CYBSP_BT_UART_RTS,
                                 CYHAL_GPIO_DIR_OUTPUT,
                                 CYHAL_GPIO_DRIVE_PULL_NONE);
            cyhal_gpio_configure(CYBSP_BT_UART_CTS,
                                 CYHAL_GPIO_DIR_OUTPUT,
                                 CYHAL_GPIO_DRIVE_PULL_NONE);
            retVal = CY_SYSPM_SUCCESS;
            break;
        }
        default:
        break;
    }
    return retVal;
}

/*******************************************************************************
* Function Name: cy_app_dsram_callback
********************************************************************************
* Summary:
*  DeepSleep-RAM Callback Function
*
* Parameters:
*  callbackParams Pointer to cy_stc_syspm_callback_params_t
*  mode cy_en_syspm_callback_mode_t
*
* Return:
*  cy_en_syspm_status_t: CY_SYSPM_SUCCESS or CY_SYSPM_FAIL
*
*******************************************************************************/
cy_en_syspm_status_t cy_app_dsram_callback(cy_stc_syspm_callback_params_t*
callbackParams,cy_en_syspm_callback_mode_t mode)
{
    cy_en_syspm_status_t retVal = CY_SYSPM_FAIL;

    CY_UNUSED_PARAMETER(callbackParams);

    switch (mode)
    {
        case CY_SYSPM_CHECK_READY:
        {
            retVal = CY_SYSPM_SUCCESS;
            break;
        }

        case CY_SYSPM_CHECK_FAIL:
        {
            retVal = CY_SYSPM_SUCCESS;
            break;
        }

        case CY_SYSPM_BEFORE_TRANSITION:
        {
            /* CTS and RTS pins are configured as analog hign-z as it is
             * connected to kitprog3 and also drawing more current */
            cyhal_gpio_configure(CYBSP_BT_UART_RTS, CYHAL_GPIO_DIR_INPUT,
                                                    CYHAL_GPIO_DRIVE_ANALOG);
            cyhal_gpio_configure(CYBSP_BT_UART_CTS, CYHAL_GPIO_DIR_INPUT,
                                                    CYHAL_GPIO_DRIVE_ANALOG);
            System_Store_NVIC_Reg();
            retVal = CY_SYSPM_SUCCESS;
        break;
        }


        case CY_SYSPM_AFTER_DS_WFI_TRANSITION:
        {
            retVal = CY_SYSPM_SUCCESS;
            break;
        }

        case CY_SYSPM_AFTER_TRANSITION:
        {
            /* CTS and RTS pins are configured to it's default configuration */
            cyhal_gpio_configure(CYBSP_BT_UART_RTS,
                                 CYHAL_GPIO_DIR_OUTPUT,
                                 CYHAL_GPIO_DRIVE_PULL_NONE);
            cyhal_gpio_configure(CYBSP_BT_UART_CTS,
                                 CYHAL_GPIO_DIR_OUTPUT,
                                 CYHAL_GPIO_DRIVE_PULL_NONE);
            retVal = CY_SYSPM_SUCCESS;
            break;
        }
        default:
        break;
    }
    return retVal;
}


/*******************************************************************************
* Function Name: cy_app_register_syspm_dsram_callback
********************************************************************************
* Summary:
*  Creates a syspm Callback for DS-RAM mode
*
* Parameters:
*  void
*
* Return:
*  True if callback was unregistered.
*  False if it was not unregistered or no callbacks are registered.
*
*******************************************************************************/
cy_rslt_t cy_app_register_syspm_dsram_callback(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    static cy_stc_syspm_callback_params_t cy_app_sysclk_pm_callback_param =
                                              { NULL, NULL };
    static cy_stc_syspm_callback_t cy_app_sysclk_pm_callback =
    {
        .callback = &cy_app_dsram_callback,
        .type = CY_SYSPM_DEEPSLEEP_RAM,
        .callbackParams = &cy_app_sysclk_pm_callback_param,
        .order = 0u
    };

    if (!Cy_SysPm_RegisterCallback(&cy_app_sysclk_pm_callback))
    {
        result = CYBSP_RSLT_ERR_SYSCLK_PM_CALLBACK;
    }
    return result;
}

/*******************************************************************************
* Function Name: cy_app_register_syspm_ds_callback
********************************************************************************
* Summary:
*  Creates a syspm Callback for DS mode
*
* Parameters:
*  void
*
* Return:
*  True if callback was unregistered.
*  False if it was not unregistered or no callbacks are registered.
*
*******************************************************************************/
cy_rslt_t cy_app_register_syspm_ds_callback(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    static cy_stc_syspm_callback_params_t cy_app_sysclk_pm_callback_param =
                                              { NULL, NULL };
    static cy_stc_syspm_callback_t cy_app_sysclk_pm_callback =
    {
        .callback = &cy_app_ds_callback,
        .type = CY_SYSPM_DEEPSLEEP,
        .callbackParams = &cy_app_sysclk_pm_callback_param,
        .order = 0u
    };

    if (!Cy_SysPm_RegisterCallback(&cy_app_sysclk_pm_callback))
    {
        result = CYBSP_RSLT_ERR_SYSCLK_PM_CALLBACK;
    }
    return result;
}

/*******************************************************************************
* Function Name: cybsp_syspm_dsram_init
********************************************************************************
* Summary:
*  Sets the Entry point to cy_app_warmboot_handler to execute after wake-up
*  from DS-RAM
*
* Parameters:
*  void
*
* Return:
*  cy_rslt_t CY_RSLT_SUCCESS
*
*******************************************************************************/
cy_rslt_t cybsp_syspm_dsram_init(void)
{

    /* Sets the warm boot entry point and disables the debug pins */
    Cy_Syslib_SetWarmBootEntryPoint((uint32_t*)&syspm_app_dsram_entry_point,
                                               false);

     return CY_RSLT_SUCCESS;
}
