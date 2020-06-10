/*******************************************************************************************
* UART0Init() - Initialization of UART0, used for receiving GPS data
*
* Todd Morton, 10/09/2014
* Modified by Mike McFadden, 5/20/2019
*******************************************************************************************/

#include <uCOS/uC-CFG/app_cfg.h>
#include <uCOS/uCOS-III/os.h>
#include "MCUType.h"
#include "K22FRDM_ClkCfg.h"
#include "K22FRDM_GPIO.h"
#include "BasicIO.h"
#include "UART.h"
#include "ADC.h"

#define BIT_ONE 0x01
#define BIT_TWO 0x02
#define BIT_THREE 0x04


static OS_TCB RXTaskTCB;
static OS_TCB TXTaskTCB;
static CPU_STK RXTaskStk[APP_CFG_UART_TASK_STK_SIZE];
static CPU_STK TXTaskStk[APP_CFG_UART_TASK_STK_SIZE];

static OS_SEM RXFlag;

void RXTask(void);
void TXTask(void);

void RXPend(OS_TICK tout, OS_ERR *os_err_ptr);

void UARTInit(void){
    OS_ERR os_err;


    BIOOpen(BIO_BIT_RATE_9600);

    OSTaskCreate((OS_TCB     *)&RXTaskTCB,
                   (CPU_CHAR   *)"RX Task ",
                   (OS_TASK_PTR ) RXTask,
                   (void       *) 0,
                   (OS_PRIO     ) APP_CFG_RX_TASK_PRIO,
                   (CPU_STK    *)&RXTaskStk[0],
                   (CPU_STK     )(APP_CFG_ADC_TASK_STK_SIZE / 10u),
                   (CPU_STK_SIZE) APP_CFG_ADC_TASK_STK_SIZE,
                   (OS_MSG_QTY  ) 0,
                   (OS_TICK     ) 0,
                   (void       *) 0,
                   (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                   (OS_ERR     *)&os_err);
    while(os_err != OS_ERR_NONE){           /* Error Trap                        */
    }
    OSTaskCreate((OS_TCB     *)&TXTaskTCB,
                   (CPU_CHAR   *)"TX Task ",
                   (OS_TASK_PTR ) TXTask,
                   (void       *) 0,
                   (OS_PRIO     ) APP_CFG_TX_TASK_PRIO,
                   (CPU_STK    *)&TXTaskStk[0],
                   (CPU_STK     )(APP_CFG_ADC_TASK_STK_SIZE / 10u),
                   (CPU_STK_SIZE) APP_CFG_ADC_TASK_STK_SIZE,
                   (OS_MSG_QTY  ) 0,
                   (OS_TICK     ) 0,
                   (void       *) 0,
                   (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                   (OS_ERR     *)&os_err);

    OSSemCreate(&RXFlag,"ReceiveFlag",0,&os_err);

    while(os_err != OS_ERR_NONE){           /* Error Trap                        */
    }
}

void RXTask(void){
    OS_ERR os_err;

    INT8C drivecode;

    while(1){
        RXPend(0, &os_err);
        drivecode =BIOGetChar();

        ///////////////////////////////////
        // 8 bit code. each bit corresponds to different trigger
        //
        // first bit = low Motor battery
        // second bit = low mcu battery for car
        // third bit = motor temp to high
        //
        //////////////////////////////
        //Light up corresponding LEDS
        if((drivecode  & BIT_ONE) == BIT_ONE){
            GPIOB->PSOR = GPIO_PIN(1);
        }else{
            GPIOB->PCOR = GPIO_PIN(1);
        }
        if((drivecode  & BIT_TWO) == BIT_TWO){
            GPIOB->PSOR = GPIO_PIN(2);
        }else{
            GPIOB->PCOR = GPIO_PIN(2);
        }
        if((drivecode  & BIT_THREE) == BIT_THREE){
            GPIOB->PSOR = GPIO_PIN(3);
        }else{
            GPIOB->PCOR = GPIO_PIN(3);
        }



    }
    while(os_err != OS_ERR_NONE){           /* Error Trap                        */
    }
}
void TXTask(void){
    OS_ERR os_err;


    while(1){
        Drive.tsample = DrivePend(0, &os_err);
        //UART will not receive all zeros, this ensures receive knows when to stop
        if(Drive.tsample == 0){
            Drive.tsample = 1;
        }else{}
        BIOWrite(Drive.tsample);

        OSSemPost(&RXFlag, OS_OPT_POST_1, &os_err);
    }
    while(os_err != OS_ERR_NONE){           /* Error Trap                        */
    }
}


/*******************************************************************************************
* UART1Init() - Initialization of UART0, used for sending and receiving characters from RS-232
* (USB to serial UART interfaces)
*
* Todd Morton, 10/09/2014
* Modified by Shaun Walsh, 5/20/2019
*******************************************************************************************/
void RXPend(OS_TICK tout, OS_ERR *p_err){
    OSSemPend(&RXFlag, tout, OS_OPT_PEND_BLOCKING,(void *)0, p_err);
}

