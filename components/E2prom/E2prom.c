#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "E2prom.h"

#define I2C_MASTER_SCL_IO               (GPIO_NUM_14)               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO               (GPIO_NUM_27)               /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM                  I2C_NUM_1        /*!< I2C port number for master dev */
#define I2C_MASTER_TX_BUF_DISABLE       0                /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE       0                /*!< I2C master do not need buffer */
#define I2C_MASTER_FREQ_HZ              100000           /*!< I2C master clock frequency */


#define ACK_CHECK_EN                       0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                      0x0              /*!< I2C master will not check ack from slave */
#define ACK_VAL                            0x0              /*!< I2C ack value */
#define NACK_VAL                           0x1              /*!< I2C nack value */

#define ADDR_PAGE0 0xA8
#define ADDR_PAGE1 0xAA
#define ADDR_PAGE2 0xAC
#define ADDR_PAGE3 0xAE





static const char *TAG = "EEPROM";

static esp_err_t EE_Page_Write(uint8_t page,uint8_t reg_addr,uint8_t *dat,int length)
{
	int ret;
    int i;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, page, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);
    for(i=0;i<length;i++)
    {
        i2c_master_write_byte(cmd, *dat, ACK_CHECK_EN);
        dat++;	
    }
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;

}



static esp_err_t EE_Page_Read(uint8_t page,uint8_t reg_addr,uint8_t *dat,int length)
{
	int ret;
    int i;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, page, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, page+1, ACK_CHECK_EN);


    for(i=0;i<length;i++)
    {
        if(i!=length-1)
        {
            i2c_master_read_byte(cmd, dat, ACK_VAL);
            dat++;
        }
        else
        {
            i2c_master_read_byte(cmd, dat, NACK_VAL);
        }

    }
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

void E2prom_Init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode,
                       I2C_MASTER_RX_BUF_DISABLE,
                       I2C_MASTER_TX_BUF_DISABLE, 0);
}

int E2prom_BluWrite(uint8_t addr,uint8_t*data_write,int len)
{
    if((addr%16)!=0)
    {
        ESP_LOGE(TAG, "Addr Mast Multiple 16!");
        return 0;
    }
    if(len>256)
    {
        ESP_LOGE(TAG, "bludata len must <= 256 byte");
        return 0;
    }
    int ret=0;
    int i=0,j=0;
    i=len/16;
    j=len%16;
    uint8_t *data_write_temp = data_write; 
    while(i>0)
    {
        ret = EE_Page_Write(ADDR_PAGE1,addr,data_write,16);
        if(ret == ESP_ERR_TIMEOUT) 
        {
            ESP_LOGE(TAG, "Write timeout");
            return ret;
        } 
        else if(ret == ESP_OK) 
        {
            ESP_LOGI(TAG, "Write ok");
        } 
        else 
        {
            ESP_LOGE(TAG, "Write No ack, chip not connected");
            return ret;
        }
        data_write+=16;
        addr+=16;
        i--;
        vTaskDelay(20 / portTICK_RATE_MS);
    }

    if(j>0)
    {
        ret = EE_Page_Write(ADDR_PAGE1,addr,data_write,j);
        if(ret == ESP_ERR_TIMEOUT) 
        {
            ESP_LOGE(TAG, "Write timeout");
            return ret;
        } 
        else if(ret == ESP_OK) 
        {
            ESP_LOGI(TAG, "Write ok");
        } 
        else 
        {
            ESP_LOGE(TAG, "Write No ack, chip not connected");
            return ret;
        }     
        j=0;
    }
    data_write=data_write_temp;
    vTaskDelay(20 / portTICK_RATE_MS);
    return ret;

}


int E2prom_Write(uint8_t addr,uint8_t*data_write,int len)
{
    if((addr%16)!=0)
    {
        ESP_LOGE(TAG, "Addr Mast Multiple 16!");
        return 0;
    }
    int ret=0;
    int i=0,j=0;
    i=len/16;
    j=len%16;
    uint8_t *data_write_temp = data_write; 
    while(i>0)
    {
        ret = EE_Page_Write(ADDR_PAGE0,addr,data_write,16);
        if(ret == ESP_ERR_TIMEOUT) 
        {
            ESP_LOGE(TAG, "Write timeout");
            return ret;
        } 
        else if(ret == ESP_OK) 
        {
            ESP_LOGI(TAG, "Write ok");
        } 
        else 
        {
            ESP_LOGE(TAG, "Write No ack, chip not connected");
            return ret;
        }
        data_write+=16;
        addr+=16;
        i--;
        vTaskDelay(20 / portTICK_RATE_MS);
    }

    if(j>0)
    {
        ret = EE_Page_Write(ADDR_PAGE0,addr,data_write,j);
        if(ret == ESP_ERR_TIMEOUT) 
        {
            ESP_LOGE(TAG, "Write timeout");
            return ret;
        } 
        else if(ret == ESP_OK) 
        {
            ESP_LOGI(TAG, "Write ok");
        } 
        else 
        {
            ESP_LOGE(TAG, "Write No ack, chip not connected");
            return ret;
        }     
        j=0;
    }
    data_write=data_write_temp;
    vTaskDelay(20 / portTICK_RATE_MS);
    return ret;

}


int E2prom_Read(uint8_t addr,uint8_t*data_read,int len)
{
    if((addr%16)!=0)
    {
        ESP_LOGE(TAG, "Addr Mast Multiple 16!");
        return 0;
    }
    int ret=0;
    int i=0,j=0;
    i=len/16;
    j=len%16; 
    uint8_t *data_read_temp = data_read; 
    while(i>0)
    {
        ret = EE_Page_Read(ADDR_PAGE0,addr,data_read,16);
        if(ret == ESP_ERR_TIMEOUT) 
        {
            ESP_LOGE(TAG, "Read timeout");
            return ret;
        } 
        else if(ret == ESP_OK) 
        {
            ESP_LOGI(TAG, "Read ok");
        } 
        else 
        {
            ESP_LOGE(TAG, "Read No ack, chip not connected");
            return ret;
        }
        i--;
        addr+=16;
        data_read+=16;
        vTaskDelay(20 / portTICK_RATE_MS);
    }

     if(j>0)
     {
        ret = EE_Page_Read(ADDR_PAGE0,addr,data_read,j);
        if(ret == ESP_ERR_TIMEOUT) 
        {
            ESP_LOGE(TAG, "Read timeout");
            return ret;
        } 
        else if(ret == ESP_OK) 
        {
            ESP_LOGI(TAG, "Read ok");
        } 
        else 
        {
            ESP_LOGE(TAG, "Read No ack, chip not connected");
            return ret;
        }
     }

     data_read=data_read_temp;
     vTaskDelay(20 / portTICK_RATE_MS);
     return ret;
}




int E2prom_BluRead(uint8_t addr,uint8_t*data_read,int len)
{
    if((addr%16)!=0)
    {
        ESP_LOGE(TAG, "Addr Mast Multiple 16!");
        return 0;
    }
    int ret=0;
    int i=0,j=0;
    i=len/16;
    j=len%16; 
    uint8_t *data_read_temp = data_read; 
    while(i>0)
    {
        ret = EE_Page_Read(ADDR_PAGE1,addr,data_read,16);
        if(ret == ESP_ERR_TIMEOUT) 
        {
            ESP_LOGE(TAG, "Read timeout");
            return ret;
        } 
        else if(ret == ESP_OK) 
        {
            ESP_LOGI(TAG, "Read ok");
        } 
        else 
        {
            ESP_LOGE(TAG, "Read No ack, chip not connected");
            return ret;
        }
        i--;
        addr+=16;
        data_read+=16;
        vTaskDelay(20 / portTICK_RATE_MS);
    }

     if(j>0)
     {
        ret = EE_Page_Read(ADDR_PAGE1,addr,data_read,j);
        if(ret == ESP_ERR_TIMEOUT) 
        {
            ESP_LOGE(TAG, "Read timeout");
            return ret;
        } 
        else if(ret == ESP_OK) 
        {
            ESP_LOGI(TAG, "Read ok");
        } 
        else 
        {
            ESP_LOGE(TAG, "Read No ack, chip not connected");
            return ret;
        }
     }

     data_read=data_read_temp;
     vTaskDelay(20 / portTICK_RATE_MS);
     return ret;
}

