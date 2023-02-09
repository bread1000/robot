/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
//#include <rc5_decode.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUFFER_LEN   1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint32_t battery_adc = 0;
static int drive = 10;
static bool DOCKING = false;
static uint8_t RX_BUFFER[BUFFER_LEN] = {0};
static int speed = 0;
static float battery_voltage = 0;
static bool rec_left = false;
static bool rec_right = false;
static bool spin_right = false;
static bool spin_left = false;
static bool center = false;
static bool right = false;
static bool left = false;
static bool on_left = false;
static bool on_right = false;
static uint32_t distance_left = 0;
static uint32_t distance_right = 0;
static bool STATION = false;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//Funkcja umożliwiająca wysyłanie znaków przez uart za pomocą funkcji 'printf()'
int __io_putchar(int ch)
{
    if (ch == '\n')
    {
        uint8_t ch2 = '\r';
        HAL_UART_Transmit(&huart2, &ch2, 1, HAL_MAX_DELAY);
    }

    HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return 1;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	//przerwanie od TIM 10 - co 5 sekund
	if(htim == &htim10)
	{
		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15); //sygnalizacja nadawania
		printf("%lu\n", battery_adc);			//przesłanie stacji informacji o zmierzonym napięciu baterii
	}
}

//Funkcja wysyłająca wymuszająca w Arduino zwiększenie prędkości obrotu silników
void speed_plus(int i)
{
	int k = 0;
	for (k == 0; k++; k < i)
	{
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
		HAL_Delay(50);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);
		HAL_Delay(50);
	}
}

//Funkcja wysyłająca wymuszająca w Arduino zmniejszenie prędkości obrotu silników
void speed_minus(int i)
{
	int k = 0;
	for (k == 0; k++; k < i)
	{
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);
		HAL_Delay(50);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
		HAL_Delay(50);
	}
}

//Funkcja zatrzymująca silniki - zatrzymanie robota
void STOP(void)
{
	speed = 0;
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
}

//Przerwanie UART - odebranie danych
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	drive = atoi((char*)&RX_BUFFER[0]); 					//zapisanie odebranego znaku 'char' do zmiennej 'int'
	if (drive == 0)
		STOP();												//jeśli odebrano wartość '0' - bezwarunkowe zatrzymanie robota

	HAL_UART_Receive_IT(&huart2, RX_BUFFER, BUFFER_LEN);	//wlaczenie nasluchiwania na kanale UART
}

//Zestaw funkcji nadających kierunek i prędkość silnikom
void FORWARD(void){
	HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, speed);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);	//1 przód
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);	//2 przód
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);	//3 przód
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);	//4 przód
}
void LEFT(void){
	HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, speed);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);		//1 przód
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);	//2 tył
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);		//3 przód
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);	//4 tył
}
void RIGHT(void){
	HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, speed);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);	//1 tył
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);		//2 przód
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);	//3 tył
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);		//4 przód
}
void SPIN_R(void){
	HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, speed);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);		//1 przód
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);	//2 tył
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);	//3 tył
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);		//4 przód
}
void SPIN_L(void){
	HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, speed);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);	//1 tył
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);		//2 przód
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);		//3 przód
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);	//4 tył
}
void BACK(void){
	//HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, speed);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);	//1 tył
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);	//2 tył
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);	//3 tył
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);	//4 tył
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM10_Init();
  MX_USART2_UART_Init();
  MX_ADC3_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

  HAL_ADC_Start_DMA(&hadc3, &battery_adc, 1);	//uruchomienie konwersji ADC z DMA

  HAL_TIM_Base_Start_IT(&htim10);				//wlaczenie przerwan od timera 10 - dioda określająca działanie programu
  HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_1); 		//timer 2 - obsługuje lewy czujnik odległości HC-SR04
  HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);

  HAL_TIM_IC_Start(&htim3, TIM_CHANNEL_1);		//timer 3 - obsługuje prawy czujnik odległości HC-SR04
  HAL_TIM_IC_Start(&htim3, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

  HAL_UART_Receive_IT(&huart2, RX_BUFFER, BUFFER_LEN);	//wlaczenie nasluchiwania na kanale UART
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint8_t start1 = 0;
  uint8_t start2 = 0;
  uint8_t stop1 = 0;
  uint8_t stop2 = 0;

  while (1)
  {
	  /*--------------CZUJNIKI ODLEGLOSCI----------------*/
	  start1 = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_1);	//wystąpienie zbocza narastającego sygnału 'echo'
	  stop1 = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_2);		//wystąpienie zbocza opadającego sygnału 'echo'
	  distance_left = (stop1 - start1) / 58.0f;						//czas wystąpienia sygnału 'echo'

	  start2 = HAL_TIM_ReadCapturedValue(&htim3, TIM_CHANNEL_1);
	  stop2 = HAL_TIM_ReadCapturedValue(&htim3, TIM_CHANNEL_2);
	  distance_right = (stop2 - start2) / 58.0f;

	  /* sygnał przychodzący od odbiorników -------------------------------------------IR */
	  if (HAL_GPIO_ReadPin(rec_left_GPIO_Port, rec_left_Pin) == GPIO_PIN_RESET)
		  rec_left = true;
	  else
		  rec_left = false;

	  if (HAL_GPIO_ReadPin(rec_right_GPIO_Port, rec_right_Pin) == GPIO_PIN_RESET)
		  rec_right = true;
	  else
		  rec_right = false;

	  /*-----SWITCH DLA DANYCH ZE STACJI-------*/
	  switch(drive)
	  {
	  	  case 0:
	  		  STOP();				//zatrzymanie robota
	  		  drive = 10;
	  		  break;
	  	  case 1:
	  		  STATION = true;		//czujnik odległości stacji wykrył stację przed stykami
	  		  drive = 10;
	  		  break;
	  	  case 2:
			  STATION = false;		//czujnik odległości stacji wykrył oddalenie się robota od styków
			  drive = 10;
			  break;
	  	  case 3:
	  		  STOP(); //----------zmienic docking na docked
	  		  if (DOCKING == true)	//stacja wykryła napięcie na stykach - robot jest zadokowany do stacji
	  			  DOCKING = false;
	  		  drive = 10;
			  break;
	  	  case 4:
  		  	  STOP();
  		  	  if (DOCKING == false)	//stacja przestała wykrywać napięcie na stykach - robot odłączył się od stacji
	  	  	  {
  		  		  DOCKING = true;
  		  		  //reset pomocniczych zmiennych procesu dokowania
				  spin_right = false;
				  spin_left = false;
				  left = false;
				  center = false;
				  on_right = false;
			  }
  		  	  drive = 10;
			  break;
	  	case 5:
			  STOP();
			  if (DOCKING == false)	//stacja wymusza proces dokowania
				  DOCKING = true;
			  drive = 10;
			  break;
		  default:
			  break;
	  	  }

	  if (DOCKING == true)
	  {
		  if (rec_left == true && rec_right == true && distance_left < 12 && distance_right > distance_left && STATION == false)
		  {
			  on_left = true;
			  on_right = false;
			  left = false;
			  right = false;
			  spin_left = false;
			  spin_right = false;
			  center = false;
		  }
		  else if (rec_left == true && rec_right == true && distance_right < 12 && distance_left > distance_right && STATION == false)
		  {
			  on_left = false;
			  on_right = true;
			  left = false;
			  right = false;
			  spin_left = false;
			  spin_right = false;
			  center = false;
		  }

		  if (on_left == false && on_right == false)
		  {
			  //lewy TAK; prawy NIE + jest przy samej stacji + na wprost
			  if (rec_left == true && rec_right == false && STATION == true && left == false && distance_left < 12 && distance_right < 12)
			  {
				  left = true;
				  right = false;
				  spin_left = false;
				  spin_right = false;
				  center = false;
			  }
			  //lewy NIE; prawy TAK + jest przy samej stacji + na wprost
			  else if (rec_left == false && rec_right == true && STATION == true && right == false && distance_left < 12 && distance_right < 12)
			  {
				  left = false;
				  right = true;
				  spin_left = false;
				  spin_right = false;
				  center = false;
			  }
			  //lewy NIE; prawy NIE
			  else if (	(rec_left == false && spin_right == false && STATION == false) ||
					//lewy NIE; prawy TAK
					(rec_left == false && rec_right == true && spin_right == false && STATION == false) ||
					//lewy NIE; prawy TAK + jest przy samej stacji + nie na wprost
					(rec_left == false && rec_right == true && STATION == true && spin_right == false && distance_left > distance_right && distance_right < 12)||
					//lewy TAK; prawy NIE + jest przy samej stacji + nie na wprost
					(rec_left == true && rec_right == false && STATION == true && spin_right == false && distance_left > distance_right && distance_right < 12))
			  {
				  left = false;
				  right = false;
				  spin_left = false;
				  spin_right = true;
				  center = false;
			  }
			  //lewy TAK; prawy NIE
			  else if ( (rec_left == true && rec_right == false && spin_left == false && STATION == false) ||
					  //lewy TAK; prawy NIE + jest przy samej stacji + nie na wprost
					  (rec_left == true && rec_right == false && STATION == true && spin_left == false && distance_left < 12 && distance_right > distance_left)||
					  //lewy NIE; prawy TAK + jest przy samej stacji + nie na wprost
					  (rec_left == false && rec_right == true && STATION == true && spin_left == false && distance_left < 12 && distance_right > distance_left))
			  {
				  left = false;
				  right = false;
				  spin_left = true;
				  spin_right = false;
				  center = false;
			  }
			  //lewy TAK; prawy TAK
			  else if (rec_left == true && rec_right == true && center == false)
			  {
				  left = false;
				  right = false;
				  spin_left = false;
				  spin_right = false;
				  center = true;
			  }
		  }
		  else if(on_left == true)
		  {
			  if(	(rec_left == true && rec_right == true && spin_left == false && right == false) ||
					  	(rec_left == false && rec_right == false) )
			  {
				  right = false;
				  spin_left = true;
			  }
			  else if(rec_left == false && rec_right == true)
			  {
				  right = true;
				  spin_left = false;
			  }
			  else if(rec_left == true && rec_right == false && right == false)
			  {
				  //right = false;
				  spin_left = false;
				  on_left = false;
			  }
		  }
		  else if (on_right == true)
		  {
			  if(	(rec_left == true && rec_right == true && spin_right == false && left == false) ||
						(rec_left == false && rec_right == false) )
			  {
				  left = false;
				  spin_right = true;
			  }
			  else if(rec_left == true && rec_right == false)
			  {
				  left = true;
				  spin_right = false;
			  }
			  else if(rec_left == false && rec_right == true && left == false)
			  {
				  //left = false;
				  spin_right = false;
				  on_right = false;
			  }
		  }

		  if (center == true && distance_left < 12 && distance_right < 12 && on_left == false && on_right == false && STATION == true)
		  {
			  STOP();
		  }
		  else if (spin_left == true)
		  {
			  if (speed != 40)
			  {
				  STOP();
				  SPIN_L();
				  //zwiekszenie predkosci x4
				  speed = 40;
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);

				  HAL_Delay(50);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);

				  HAL_Delay(50);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);

				  HAL_Delay(50);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);
			  }
		  }
		  else if (spin_right == true)
	  	  {
			  if (speed != 40)
			  {
				  STOP();
				  SPIN_R();
				  //zwiekszenie predkosci x4
				  speed = 40;
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);

				  HAL_Delay(50);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);

				  HAL_Delay(50);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);

				  HAL_Delay(50);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);
			  }
	  	  }
		  else if (left == true)
		  {
			  if (speed != 40)
			  {
				  STOP();
				  LEFT();
				  //zwiekszenie predkosci x4
				  speed = 40;
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);

				  HAL_Delay(50);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);

				  HAL_Delay(50);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);

				  HAL_Delay(50);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);
			  }
		  }
		  else if (right == true)
		  {
			  if (speed != 40)
			  {
				  STOP();
				  RIGHT();
				  //zwiekszenie predkosci x4
				  speed = 40;
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);

				  HAL_Delay(50);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);

				  HAL_Delay(50);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);

				  HAL_Delay(50);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);
			  }
		  }
	  	  else if (center == true)
		  {
			  if (speed != 40)
			  {
				  STOP();
				  FORWARD();
				  //zwiekszenie predkosci x3
				  speed = 40;
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);

				  HAL_Delay(50);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);

				  HAL_Delay(50);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);

				  HAL_Delay(50);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
				  HAL_Delay(100);
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);
			  }
		  }

  	  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /**
    * @brief  Identify TIM clock
    * @param  None
    * @retval Timer clock
    */


  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
