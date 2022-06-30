#include "Test.hpp"
#include "rlm3-flash.h"
#include "FreeRTOS.h"
#include "task.h"
#include "logger.h"
#include <random>


LOGGER_ZONE(FLASH_TESTS);


static uint8_t g_buffer[RLM3_FLASH_SIZE + 1];


TEST_CASE(Flash_Lifecycle_HappyCase)
{
	ASSERT(!RLM3_Flash_IsInit());
	RLM3_Flash_Init();
	ASSERT(RLM3_Flash_IsInit());
	RLM3_Flash_Deinit();
	ASSERT(!RLM3_Flash_IsInit());
}

TEST_CASE(Flash_Read_HappyCase)
{
	RLM3_Flash_Init();

	TickType_t start_time = xTaskGetTickCount();
	ASSERT(RLM3_Flash_Read(0, g_buffer, RLM3_FLASH_SIZE));
	TickType_t end_time = xTaskGetTickCount();

	RLM3_Flash_Deinit();

	LOG_ALWAYS("Read flash in %d ms", (int)(end_time - start_time));
}

TEST_CASE(Flash_Read_NonInit)
{
	ASSERT(!RLM3_Flash_Read(0, g_buffer, 10));
}

TEST_CASE(Flash_Read_InvalidAddress)
{
	RLM3_Flash_Init();
	ASSERT(!RLM3_Flash_Read(0, g_buffer, 2049));
	ASSERT(!RLM3_Flash_Read(2048, g_buffer, 1));
	RLM3_Flash_Deinit();
}

TEST_CASE(Flash_Write_HappyCase)
{
	std::default_random_engine random_set(20220630);
	std::default_random_engine random_check(20220630);

	RLM3_Flash_Init();

	for (size_t i = 0; i < RLM3_FLASH_SIZE; i++)
		g_buffer[i] = (uint8_t)random_set();

	TickType_t start_time = xTaskGetTickCount();
	ASSERT(RLM3_Flash_Write(0, g_buffer, RLM3_FLASH_SIZE));
	TickType_t end_time = xTaskGetTickCount();

	for (size_t i = 0; i < RLM3_FLASH_SIZE; i++)
		g_buffer[i] = 0;

	ASSERT(RLM3_Flash_Read(0, g_buffer, RLM3_FLASH_SIZE));

	for (size_t i = 0; i < RLM3_FLASH_SIZE; i++)
		ASSERT(g_buffer[i] == (uint8_t)random_check());

	RLM3_Flash_Deinit();

	LOG_ALWAYS("Wrote flash in %d ms", (int)(end_time - start_time));
}

TEST_CASE(Flash_Write_NonInit)
{
	ASSERT(!RLM3_Flash_Write(0, g_buffer, 10));
}

TEST_CASE(Flash_Write_InvalidAddress)
{
	RLM3_Flash_Init();
	ASSERT(!RLM3_Flash_Write(0, g_buffer, 2049));
	ASSERT(!RLM3_Flash_Write(2048, g_buffer, 1));
	RLM3_Flash_Deinit();
}

TEST_CASE(Flash_Write_Random)
{
	std::default_random_engine random(20220631);
	std::default_random_engine random_set(20220630);
	std::default_random_engine random_check(20220630);

	RLM3_Flash_Init();

	for (size_t i = 0; i < 128; i++)
	{
		std::uniform_int_distribution<size_t> size_dist(1, 32);
		size_t size = size_dist(random);
		std::uniform_int_distribution<size_t> start_dist(0, RLM3_FLASH_SIZE - size);
		size_t start = start_dist(random);

		for (size_t i = start; i < start + size; i++)
			g_buffer[i] = (uint8_t)random_set();
		ASSERT(RLM3_Flash_Write(start, g_buffer + start, size));

		for (size_t i = start; i < start + size; i++)
			g_buffer[i] = 0;
		ASSERT(RLM3_Flash_Read(start, g_buffer + start, size));

		for (size_t i = start; i < start + size; i++)
			ASSERT(g_buffer[i] == (uint8_t)random_check());
	}

	RLM3_Flash_Deinit();
}

TEST_TEARDOWN(Flash_Cleanup)
{
	if (RLM3_Flash_IsInit())
		RLM3_Flash_Deinit();
}
