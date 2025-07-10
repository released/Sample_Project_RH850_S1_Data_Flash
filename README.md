# Sample_Project_RH850_S1_Data_Flash
Sample_Project_RH850_S1_Data_Flash

update @ 2025/07/10

1. base on EVM : Y-BLDC-SK-RH850F1KM-S1-V2 , initial below function

- TAUJ0_0 : timer interval for 1ms interrupt

- UART : RLIN3 (TX > P10_10 , RX > P10_9) , for printf and receive from keyboard

- LED : LED18 > P0_14 , LED17 > P8_5 , toggle per 1000ms

- initial data flash

	- RH850 data flash library 
	
	https://www.renesas.com/en/document/lbr/rh850-data-flash-libraries?r=1170176
	

	- RH850 data flash application note

	https://www.renesas.com/en/document/apn/rh850-data-flash-libraries?language=en&r=1170176

- need to add section as below , 

![image](https://github.com/released/Sample_Project_RH850_S1_Data_Flash/blob/main/section2.jpg)

![image](https://github.com/released/Sample_Project_RH850_S1_Data_Flash/blob/main/section.jpg)
 
2. below is log message :

press 1 : to erase data flash

press 3 : write buffer to data flash , modify value with counter 

press 4 : write const data to data flash 

press 2 : to read data flash

![image](https://github.com/released/Sample_Project_RH850_S1_Data_Flash/blob/main/digit_1.jpg)

