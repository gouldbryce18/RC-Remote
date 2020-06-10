/*
 * ADC.h
 *
 *  Created on: Apr 21, 2020
 *      Author: Bryce Gould
 */

#ifndef ADC_H_
#define ADC_H_


void ADCInit(void);

INT16U DrivePend(OS_TICK tout, OS_ERR *os_err_ptr);


void BattPend(OS_TICK tout, OS_ERR *os_err_ptr);

#endif /* ADC_H_ */
