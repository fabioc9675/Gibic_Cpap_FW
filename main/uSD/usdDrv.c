#include "uSD/usdDrv.h"

void init_sdmmc(void) {

    // Configura los pines para el módulo SDMMC
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.flags = SDMMC_HOST_FLAG_4BIT; // Configura para 4 líneas de datos
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED; // Configura la frecuencia máxima a 20MHz

    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 4; // Configura para 4 líneas de datos

    // Configura los pines específicos para el ESP32-S3
    slot.gpio_cd = SDMMC_SLOT_NO_CD; // No se usa pin de detección de tarjeta
    slot.gpio_wp = SDMMC_SLOT_NO_WP; // No se usa pin de protección contra escritura

    // Especifica los pines GPIO para las líneas de datos
    
    slot.clk = 39;  // Pin para CLK
    slot.cmd = 40;  // Pin para CMD
    slot.d0 = 38;    // Pin para D0
    slot.d1 = 37;    // Pin para D1
    slot.d2 = 42;   // Pin para D2
    slot.d3 = 41;   // Pin para D3

    // Monta el sistema de archivos FAT
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sd", &host, &slot, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            printf("Failed to mount filesystem. "
                   "If you want the card to be formatted, set format_if_mount_failed = true.\n");
        } else {
            printf("Failed to initialize the card (%s). "
                   "Make sure SD card lines have pull-up resistors in place.\n", esp_err_to_name(ret));
        }
        return;
    }

    // Tarjeta inicializada correctamente
    sdmmc_card_print_info(stdout, card);
}