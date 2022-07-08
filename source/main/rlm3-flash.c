#include "rlm3-flash.h"
#include "rlm3-i2c.h"
#include "Assert.h"
#include <string.h>
#include "logger.h"
#include "FreeRTOS.h"
#include "task.h"


// Driver for Microchip 24LC16BT


LOGGER_ZONE(FLASH);


#define RLM3_FLASH_PAGESIZE (16)
#define RLM3_FLASH_I2C_ADDRESS (0x50)
#define RLM3_FLASH_MAX_WRITE_CYCLE_TIME_MS (5)


extern void RLM3_Flash_Init()
{
	LOG_TRACE("INIT");
	RLM3_I2C1_Init(RLM3_I2C1_DEVICE_FLASH);
}

extern void RLM3_Flash_Deinit()
{
	LOG_TRACE("DEINIT");
	RLM3_I2C1_Deinit(RLM3_I2C1_DEVICE_FLASH);
}

extern bool RLM3_Flash_IsInit()
{
	return RLM3_I2C1_IsInit(RLM3_I2C1_DEVICE_FLASH);
}

static bool FlashWritePage(uint32_t flash_address, const uint8_t* data, size_t size)
{
	LOG_TRACE("PAGE 0x%x %d", (int)flash_address, (int)size);

	// Make sure this is a valid address.
	ASSERT(flash_address < RLM3_FLASH_SIZE);
	ASSERT(flash_address + size <= RLM3_FLASH_SIZE);

	// Make sure the whole write is to a single page.
	ASSERT(0 < size && size <= RLM3_FLASH_PAGESIZE);
	ASSERT(flash_address / RLM3_FLASH_PAGESIZE == (flash_address + size - 1) / RLM3_FLASH_PAGESIZE);

	// Prepare a message to write this block.
	uint8_t message[1 + RLM3_FLASH_PAGESIZE];
	message[0] = (uint8_t)flash_address;
	memcpy(message + 1, data, size);

	uint8_t block_addr = RLM3_FLASH_I2C_ADDRESS | (uint8_t)(flash_address >> 8);
	bool result = RLM3_I2C1_Transmit(block_addr, message, 1 + size);

	vTaskDelay(1 + pdMS_TO_TICKS(RLM3_FLASH_MAX_WRITE_CYCLE_TIME_MS));

	return result;
}

extern bool RLM3_Flash_Write(uint32_t flash_address, const uint8_t* data, size_t size)
{
	LOG_TRACE("WRITE 0x%x %d", (int)flash_address, (int)size);

	if (!RLM3_Flash_IsInit())
		return false;
	if (flash_address > RLM3_FLASH_SIZE || flash_address + size > RLM3_FLASH_SIZE)
		return false;

	bool result = true;
	while (result && size > 0)
	{
		// Write in page size blocks that never cross a block boundary.
		size_t length = RLM3_FLASH_PAGESIZE - flash_address % RLM3_FLASH_PAGESIZE;
		if (length > size)
			length = size;

		result = FlashWritePage(flash_address, data, length);

		flash_address += length;
		data += length;
		size -= length;
	}

	return result;
}

extern bool RLM3_Flash_Read(uint32_t flash_address, uint8_t* data, size_t size)
{
	LOG_TRACE("READ 0x%x %d", (int)flash_address, (int)size);

	if (!RLM3_Flash_IsInit())
		return false;
	if (flash_address > RLM3_FLASH_SIZE || flash_address + size > RLM3_FLASH_SIZE)
		return false;
	if (size == 0)
		return true;

	uint8_t block_addr = RLM3_FLASH_I2C_ADDRESS | (uint8_t)(flash_address >> 8);
	uint8_t byte_addr = (uint8_t)flash_address;

	return RLM3_I2C1_TransmitReceive(block_addr, &byte_addr, 1, data, size);
}
