#include "Test.hpp"
#include "rlm3-flash.h"
#include "rlm3-i2c.h"
#include "Mock.hpp"
#include <random>
#include "FreeRTOS.h"
#include "task.h"


TEST_CASE(Flash_Init_HappyCase)
{
	EXPECT(RLM3_I2C1_Init(RLM3_I2C1_DEVICE_FLASH));

	RLM3_Flash_Init();
}

TEST_CASE(Flash_Deinit_HappyCase)
{
	EXPECT(RLM3_I2C1_Deinit(RLM3_I2C1_DEVICE_FLASH));

	RLM3_Flash_Deinit();
}

TEST_CASE(Flash_IsInit_HappyCase)
{
	EXPECT(RLM3_I2C1_IsInit(RLM3_I2C1_DEVICE_FLASH))_AND_RETURN(true);
	EXPECT(RLM3_I2C1_IsInit(RLM3_I2C1_DEVICE_FLASH))_AND_RETURN(false);

	ASSERT(RLM3_Flash_IsInit());
	ASSERT(!RLM3_Flash_IsInit());
}

TEST_CASE(Flash_Write_HappyCase)
{
	std::default_random_engine random(20220708);
	uint8_t data[RLM3_FLASH_SIZE];
	for (uint8_t& x : data)
		x = random();

	EXPECT(RLM3_Flash_IsInit())_AND_RETURN(true);
	for (size_t i = 0; i < RLM3_FLASH_SIZE; i += 16)
	{
		uint8_t block[17];
		block[0] = i;
		for (size_t j = 0; j < 16; j++)
			block[1 + j] = data[i + j];
		EXPECT(RLM3_I2C1_Transmit(0x50 | (i >> 8), block, 17))_AND_RETURN(true);
		EXPECT(vTaskDelay(6));
	}

	ASSERT(RLM3_Flash_Write(0, data, RLM3_FLASH_SIZE));
}

TEST_CASE(Flash_Write_NotInitialized)
{
	uint8_t data[RLM3_FLASH_SIZE];

	EXPECT(RLM3_Flash_IsInit())_AND_RETURN(false);

	ASSERT(!RLM3_Flash_Write(0, data, RLM3_FLASH_SIZE));
}

TEST_CASE(Flash_Write_Failure)
{
	std::default_random_engine random(20220708);
	uint8_t data[RLM3_FLASH_SIZE];
	for (uint8_t& x : data)
		x = random();

	EXPECT(RLM3_Flash_IsInit())_AND_RETURN(true);
	uint8_t block[17] = {};
	for (size_t j = 0; j < 16; j++)
		block[1 + j] = data[j];
	EXPECT(RLM3_I2C1_Transmit(0x50, block, 17))_AND_RETURN(false);
	EXPECT(vTaskDelay(6));

	ASSERT(!RLM3_Flash_Write(0, data, RLM3_FLASH_SIZE));
}

TEST_CASE(Flash_Write_InvalidAddress)
{
	std::default_random_engine random(20220708);
	uint8_t data[RLM3_FLASH_SIZE];
	for (uint8_t& x : data)
		x = random();

	EXPECT(RLM3_Flash_IsInit())_AND_RETURN(true);

	ASSERT(!RLM3_Flash_Write(RLM3_FLASH_SIZE, data, 1));
}

TEST_CASE(Flash_Write_InvalidSize)
{
	std::default_random_engine random(20220708);
	uint8_t data[RLM3_FLASH_SIZE + 1];
	for (uint8_t& x : data)
		x = random();

	EXPECT(RLM3_Flash_IsInit())_AND_RETURN(true);

	ASSERT(!RLM3_Flash_Write(0, data, RLM3_FLASH_SIZE + 1));
}

TEST_CASE(Flash_Write_Empty)
{
	EXPECT(RLM3_Flash_IsInit())_AND_RETURN(true);

	uint8_t data = 0;
	ASSERT(RLM3_Flash_Write(0, &data, 0));
}

TEST_CASE(Flash_Write_PartialBlocks)
{
	std::default_random_engine random(20220708);
	uint8_t data[RLM3_FLASH_SIZE];
	for (uint8_t& x : data)
		x = random();

	EXPECT(RLM3_Flash_IsInit())_AND_RETURN(true);
	uint8_t block[17];
	block[0] = 7;
	for (size_t j = 0; j < 9; j++)
		block[1 + j] = data[7 + j];
	EXPECT(RLM3_I2C1_Transmit(0x50, block, 10))_AND_RETURN(true);
	EXPECT(vTaskDelay(6));
	block[0] = 16;
	for (size_t j = 0; j < 16; j++)
		block[1 + j] = data[16 + j];
	EXPECT(RLM3_I2C1_Transmit(0x50, block, 17))_AND_RETURN(true);
	EXPECT(vTaskDelay(6));
	block[0] = 32;
	for (size_t j = 0; j < 5; j++)
		block[1 + j] = data[32 + j];
	EXPECT(RLM3_I2C1_Transmit(0x50, block, 6))_AND_RETURN(true);
	EXPECT(vTaskDelay(6));

	ASSERT(RLM3_Flash_Write(7, data + 7, 9 + 16 + 5));
}

TEST_CASE(Flash_Read_HappyCase)
{
	std::default_random_engine random(20220708);
	uint8_t expected_data[RLM3_FLASH_SIZE];
	for (uint8_t& x : expected_data)
		x = random();

	EXPECT(RLM3_Flash_IsInit())_AND_RETURN(true);
	uint8_t byte_address = 0;
	EXPECT(RLM3_I2C1_TransmitReceive(0x50, &byte_address, 1, expected_data, RLM3_FLASH_SIZE))_AND_RETURN(true);

	uint8_t actual_data[RLM3_FLASH_SIZE] = {};
	ASSERT(RLM3_Flash_Read(0, actual_data, RLM3_FLASH_SIZE));

	for (size_t i = 0; i < RLM3_FLASH_SIZE; i++)
		ASSERT(expected_data[i] == actual_data[i]);
}

TEST_CASE(Flash_Read_NotInitialized)
{
	EXPECT(RLM3_Flash_IsInit())_AND_RETURN(false);

	uint8_t actual_data[RLM3_FLASH_SIZE] = {};
	ASSERT(!RLM3_Flash_Read(0, actual_data, RLM3_FLASH_SIZE));
}

TEST_CASE(Flash_Read_Failure)
{
	std::default_random_engine random(20220708);
	uint8_t expected_data[RLM3_FLASH_SIZE];
	for (uint8_t& x : expected_data)
		x = random();

	EXPECT(RLM3_Flash_IsInit())_AND_RETURN(true);
	uint8_t byte_address = 0;
	EXPECT(RLM3_I2C1_TransmitReceive(0x50, &byte_address, 1, expected_data, RLM3_FLASH_SIZE))_AND_RETURN(false);

	uint8_t actual_data[RLM3_FLASH_SIZE] = {};
	ASSERT(!RLM3_Flash_Read(0, actual_data, RLM3_FLASH_SIZE));
}

TEST_CASE(Flash_Read_InvalidAddress)
{
	EXPECT(RLM3_Flash_IsInit())_AND_RETURN(true);

	uint8_t data = 0;
	ASSERT(!RLM3_Flash_Read(RLM3_FLASH_SIZE, &data, 1));
}

TEST_CASE(Flash_Read_InvalidSize)
{
	EXPECT(RLM3_Flash_IsInit())_AND_RETURN(true);

	uint8_t data[RLM3_FLASH_SIZE + 1] = {};
	ASSERT(!RLM3_Flash_Read(0, data, RLM3_FLASH_SIZE + 1));
}

TEST_CASE(Flash_Read_Empty)
{
	EXPECT(RLM3_Flash_IsInit())_AND_RETURN(true);

	uint8_t data = 0;
	ASSERT(RLM3_Flash_Read(7, &data, 0));
}

TEST_CASE(Flash_Read_PartialBlock)
{
	std::default_random_engine random(20220708);
	uint8_t expected_data[9 + 16 + 5];
	for (uint8_t& x : expected_data)
		x = random();

	EXPECT(RLM3_Flash_IsInit())_AND_RETURN(true);
	uint8_t byte_address = 7;
	EXPECT(RLM3_I2C1_TransmitReceive(0x50, &byte_address, 1, expected_data, 9 + 16 + 5))_AND_RETURN(true);

	uint8_t actual_data[9 + 16 + 5] = {};
	ASSERT(RLM3_Flash_Read(7, actual_data, 9 + 16 + 5));

	for (size_t i = 0; i < 9 + 16 + 5; i++)
		ASSERT(expected_data[i] == actual_data[i]);
}

