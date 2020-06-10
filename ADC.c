/*
 * ADC.c
 *
 *  Created on: Apr 21, 2020
 *      Author: Bryce Gould
 */
#include <BasicIO.h>
#include <MCUType.h>
#include <uCOS/uC-CFG/app_cfg.h>
#include <uCOS/uCOS-III/os.h>
#include "K22FRDM_ClkCfg.h"
#include "K22FRDM_GPIO.h"
#include "ADC.h"
#include "UART.h"
#include "SeniorProject.h"

#define SET_FIRST_BIT 0x80
#define CLEAR_FIRST_BIT 0x7F
#define SET_SECOND_BIT 0x40
#define CLEAR_SECOND_BIT 0xDF
#define CLEAR_SECOND_THIRD_BIT 0x9F
#define SET_THIRD_BIT 0x20
#define CLEAR_THIRD_BIT 0xBF
#define SET_FOURTH_BIT 0x10
#define CLEAR_FOURTH_BIT 0xEF
#define OFFSET 1000
#define OFFSET_TWO 10000


static OS_TCB ADCTaskTCB;
static CPU_STK ADCTaskStk[APP_CFG_UART_TASK_STK_SIZE];
static OS_SEM BattFlag;

static INT16U speedinit;
static INT16U steerinit;

void ADCTask(void);

void ADCTask(void){
    OS_ERR os_err;
    INT16U speedsample;
    INT16U speed;
    INT16U steersample;
    INT16U batt;
    INT8U drivedata;
    INT16U max = 65535;
    INT16U half = max/2;
    INT16U quarter = half/2;

    while(1){
        OSTimeDly(150,OS_OPT_TIME_PERIODIC,&os_err);
        while(os_err != OS_ERR_NONE){           /* Error Trap                        */
        }

        //gets rid of previous data
        drivedata = 0;
        //////////////////////////////////////////////////////////////
        // Speed - vertical voltage divider/analog stick
        /////////////////////////////////////////////////////////////
        ADC0->SC1[0] = ADC_SC1_ADCH(6);
        while((ADC0->SC1[0] & ADC_SC1_COCO_MASK)==0){}
        speedsample = ADC0->R[0]; //Get result

        if(speedsample < (speedinit - OFFSET)){
            // 0 == forward
            // 1 == reverse
            drivedata = drivedata | SET_FIRST_BIT;
            speed = speedinit - speedsample;
            speed = (INT32U)(16*speed)/half;
        }else{
            if(speedsample > (speedinit + OFFSET)){
                drivedata = drivedata & CLEAR_FIRST_BIT;
                speed = speedsample - speedinit;
                speed = (INT32U)(16*speed)/half;
                //prevents roll over
                if(speed > 15){
                    speed = 0xf;
                }else{}
            }else{
                speed = 0;
            }
        }
        drivedata = drivedata | speed;
        /////////////////////////////////////////////////////////////
        // Steering - horizontal voltage divider/analog stick
        /////////////////////////////////////////////////////////////
        ADC0->SC1[0] = ADC_SC1_ADCH(7); //Start
        while((ADC0->SC1[0] & ADC_SC1_COCO_MASK)==0){}
        steersample = ADC0->R[0]; //Get result

        //Set turning direction
        if(steersample > (steerinit + OFFSET_TWO)){
            //left
            drivedata = drivedata | SET_THIRD_BIT;
            drivedata = drivedata & CLEAR_THIRD_BIT; // clear turn right bit
        }else{
            if(steersample < (steerinit - OFFSET_TWO)){
                //right
                drivedata = drivedata | SET_SECOND_BIT;
                drivedata = drivedata & CLEAR_SECOND_BIT; // clear turn left bit
            }else{
                drivedata = drivedata & CLEAR_SECOND_THIRD_BIT; //clear turn left and right bits
            }
        }
        //set turning strength
        if(steersample > (steerinit + quarter)){
            //4th bit to one
            drivedata = drivedata | SET_FOURTH_BIT;
        }else{
            if(steersample < (steerinit - quarter)){
                //set 4th bit to one
                drivedata = drivedata | SET_FOURTH_BIT;
            }else{
                //set 4th bit to zero
                drivedata = drivedata & CLEAR_FOURTH_BIT;
            }
        }

        ////////////////////////////////////////////////////////
        // drivedata is a INT8U with the first bit being forward or reverse, second bit left or right. 3rd and 4th bit the strength of the turn
        // and the last 4 bits the speed
        // drivedata = 0  00           0         0000
        //    direction^  ^left = 01   ^turn       ^drive strength
        //    forward=0    right= 10    strength
        //    reverse=1    strght=00
        //////////////////////////////////////////////////////
        Drive.tsample = drivedata;

        //Send data to UART
        OSSemPost(&Drive.flag, OS_OPT_POST_1, &os_err);
        ////////////////////////////////////////////////////////////
        // Battery monitoring
        ////////////////////////////////////////////////////////////

        ADC1->SC1[0] = ADC_SC1_ADCH(4);
        while((ADC1->SC1[0] & ADC_SC1_COCO_MASK)==0){}
        batt = ADC1->R[1]; //Get result

        if(batt < 40000){

            (void)OSSemPost(&BattFlag, OS_OPT_POST_1, &os_err);
        }else{
            GPIOB->PCOR = GPIO_PIN(0);
        }



   }
    while(os_err != OS_ERR_NONE){           /* Error Trap                        */
    }

}
void ADCInit(void){
    OS_ERR os_err;

    //Software triggered conversions
    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;
    SIM->SCGC6 |= SIM_SCGC6_ADC1_MASK;

    ADC0->CFG1 |= ADC_CFG1_ADIV(3)|ADC_CFG1_MODE(3)|ADC_CFG1_ADLSMP_MASK;
    ADC1->CFG1 |= ADC_CFG1_ADIV(3)|ADC_CFG1_MODE(3)|ADC_CFG1_ADLSMP_MASK;

    ADC0->CFG2 |= ADC_CFG2_MUXSEL(1); //Select ADC b


    //////////////////////////////////////////////////////
    // sets initialization for analog stick default value
    //////////////////////////////////////////////////////
    ADC0->SC1[0] = ADC_SC1_ADCH(6);
    while((ADC0->SC1[0] & ADC_SC1_COCO_MASK)==0){}
    speedinit = ADC0->R[0]; //Get result

    ADC0->SC1[0] = ADC_SC1_ADCH(7); //Start
    while((ADC0->SC1[0] & ADC_SC1_COCO_MASK)==0){}
    steerinit = ADC0->R[0]; //Get result

    OSTaskCreate((OS_TCB     *)&ADCTaskTCB,
                   (CPU_CHAR   *)"ADC Task ",
                   (OS_TASK_PTR ) ADCTask,
                   (void       *) 0,
                   (OS_PRIO     ) APP_CFG_ADC_TASK_PRIO,
                   (CPU_STK    *)&ADCTaskStk[0],
                   (CPU_STK     )(APP_CFG_ADC_TASK_STK_SIZE / 10u),
                   (CPU_STK_SIZE) APP_CFG_ADC_TASK_STK_SIZE,
                   (OS_MSG_QTY  ) 0,
                   (OS_TICK     ) 0,
                   (void       *) 0,
                   (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                   (OS_ERR     *)&os_err);

    OSSemCreate(&(Drive.flag),"DriveFlag",0,&os_err);

    OSSemCreate(&BattFlag,"BattFlag",0,&os_err);

    while(os_err != OS_ERR_NONE){           /* Error Trap                        */
    }

}
INT16U DrivePend(OS_TICK tout, OS_ERR *os_err_ptr){

    OSSemPend(&(Drive.flag), tout, OS_OPT_PEND_BLOCKING,(void *)0, os_err_ptr);
    return Drive.tsample;
}

void BattPend(OS_TICK tout, OS_ERR *p_err){
    OSSemPend(&BattFlag, tout, OS_OPT_PEND_BLOCKING,(void *)0, p_err);
}

