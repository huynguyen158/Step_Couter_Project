
#include "main.h"
#include "stm32f1xx_it.h"
#include "stm32f1xx_hal_tim.h"
#include "stm32f1xx_hal_gpio.h"
#include "i2c-lcd.h"

/* Private includes ----------------------------------------------------------*/
#include "stdlib.h"
#include "stdio.h"
#include "stdarg.h"
#include "string.h"
#include "math.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/

#define MPU6050_ADDR 0xD0


#define SMPLRT_DIV_REG 0x19
#define GYRO_CONFIG_REG 0x1B
#define ACCEL_CONFIG_REG 0x1C
#define ACCEL_XOUT_H_REG 0x3B
#define TEMP_OUT_H_REG 0x41
#define GYRO_XOUT_H_REG 0x43
#define PWR_MGMT_1_REG 0x6B
#define WHO_AM_I_REG 0x75


int16_t Accel_X_RAW = 0;
int16_t Accel_Y_RAW = 0;
int16_t Accel_Z_RAW = 0;

int16_t Gyro_X_RAW = 0;
int16_t Gyro_Y_RAW = 0;
int16_t Gyro_Z_RAW = 0;

float Ax, Ay, Az;

float vectorprevious;
float vector;
float totalvector;
int Steps = 0;
uint32_t Count = 0;

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void MPU6050_Init(void)
{
    uint8_t check;
    uint8_t Data;

    // Kiem tra WHO_AM_I register dê xác nhan device ID cua MPU6050

    HAL_I2C_Mem_Read(&hi2c2, MPU6050_ADDR, WHO_AM_I_REG, 1, &check, 1, 1000);

    if (check == 0x68)  // 0x68 se duoc tra ve cam bien neu moi thu dien ra tot
    {
        // Ghi tât ca các bit trong PWR_MGMT_1 register là 0 de dánh thuc cam bien
        Data = 0;
        HAL_I2C_Mem_Write(&hi2c2, MPU6050_ADDR, PWR_MGMT_1_REG, 1, &Data, 1, 1000);

        // Thiet lap DATA RATE là 1kHz bang cách ghi vào SMPLRT_DIV register
        Data = 0x07;
        HAL_I2C_Mem_Write(&hi2c2, MPU6050_ADDR, SMPLRT_DIV_REG, 1, &Data, 1, 1000);

        // Thiet lap cau hình gia toc trong ACCEL_CONFIG register
        // XA_ST=0, YA_ST=0, ZA_ST=0, FS_SEL=0 -> ± 2g
        Data = 0x00;
        HAL_I2C_Mem_Write(&hi2c2, MPU6050_ADDR, ACCEL_CONFIG_REG, 1, &Data, 1, 1000);

        // Thiet lap cau hình gyroscope trong GYRO_CONFIG register
        // XG_ST=0, YG_ST=0, ZG_ST=0, FS_SEL=0 -> ± 250 °/s
        Data = 0x00;
        HAL_I2C_Mem_Write(&hi2c2, MPU6050_ADDR, GYRO_CONFIG_REG, 1, &Data, 1, 1000);
    }
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  
  HAL_Init();


  SystemClock_Config();


  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_TIM2_Init();
//	HAL_TIM_Base_Start_IT(&htim2);
	MPU6050_Init();
	lcd_init();
	lcd_clear();
  /* USER CODE BEGIN 2 */
	char data[4];
	uint8_t Rec_Data[6] = {0,0,0,0,0,0};
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		// Ðoc 6 byte du lieu tu thanh ghi ACCEL_XOUT_H_REG cua MPU6050 thông qua giao dien I2C. Ket qua duoc luu vào mang Rec_Data.
    HAL_I2C_Mem_Write(&hi2c2, MPU6050_ADDR, ACCEL_CONFIG_REG, 1, 0, 0, 1000);
		HAL_I2C_Mem_Read (&hi2c2, MPU6050_ADDR, ACCEL_XOUT_H_REG, 1, Rec_Data, 6, 1000);
		// chuyen doi du lieu doc duoc thanh gia tri gia toc
		//Ket hop hai byte dau cua Rec_Data de tao giá tri gia toc theo truc X.
		Accel_X_RAW = (int16_t)(Rec_Data[0] << 8 | Rec_Data [1]);
		//Ket hop hai byte dau cua Rec_Data de tao giá tri gia toc theo truc y
		Accel_Y_RAW = (int16_t)(Rec_Data[2] << 8 | Rec_Data [3]);
		//Ket hop hai byte dau cua Rec_Data de tao giá tri gia toc theo truc Z
		Accel_Z_RAW = (int16_t)(Rec_Data[4] << 8 | Rec_Data [5]);

		
// Tinh toan gia tri vecto va kiem tra buoc chan
		//Ax, Ay, Az là giá tri gia toc chuan hóa (tu -2 den +2) theo các truc.
		Ax = Accel_X_RAW/16384.0;
		Ay = Accel_Y_RAW/16384.0;
		Az = Accel_Z_RAW/16384.0;
		//
		if(0u == HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_4)) {
			lcd_clear();																						// Clear display LCD
			HAL_TIM_Base_Stop_IT(&htim2);															// Disable TIM2
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, 0);
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, 1);
			Steps = 0;																							// Reset counter step
			lcd_put_cur(0, 1);																			// Print string in column 2, row 1
			lcd_send_string ("HUY LOC");																								 																													
			lcd_put_cur(1, 3);																			// Print string in column 4, row 2
			lcd_send_string("Steps= ");																// Print string
			sprintf(data, "%d", Steps);															// Convert Steps from int to char
			lcd_send_string(data);
		}
		else {
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, 0);
			if(0u == HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_3)) {
				HAL_Delay(100);																						// Wait for some time to stabilize the system
				if(0u == HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_3)) {															// Check pressed SS Pin Switch again
				while(0u == HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_3));													// Wait to Unpressed SS Pin Switch
				Count++;																						// Increase the counter by 1
				}
			}
			
			if(Count % 2 !=0) {
				HAL_TIM_Base_Start_IT(&htim2);
				//Tính toán d? dài vector trong không gian 3D b?ng cách tính can b?c hai cua t?ng bình phuong các thành ph?n gia t?c.
				vector = sqrt((Ax * Ax) + (Ay * Ay) + (Az * Az));
				// Tính toán s? khác bi?t gi?a vecto hi?n t?i và vector tru?c dó
				totalvector = vector - vectorprevious;								// Calculate the difference from the previous value
    //N?u totalvector nho hon -4, có the xác dinh là mot buoc chân và tang bien Steps lên 1.
				if ((totalvector*100) < -4){
					Steps++;																						// Increase the Steps value by 1
				}
			}
			/* If Count is even*/
			else {
				HAL_TIM_Base_Stop_IT(&htim2);
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, 0); //Tat chân GPIOA_PIN_1.
				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, 1);//Bat chân GPIOC_PIN_13.
			}
		}
		lcd_put_cur(0, 1);																		
		lcd_send_string("Step Couter");																 																													
		lcd_put_cur(1, 3);																			// Print string in column 4, row 2
		lcd_send_string("Steps= ");																// Print string
		sprintf(data, "%d", Steps);															// Convert Steps from int to char
		lcd_send_string(data);
    vectorprevious = vector;
		HAL_Delay(800);
  }
  /* USER CODE END 3 */
}
//khi mot chu ki cua Timer duoc dinh cau hình de ket thúc, hàm HAL_TIM_PeriodElapsedCallback se duoc goi và thuc hien các thao tác bên trong
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	//duoc su dung de dao trang thái cua chân GPIOA_PIN_1. Neu chân này dang o trong thái HIGH, thì hàm se chuyen nó thành trang thái LOW và nguoc lai.
	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{


  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7199;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB3 PB4 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
