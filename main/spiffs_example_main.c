/* SPIFFS filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "example";
char filename[20] = "/spiffs/test.txt";

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true};

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return;
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    // Check consistency of reported partiton size info.
    if (used > total)
    {
        ESP_LOGW(TAG, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
        ret = esp_spiffs_check(conf.partition_label);
        // Could be also used to mend broken files, to clean unreferenced pages, etc.
        // More info at https://github.com/pellepl/spiffs/wiki/FAQ#powerlosses-contd-when-should-i-run-spiffs_check
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            return;
        }
        else
        {
            ESP_LOGI(TAG, "SPIFFS_check() successful");
        }
    }

    // set max file size to 75% of the reported total size
    int32_t available_space = (total - used);
    FILE *f;

    ESP_LOGI(TAG, "Checking for  existing file or creating new file");
    ret = ESP_OK;
    // Check if file already exists
    struct stat st;
    if (stat(filename, &st) != 0)
    {
        // File does not exist yet
        f = fopen(filename, "w"); // create new file
        fclose(f);
        if (f == NULL)
        {
            ESP_LOGE(TAG, "Failed to create file");
            ret = ESP_FAIL;
        }
        else
        {
            ESP_LOGI(TAG, "New file created: %s", filename);
        }
    }
    else
    {
        ESP_LOGI(TAG, "Using existing file: %s", filename);
    }

    // CONDITION: file already existed, or is newly created
    while (available_space > 1000)
    {
        f = fopen(filename, "r+"); // open text file for update (read and write)
        if (f == NULL)
        {
            for (int gdb=1; gdb==1;) //value of gdb can be changed in gdb
            {
                ESP_LOGE(TAG, "THIS SHOULD NOT HAPPEN - Failed to open file for updating, %d ", available_space);
                //vTaskDelay(1000 / portTICK_PERIOD_MS);
                // abort();
            }
        }
        else
        {
            fseek(f, 0, SEEK_END);

            fprintf(f, "this is a log test string to fill up the file, for testing whether everything works OK\n");
            fclose(f);

            ret = esp_spiffs_info(conf.partition_label, &total, &used);
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
                esp_spiffs_format(conf.partition_label);
                return;
            }
            else
            {
                ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
                available_space = (total - used);
            }
        }
    }

    // All done, unmount partition and disable SPIFFS
    esp_vfs_spiffs_unregister(conf.partition_label);
    ESP_LOGI(TAG, "SPIFFS unmounted");
}
