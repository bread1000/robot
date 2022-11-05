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
#include "dac.h"
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
#include <rc5_decode.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

extern RC5_Frame_t RC5_FRAME;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

uint32_t battery_adc = 0;
static int drive = 10;
static uint8_t received = 10;
static int speed = 0;
static float battery_voltage = 0;

extern volatile enum StatusYesOrNo RC5FrameReceived;
extern const uint8_t* aRC5Devices[32];
extern const uint8_t* aRC5Commands[128];

uint32_t ICValue1 = 0;
uint32_t ICValue2 = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int __io_putchar(int ch)
{
    if (ch == '\n') {
        uint8_t ch2 = '\r';
        HAL_UART_Transmit(&huart2, &ch2, 1, HAL_MAX_DELAY);
    }

    HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return 1;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	/*przerwanie od TIM 10 - co 5 sekund wysyla wiadomosc o czasie dzialania programu*/
	if(htim == &htim10)
	{
		static uint16_t cnt = 0; //licznik sekund
		cnt+=5;

		printf("Czas pracy [s]: %d\n", cnt);
		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15); //sygnalizacja nadawania
	}

	/*przerwanie od od TIM 2 - dekodowanie IR*/
	if (htim == &htim2)
	{
		printf("TIM2 przerwanie\n");
		RC5_ResetPacket();
	}
}

/*przerwanie od od TIM 2 - dekodowanie IR*/
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	/* - Timer Falling Edge Event:
	    The Timer interrupt is used to measure the period between two
	    successive falling edges (The whole pulse duration).

	    - Timer Rising Edge Event:
	    It is also used to measure the duration between falling and rising
	    edges (The low pulse duration).
	    The two durations are useful to determine the bit value. Each bit is
	    determined according to the last bit.

	    Update event:InfraRed decoders time out event.
	    ---------------------------------------------
	    It resets the InfraRed decoders packet.
	    - The Timer Overflow is set to 3.6 ms .*/

	    /* IC2 Interrupt*/
	    if (HAL_TIM_GetActiveChannel(&htim2) == HAL_TIM_ACTIVE_CHANNEL_1)
	    {
	    	printf("CH 1 przerwanie\n");
	      ICValue2 = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_1);
	      /* RC5 */
	      RC5_DataSampling(ICValue2 - ICValue1 , 0);

	    }  /* IC1 Interrupt */
	    else if (HAL_TIM_GetActiveChannel(&htim2) == HAL_TIM_ACTIVE_CHANNEL_2)
	    {
	    	printf("CH 2 przerwanie\n");
	      ICValue1 =  HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_2);
	      RC5_DataSampling(ICValue1 , 1);
	    }
}

void STOP(void)
{
	speed = 0;
	HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, speed);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) //przerwanie od receivera uart
{
	/*podejmwoanie dzialan na podstawie odebranych danych
	oraz wpisywanie informacji zwrotnej do bufora*/
	switch( atoi((char*)&received) )
	{
	case 0:
		drive=0;
		STOP();
		printf("Odebrano: 0\n");
		break;
	case 1:
		drive = 1;
		speed = speed + 10;
		//HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, speed);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
		HAL_Delay(100);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);
		printf("Odebrano: 1\n");
		break;
	case 2:
		//STOP();
		drive = 2;
		printf("Odebrano: 2\n");
		break;
	case 3:
		speed = speed - 10;
		//HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, speed);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_10, GPIO_PIN_RESET);
		HAL_Delay(100);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);
		drive = 3;
		printf("Odebrano: 3\n");
		break;
	case 4:
		//STOP();
		drive = 4;
		printf("Odebrano: 4\n");
		break;
	case 5:
		drive = 5;
		printf("Odebrano: 5\n");
		break;
	case 6:
		//STOP();
		drive = 6;
		printf("Odebrano: 6\n");
		break;
	case 7:
		//STOP();
		drive = 7;
		printf("Odebrano: 7\n");
		break;
	case 8:
		//STOP();
		drive = 8;
		printf("Odebrano: 8\n");
		break;
	case 9:
		//STOP();
		drive = 9;
		printf("Odebrano: 9\n");
		break;
	default:
		break;
	}

	HAL_UART_Receive_IT(&huart2, &received, 1); //wlaczenie nasluchiwania na kanale UART
}


void PRZOD(void){
	HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, speed);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);	//1 przód
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);	//2 przód
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);	//3 przód
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);	//4 przód
}

void LEWA(void){
	HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, speed);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);		//1 przód
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);	//2 tył
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);		//3 przód
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);	//4 tył
}

void PRAWA(void){
	HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, speed);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);	//1 tył
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);		//2 przód
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);	//3 tył
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);		//4 przód
}

void OBROT_P(void){
	HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, speed);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);		//1 przód
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);	//2 tył
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);	//3 tył
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);		//4 przód
}

void OBROT_L(void){
	HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, speed);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);	//1 tył
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);		//2 przód
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);		//3 przód
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);	//4 tył
}

void TYL(void){
	//HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, speed);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);	//1 tył
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);	//2 tył
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);	//3 tył
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);	//4 tył
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

  //uint32_t DAC_OUT[100];

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
  MX_DAC_Init();
  MX_ADC3_Init();
  //MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  RC5_Init();

  HAL_ADC_Start_DMA(&hadc3, &battery_adc, 1); //uruchomienie konwersji ADC z DMA

  HAL_TIM_Base_Start_IT(&htim10); //wlaczenie przerwan od timera 10
  HAL_TIM_Base_Start_IT(&htim2);
  HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
  HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_2);

  HAL_UART_Receive_IT(&huart2, &received, 1); //wlaczenie nasluchiwania na kanale UART

  //aktywacja Core2530 do pracy
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);
  HAL_Delay(200);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  /*-----------Odczyt stanu bterii---------------*/
	  battery_voltage = 3.3f * battery_adc / (4096.0f-1);	//przeliczenie wartości napięcia zasilania
	  //printf("ADC = %lu (%.3f V)\n", battery_adc, battery_voltage);
	  //HAL_Delay(500);

	  /* sygnał przychodzący od odbiorników -------------------------------------------IR */
	  /*-------sprawdzenie czy sygnal przychodzi-------*/
/*	  if (HAL_GPIO_ReadPin(rec_left_GPIO_Port, rec_left_Pin) == GPIO_PIN_RESET)
	  {
		  printf("lewy\n");
	  }
	  if (HAL_GPIO_ReadPin(rec_right_GPIO_Port, rec_right_Pin) == GPIO_PIN_RESET)
	  {
		  printf("prawy\n");
	  }
*/
	  /*-------RC5-------*/
	  /* if data do not received */
	  if(0x00 == RC5FrameReceived)
	  {
		  //printf("Ni ma\n");
	  }else{
		  /* decode the rc5 frame */
		  RC5_Decode(&RC5_FRAME);
	  }

	  /* w switchu nastepuje wykonywanie instrukcji wedlug odebranych z komputera danych
	  wartosc zmiennej drive jest nadawana w przerwaniu UART po odebraniu komunikatu*/
	  switch(drive)
	  {
	  case 0:
		  STOP();
		  //nadawanie informacji przez UART
		  printf("STOP\n");
		  drive = 10;
		  break;
	  case 1:
		  //Zmiana wartości wypełnienia (+10)
		  printf("Wypelnienie: %d\n", speed);
		  drive = 10;
		  break;
	  case 2:
		  PRZOD(); /*funkcja nadajaca keirunek jazdy robota do przodu*/
		  printf("Jazda do przodu\n");
		  drive = 10;
		  break;
	  case 3:
		  //Zmiana wartości wypełnienia (-10)
		  printf("Wypelnienie: %d\n", speed);
		  drive = 10;
		  break;
	  case 4:
		  LEWA(); /*funkcja nadajaca keirunek jazdy robota w lewo*/
		  printf("Jazda w lewo\n");
		  drive = 10;
		  break;
	  case 5:
		  //speed=50; /*ustawienie wypelnienia na wartosc poczatkowa (50)*/
		  //printf("Wypelnienie: 50\n");
		  printf("NIC\n");
		  drive = 10;
		  break;
	  case 6:
		  PRAWA(); /*funkcja nadajaca keirunek jazdy robota w prawo*/
		  printf("Jazda w prawo\n");
		  drive = 10;
		  break;
	  case 7:
		  OBROT_L(); /*funkcja powodujaca rotacje robota w lewo*/
		  printf("Obrot w lewo\n");
		  drive = 10;
		  break;
	  case 8:
		  TYL(); /*funkcja nadajaca keirunek jazdy robota do tylu*/
		  printf("Jazda do tylu\n");
		  drive = 10;
		  break;
	  case 9:
		  OBROT_P(); /*funkcja powodujaca rotacje robota w prawo*/
		  printf("Obrot w prawo\n");
		  drive = 10;
		  break;
	  default:
		  break;
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
