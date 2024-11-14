#include <esp_log.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <diskio.h>
#include <esp_heap_caps.h>
#include "esp_spiffs.h"

#include <dirent.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "sdcard.h"

#define SD_PIN_NUM_CS 22
#define SD_PIN_NUM_MISO 19
#define SD_PIN_NUM_MOSI 23
#define SD_PIN_NUM_CLK  18

static const char *TAG = "RETRO-RULER";

extern esp_err_t ff_diskio_get_drive(BYTE* out_pdrv);
extern void ff_diskio_register_sdmmc(unsigned char pdrv, sdmmc_card_t* card);

inline static void swap(char** a, char** b)
{
    char* t = *a;
    *a = *b;
    *b = t;
}

static int strcicmp(char const *a, char const *b)
{
    for (;; a++, b++)
    {
        int d = tolower((int)*a) - tolower((int)*b);
        if (d != 0 || !*a) return d;
    }
}

static int partition(char* arr[], int low, int high)
{
    char* pivot = arr[high];
    int i = (low - 1);

    for (int j = low; j <= high- 1; j++)
    {
        if (strcicmp(arr[j], pivot) < 0)
        {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

static void quick_sort(char* arr[], int low, int high)
{
    if (low < high)
    {
        int pi = partition(arr, low, high);

        quick_sort(arr, low, pi - 1);
        quick_sort(arr, pi + 1, high);
    }
}

static void sort_files(char** files, int count)
{
    if (count > 1)
    {
        quick_sort(files, 0, count - 1);
    }
}


int odroid_sdcard_files_get(const char* path, const char* extension, char*** filesOut)
{
    const int MAX_FILES = 1024;

    int count = 0;
    char** result = (char**)malloc(MAX_FILES * sizeof(void*));
    if (!result) abort();


    DIR *dir = opendir(path);
    if( dir == NULL )
    {
        return 0;
    }

    int extensionLength = strlen(extension);
    if (extensionLength < 1) abort();

    struct dirent *entry;
    while ((entry = readdir(dir)))
    {
        size_t len = strlen(entry->d_name);

        if (len < extensionLength)
            continue;

        if (entry->d_name[0] == '.')
            continue;

        if (strcasecmp(extension, &entry->d_name[len - extensionLength]) != 0)
            continue;

        if (!(result[count++] = strdup(entry->d_name)))
            abort();

        if (count >= MAX_FILES)
            break;
    }

    closedir(dir);

    sort_files(result, count);

    *filesOut = result;
    return count;
}

void odroid_sdcard_files_free(char** files, int count)
{
    for (int i = 0; i < count; ++i)
    {
        free(files[i]);
    }

    free(files);
}

esp_err_t odroid_sdcard_open(void)
{
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
            .format_if_mount_failed = false,
            .max_files = 5,
            .allocation_unit_size = 16 * 1024,
    };

    sdmmc_card_t* card;

    sdmmc_host_t host_config = SDSPI_HOST_DEFAULT();
    host_config.slot = HSPI_HOST;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs =  SD_PIN_NUM_CS;
    slot_config.host_id = host_config.slot;

    ESP_LOGI(TAG, "Mounting SD card");
    esp_err_t ret = esp_vfs_fat_sdspi_mount(SDCARD_BASE_PATH, &host_config, &slot_config, &mount_config, &card);

    if (ret == ESP_OK || ret == ESP_ERR_INVALID_STATE)
    {
        ret = ESP_OK;
    }

    return ret;
}

esp_err_t odroid_sdcard_close(void)
{
    esp_err_t ret = esp_vfs_fat_sdmmc_unmount();
    return ret;
}

/*
esp_err_t odroid_sdcard_format(int fs_type)
{
    esp_err_t err = ESP_FAIL;
    const char *errmsg = "success!";
    sdmmc_card_t card;
    void *buffer = malloc(4096);
    DWORD partitions[] = {100, 0, 0, 0};
    BYTE drive = 0xFF;
    
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();

    if (buffer == NULL) {
        return false;
    }

    odroid_sdcard_close();

    err = ff_diskio_get_drive(&drive);
    if (drive == 0xFF) {
        errmsg = "ff_diskio_get_drive() failed";
        goto _cleanup;
    }

    err = (*host_config.init)();
    if (err != ESP_OK) {
        errmsg = "host_config.init() failed";
        goto _cleanup;
    }

    err = sdmmc_host_init_slot(host_config.slot, &slot_config);

    if (err != ESP_OK) {
        errmsg = "sdmmc_host_init_slot() failed";
        goto _cleanup;
    }

    err = sdmmc_card_init(&host_config, &card);
    if (err != ESP_OK) {
        errmsg = "sdmmc_card_init() failed";
        goto _cleanup;
    }

    ff_diskio_register_sdmmc(drive, &card);

    if (f_fdisk(drive, partitions, buffer) != FR_OK) {
        errmsg = "f_fdisk() failed";
        err = ESP_FAIL;
        goto _cleanup;
    }

    char path[3] = {(char)('0' + drive), ':', 0};
    if (f_mkfs(path, fs_type ? FM_EXFAT : FM_FAT32, 0, buffer, 4096) != FR_OK) {
        errmsg = "f_mkfs() failed";
        err = ESP_FAIL;
        goto _cleanup;
    }

    err = ESP_OK;

_cleanup:

    free(buffer);
    host_config.deinit();
    ff_diskio_register_sdmmc(drive, NULL);

    return err;
}
*/