/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>

#include "ring_buffer.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
ring_buffer_t ring_buffer_uart_rx;
uint8_t rx_buffer[16];
uint8_t rx_data;
ring_buffer_t ring_buffer_keypad;
uint8_t keypad_data[5];
uint8_t birth_year [5] = "2002";
uint16_t key_event = 0xFF;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int _write(int file, char *ptr, int len)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
  return len;
}
/**
  * @brief  Rx Transfer completed callback.
  * @param  huart UART handle.
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (ring_buffer_put(&ring_buffer_uart_rx, rx_data) == 0) {
		printf("Rx buffer is full\r\n");
	}

	HAL_UART_Receive_IT(&huart2, &rx_data, 1);
}

/**
  * @brief  EXTI line detection callback.
  * @param  GPIO_Pin Specifies the port pin connected
  *         to corresponding EXTI line.
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	key_event = GPIO_Pin;
}

#include <stdio.h>  // Se asume que se incluye la librería necesaria para printf

void validate_birth_year(void) {
    uint8_t temp_buffer[5];
    for (uint8_t idx = 0; idx < 5; idx++) {
        ring_buffer_get(&ring_buffer_keypad, &temp_buffer[idx]);
    }
    if (memcmp(birth_year, temp_buffer, 5) == 0) {
        printf("Success\r\n");
    } else {
        printf("Fail\r\n");  // Corregido de 'print' a 'printf'
    }

}
void ring_buffer_init(ring_buffer_t *ring_buffer, uint8_t *buffer, uint16_t capacity);

void process_key_press (uint8_t key_pressed){
	ring_buffer_put (&ring_buffer_keypad, key_pressed);
	if (key_pressed == "*"){
		ring_buffer_reset(&ring_buffer_keypad);
	}else if (ring_buffer_is_full(&ring_buffer_keypad)!=0){
		validate_birth_year();
	}
}
/**
 * @brief This functions initialize the functionality of the keypad
 */
void keypad_init(void)
{
	/* Set the rows high to be detected in the columns by rising interrupt */
	ROW_1_GPIO_Port->BSRR = ROW_1_Pin;
	ROW_2_GPIO_Port->BSRR = ROW_2_Pin;
	ROW_3_GPIO_Port->BSRR = ROW_3_Pin;
	ROW_4_GPIO_Port->BSRR = ROW_4_Pin;
}

/**
 * @brief  This function debounces and identify keypad events.
 * @param  column_to_evaluate: the column where the event happened.
 * @retval 0xFF -> invalid key. [0x00 - 0x0F] -> valid key.
 */
uint8_t keypad_handler(uint16_t column_to_evaluate)
{
	uint8_t key_pressed = 0xFF; // Value to return

	/*** Debounce the key press (remove noise in the key) ***/
#define KEY_DEBOUNCE_MS 300 /*!> Minimum time required for since last press */
	static uint32_t last_pressed_tick = 0;
	if (HAL_GetTick() <= (last_pressed_tick + KEY_DEBOUNCE_MS)) {
		// less than KEY_DEBOUNCE_MS since last press. Probably noise
		return key_pressed; // return 0xFF
	}
	last_pressed_tick = HAL_GetTick();

	/*** Check in which column the event happened ***/
	switch (column_to_evaluate) {
	case COLUMN_1_Pin:
		ROW_1_GPIO_Port->BSRR = ROW_1_Pin; // turn on row 1
		ROW_2_GPIO_Port->BRR = ROW_2_Pin;  // turn off row 2
		ROW_3_GPIO_Port->BRR = ROW_3_Pin;  // turn off row 3
		ROW_4_GPIO_Port->BRR = ROW_4_Pin;  // turn off row 4
		HAL_Delay(2); // wait for voltage to establish
		if (COLUMN_1_GPIO_Port->IDR & COLUMN_1_Pin) {
			key_pressed = 0x01; // if column 1 is still high -> column 1 + row 1 = key 1
			break;
		}

		ROW_1_GPIO_Port->BRR = ROW_1_Pin; 	// turn off row 1
		ROW_2_GPIO_Port->BSRR = ROW_2_Pin; 	// turn on row 2
		HAL_Delay(2); // wait for voltage to establish
		if (COLUMN_1_GPIO_Port->IDR & COLUMN_1_Pin) {
			key_pressed = 0x04; // if column 1 is still high -> column 1 + row 2 = key 4
			break;
		}

		ROW_2_GPIO_Port->BRR = ROW_2_Pin; 	// turn off row 2
		ROW_3_GPIO_Port->BSRR = ROW_3_Pin; 	// turn on row 3
		HAL_Delay(2); // wait for voltage to establish
		if (COLUMN_1_GPIO_Port->IDR & COLUMN_1_Pin) {
			key_pressed = 0x07; // if column 1 is still high -> column 1 + row 3 = key 7
			break;
		}

		ROW_3_GPIO_Port->BRR = ROW_3_Pin;	// turn off row 3
		ROW_4_GPIO_Port->BSRR = ROW_4_Pin; 	// turn on row 4
		HAL_Delay(2); // wait for voltage to establish
		if (COLUMN_1_GPIO_Port->IDR & COLUMN_1_Pin) {
			key_pressed = 0x0E; // if column 1 is still high -> column 1 + row 4 = key *
			break;
		}
	  break;

	case COLUMN_2_Pin:
		ROW_1_GPIO_Port->BSRR = ROW_1_Pin; // turn on row 1
		ROW_2_GPIO_Port->BRR = ROW_2_Pin;  // turn off row 2
		ROW_3_GPIO_Port->BRR = ROW_3_Pin;  // turn off row 3
		ROW_4_GPIO_Port->BRR = ROW_4_Pin;  // turn off row 4
		HAL_Delay(2); // wait for voltage to establish
		if (COLUMN_2_GPIO_Port->IDR & COLUMN_2_Pin) {
			key_pressed = 0x02; // if column 1 is still high -> column 1 + row 1 = key 1
			break;
		}

		ROW_1_GPIO_Port->BRR = ROW_1_Pin; 	// turn off row 1
		ROW_2_GPIO_Port->BSRR = ROW_2_Pin; 	// turn on row 2
		HAL_Delay(2); // wait for voltage to establish
		if (COLUMN_2_GPIO_Port->IDR & COLUMN_2_Pin) {
			key_pressed = 0x05; // if column 1 is still high -> column 1 + row 2 = key 4
			break;
		}

		ROW_2_GPIO_Port->BRR = ROW_2_Pin; 	// turn off row 2
		ROW_3_GPIO_Port->BSRR = ROW_3_Pin; 	// turn on row 3
		HAL_Delay(2); // wait for voltage to establish
		if (COLUMN_2_GPIO_Port->IDR & COLUMN_2_Pin) {
			key_pressed = 0x08; // if column 1 is still high -> column 1 + row 3 = key 7
			break;
		}

		ROW_3_GPIO_Port->BRR = ROW_3_Pin;	// turn off row 3
		ROW_4_GPIO_Port->BSRR = ROW_4_Pin; 	// turn on row 4
		HAL_Delay(2); // wait for voltage to establish
		if (COLUMN_2_GPIO_Port->IDR & COLUMN_2_Pin) {
			key_pressed = 0x00; // if column 1 is still high -> column 1 + row 4 = key *
			break;
		}
	  break;

	case COLUMN_3_Pin:
		ROW_1_GPIO_Port->BSRR = ROW_1_Pin; // turn on row 1
		ROW_2_GPIO_Port->BRR = ROW_2_Pin;  // turn off row 2
		ROW_3_GPIO_Port->BRR = ROW_3_Pin;  // turn off row 3
		ROW_4_GPIO_Port->BRR = ROW_4_Pin;  // turn off row 4
		HAL_Delay(2); // wait for voltage to establish
		if (COLUMN_3_GPIO_Port->IDR & COLUMN_3_Pin) {
			key_pressed = 0x03; // if column 1 is still high -> column 1 + row 1 = key 1
			break;
		}

		ROW_1_GPIO_Port->BRR = ROW_1_Pin; 	// turn off row 1
		ROW_2_GPIO_Port->BSRR = ROW_2_Pin; 	// turn on row 2
		HAL_Delay(2); // wait for voltage to establish
		if (COLUMN_3_GPIO_Port->IDR & COLUMN_3_Pin) {
			key_pressed = 0x06; // if column 1 is still high -> column 1 + row 2 = key 4
			break;
		}

		ROW_2_GPIO_Port->BRR = ROW_2_Pin; 	// turn off row 2
		ROW_3_GPIO_Port->BSRR = ROW_3_Pin; 	// turn on row 3
		HAL_Delay(2); // wait for voltage to establish
		if (COLUMN_3_GPIO_Port->IDR & COLUMN_3_Pin) {
			key_pressed = 0x09; // if column 1 is still high -> column 1 + row 3 = key 7
			break;
		}

		ROW_3_GPIO_Port->BRR = ROW_3_Pin;	// turn off row 3
		ROW_4_GPIO_Port->BSRR = ROW_4_Pin; 	// turn on row 4
		HAL_Delay(2); // wait for voltage to establish
		if (COLUMN_3_GPIO_Port->IDR & COLUMN_3_Pin) {
			key_pressed = 0x0F; // if column 1 is still high -> column 1 + row 4 = key *
			break;
		}
	  break;

	case COLUMN_4_Pin:
		ROW_1_GPIO_Port->BSRR = ROW_1_Pin; // turn on row 1
		ROW_2_GPIO_Port->BRR = ROW_2_Pin;  // turn off row 2
		ROW_3_GPIO_Port->BRR = ROW_3_Pin;  // turn off row 3
		ROW_4_GPIO_Port->BRR = ROW_4_Pin;  // turn off row 4
		HAL_Delay(2); // wait for voltage to establish
		if (COLUMN_4_GPIO_Port->IDR & COLUMN_4_Pin) {
			key_pressed = 0x0A; // if column 1 is still high -> column 1 + row 1 = key 1
			break;
		}

		ROW_1_GPIO_Port->BRR = ROW_1_Pin; 	// turn off row 1
		ROW_2_GPIO_Port->BSRR = ROW_2_Pin; 	// turn on row 2
		HAL_Delay(2); // wait for voltage to establish
		if (COLUMN_4_GPIO_Port->IDR & COLUMN_4_Pin) {
			key_pressed = 0x0B; // if column 1 is still high -> column 1 + row 2 = key 4
			break;
		}

		ROW_2_GPIO_Port->BRR = ROW_2_Pin; 	// turn off row 2
		ROW_3_GPIO_Port->BSRR = ROW_3_Pin; 	// turn on row 3
		HAL_Delay(2); // wait for voltage to establish
		if (COLUMN_4_GPIO_Port->IDR & COLUMN_4_Pin) {
			key_pressed = 0x0C; // if column 1 is still high -> column 1 + row 3 = key 7
			break;
		}

		ROW_3_GPIO_Port->BRR = ROW_3_Pin;	// turn off row 3
		ROW_4_GPIO_Port->BSRR = ROW_4_Pin; 	// turn on row 4
		HAL_Delay(2); // wait for voltage to establish
		if (COLUMN_4_GPIO_Port->IDR & COLUMN_4_Pin) {
			key_pressed = 0x0D; // if column 1 is still high -> column 1 + row 4 = key *
			break;
		}
	  break;
	/*!\ TODO: Implement other column cases here */

	default:
		/* This should not be reached */
		printf("Unknown column: %x\r\n", column_to_evaluate);
	  break;
	}

	keypad_init(); // set the columns high again
	return key_pressed; // invalid: 0xFF, valid:[0x00-0x0F]
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
  MX_USART2_UART_Init();
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */
  ring_buffer_init(&ring_buffer_keypad, keypad_data, 5);
  ring_buffer_init(&ring_buffer_uart_rx, rx_buffer, 16);

  HAL_UART_Receive_IT(&huart2, &rx_data, 1);

  keypad_init(); // Initialize the keypad functionality
  ssd1306_Init();
  ssd1306_Fill(Black);
  ssd1306_SetCursor(20, 20);
  ssd1306_WriteString("Hello World!", Font_7x10, White);
  ssd1306_UpdateScreen();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if (key_event != 0xFF) { // check if there is a event from the EXTi callback
		  uint16_t key_pressed = keypad_handler(key_event); // call the keypad handler

		  if (key_pressed != 0xFF) {
			                              // print the key pressed
			  process_key_press(key_pressed);

		  }
		  key_event = 0xFF; // clean the event
	  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10909CEC;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|ROW_1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, ROW_2_Pin|ROW_4_Pin|ROW_3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin ROW_1_Pin */
  GPIO_InitStruct.Pin = LD2_Pin|ROW_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : COLUMN_1_Pin */
  GPIO_InitStruct.Pin = COLUMN_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(COLUMN_1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : COLUMN_4_Pin */
  GPIO_InitStruct.Pin = COLUMN_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(COLUMN_4_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : COLUMN_2_Pin COLUMN_3_Pin */
  GPIO_InitStruct.Pin = COLUMN_2_Pin|COLUMN_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : ROW_2_Pin ROW_4_Pin ROW_3_Pin */
  GPIO_InitStruct.Pin = ROW_2_Pin|ROW_4_Pin|ROW_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
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
