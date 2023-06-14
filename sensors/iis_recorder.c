#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>


#define SPI_DEVICE "/dev/spidev0.1"
#define SPI_SPEED 8000000 // 1MHz

// Global SPI file descriptor
int spi_fd = -1;
static int SENSOR_BUS;

#include <string.h>
#include <stdio.h>
#include "iis3dwb_reg.h"
#include <time.h>

/* Private macro -------------------------------------------------------------*/
/*
 * Select FIFO samples watermark, max value is 512
 * in FIFO are stored acc, gyro and timestamp samples
 */
#define BOOT_TIME         10
#define FIFO_WATERMARK    128

/* Private variables ---------------------------------------------------------*/
static uint8_t whoamI;
static uint8_t tx_buffer[1000];
static iis3dwb_fifo_out_raw_t fifo_data[FIFO_WATERMARK];

/* Private variables ---------------------------------------------------------*/
static int16_t *datax;
static int16_t *datay;
static int16_t *dataz;
static int32_t *ts;

/* Extern variables ----------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/*
 *   WARNING:
 *   Functions declare in this section are defined at the end of this file
 *   and are strictly related to the hardware platform used.
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len);
static void tx_com( uint8_t *tx_buffer, uint16_t len );
static void platform_init(void);


/* Main Example --------------------------------------------------------------*/
void main(void)
{
  iis3dwb_fifo_status_t fifo_status;
  stmdev_ctx_t dev_ctx;
  uint8_t rst;

  /* Uncomment to configure INT 1 */
  //iis3dwb_pin_int1_route_t int1_route;
  /* Uncomment to configure INT 2 */
  //iis3dwb_pin_int2_route_t int2_route;
  /* Initialize mems driver interface */
  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg = platform_read;
  dev_ctx.handle = &SENSOR_BUS;
  /* Init test platform */
  platform_init();

  /* Check device ID */
  iis3dwb_device_id_get(&dev_ctx, &whoamI);

  if (whoamI != IIS3DWB_ID)
    while (1);

  /* Restore default configuration */
  iis3dwb_reset_set(&dev_ctx, PROPERTY_ENABLE);

  do {
    iis3dwb_reset_get(&dev_ctx, &rst);
  } while (rst);

  /* Enable Block Data Update */
  iis3dwb_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
  /* Set full scale */
  iis3dwb_xl_full_scale_set(&dev_ctx, IIS3DWB_8g);

  /*
   * Set FIFO watermark (number of unread sensor data TAG + 6 bytes
   * stored in FIFO) to FIFO_WATERMARK samples
   */
  iis3dwb_fifo_watermark_set(&dev_ctx, FIFO_WATERMARK);
  /* Set FIFO batch XL/Gyro ODR to 12.5Hz */
  iis3dwb_fifo_xl_batch_set(&dev_ctx, IIS3DWB_XL_BATCHED_AT_26k7Hz);

  /* Set FIFO mode to Stream mode (aka Continuous Mode) */
  iis3dwb_fifo_mode_set(&dev_ctx, IIS3DWB_STREAM_MODE);

  /* Set Output Data Rate */
  iis3dwb_xl_data_rate_set(&dev_ctx, IIS3DWB_XL_ODR_26k7Hz);
  iis3dwb_fifo_timestamp_batch_set(&dev_ctx, IIS3DWB_DEC_8);
  iis3dwb_timestamp_set(&dev_ctx, PROPERTY_ENABLE);

  /* Wait samples */
  while (1) {
    uint16_t num = 0, k;
    /* Read watermark flag */
    iis3dwb_fifo_status_get(&dev_ctx, &fifo_status);

    if (fifo_status.fifo_th == 1) {


      num = fifo_status.fifo_level;
      /* read out all FIFO entries in a single read */
      iis3dwb_fifo_out_multi_raw_get(&dev_ctx, fifo_data, num);

      char filename[30];
      FILE* file;
      
      time_t now = time(NULL);
      struct tm* local = localtime(&now);
      int16_t id_vib = 0x56;
      int16_t id_time = 0x54;
      strftime(filename, sizeof(filename), "IIS_%d-%H-%M.bin", local);

      // Open file in append binary mode.
      file = fopen(filename, "ab");
      if(file == NULL) {
          printf("Error opening file.\n");
          continue;
      }

      for (k = 0; k < num; k++) {
        iis3dwb_fifo_out_raw_t *f_data;

        // /* print out first two and last two FIFO entries only */
        // if (k > 5 && k < num - 5)
        //   continue;

        f_data = &fifo_data[k];

        /* Read FIFO sensor value */
        datax = (int16_t *)&f_data->data[0];
        datay = (int16_t *)&f_data->data[2];
        dataz = (int16_t *)&f_data->data[4];
        ts = (int32_t *)&f_data->data[0];
        
        
        switch (f_data->tag >> 3) {
        case IIS3DWB_XL_TAG:
          // sprintf((char *)tx_buffer, "%d: ACC [mg]:\t%4.3f\t%4.3f\t%4.3f\r\n",
          //         k,
          //         iis3dwb_from_fs8g_to_mg(*datax),
          //         iis3dwb_from_fs8g_to_mg(*datay),
          //         iis3dwb_from_fs8g_to_mg(*dataz));
          // tx_com(tx_buffer, strlen((char const *)tx_buffer));

          fwrite(&id_vib, sizeof(int16_t), 1, file);
          fwrite(&datax, sizeof(int16_t), 1, file);
          fwrite(&datay, sizeof(int16_t), 1, file);
          fwrite(&dataz, sizeof(int16_t), 1, file);
          break;
        case IIS3DWB_TIMESTAMP_TAG:
          fwrite(&id_time, sizeof(int16_t), 1, file);
          fwrite(&ts, sizeof(int32_t), 1, file);   
          break;
        default:
          break;
        }
      }
      fclose(file);
      usleep(1000);
    }
  }
}


static void platform_init(void) {
  spi_fd = open(SPI_DEVICE, O_RDWR);


  uint8_t mode = SPI_MODE_3; // Depending on your device
  uint32_t speed = SPI_SPEED;

  ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
  ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
}

/* SPI Write */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len) {
  uint8_t buffer[len+1];
  struct spi_ioc_transfer spi;

  buffer[0] = reg;
  memcpy(buffer+1, bufp, len);

  memset(&spi, 0, sizeof(spi));
  spi.tx_buf = (unsigned long)buffer;
  spi.rx_buf = (unsigned long)NULL;
  spi.len = len + 1;

  if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &spi) < 1) {
    perror("Failed to write to SPI device");
    return -1;
  }

  return 0;
}

/* SPI Read */
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
  uint8_t buffer[len+1];
  struct spi_ioc_transfer spi;

  buffer[0] = reg | 0x80;;

  memset(&spi, 0, sizeof(spi));
  spi.tx_buf = (unsigned long)buffer;
  spi.rx_buf = (unsigned long)buffer;
  spi.len = len + 1;

  if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &spi) < 1) {
    perror("Failed to read from SPI device");
    return -1;
  }

  memcpy(bufp, buffer+1, len);

  return 0;
}

/* Console print */
static void tx_com(uint8_t *tx_buffer, uint16_t len) {
  // Simply write buffer to stdout
  fwrite(tx_buffer, sizeof(uint8_t), len, stdout);
}
