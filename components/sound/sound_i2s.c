// https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/i2s.html
// https://github.com/espressif/esp-idf/tree/v6.0.2/examples/peripherals/i2s

#include <string.h> // memset

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"

#include "hw.h"
#include "sound.h"

//#include "i2s_private.h" // hidden i2s_chan_handle_t members
// see: /opt/esp6/esp-idf/components/esp_driver_i2c/i2c_private.h

// dma_buffer_size = dma_frame_num * slot_num * slot_bit_width / 8
// A 2048 byte sized DMA descriptor will trigger an "Interrupt wdt timeout"
#define DMA_DESC_SZ 128 // DMA descriptor (buffer) size in bytes
#define DMA_DESC_NUM 8 // Number of DMA descriptors (buffers)
#define DMA_DEBUG 0 // Turn on DMA debug prints

#define I2S_FRAME_SZ sizeof(slot_t) // Frame size in bytes (16-bit mono)
#define I2S_FRAME_NUM (DMA_DESC_SZ/I2S_FRAME_SZ) // Number of frames in a DMA descriptor

// Make audio buffer extern for testing
#ifdef EXTERN_BUF
#define scope
#else
#define scope static
#endif

#define SOUND_VOLUME_DEFAULT 10
#define POLL_DELAY 10
#define PERCENT 100
#define IN_BIAS 0x80U

static const char *TAG = "sound";

typedef int16_t sample_t;
typedef int16_t slot_t;

// Critical section protected variables
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
scope  volatile const sample_t *abase;
scope  volatile uint32_t asize;
static volatile uint32_t aidx;
static volatile bool     cyclic;
static volatile uint32_t dcnt;

// Other global variables
static i2s_chan_handle_t i2s_handle;
static volatile bool device_en;
static volatile int32_t volume;

#if DMA_DEBUG
static volatile size_t missed_dat, missed_sil;
#endif

// NOTE:
// 1) The function i2s_channel_enable() must be called each time a new
//    sound is played from silence, otherwise the DMA buffers somehow get
//    messed up and garbled sound will be heard.
// 2) After a call to i2s_channel_write() in the I2S callback, sometimes
//    the bytes_written will be less than the bytes requested for both active
//    data and silence. It is not known where in the playback this occurs.
// 3) When starting to play a sound, preloading one DMA descriptor worth of
//    data with a call to i2s_channel_preload_data() seems to eliminate the
//    discrepancy between bytes requested and written in the I2S callback.
//    However, more than one DMA buffer needs to be preloaded before
//    playback to prevent a garbled sound.
// 4) The function i2s_channel_write() should not normally be called within
//    an interrupt context since it can block. However, it is known that at
//    least one DMA buffer is available to fill when the i2s done callback
//    occurs. Thus by limiting the data written in the interrupt handler to
//    one buffer, the function appears to not block.
// 5) The buffer of audio samples provided to i2s_channel_preload_data() and
//    i2s_channel_write() is copied directly to the DMA ring buffer. It is
//    not expanded from mono frames to stereo frames.


// Called when a TX channel finishes sending a DMA buffer.
// The event data includes the DMA buffer address and size (not used here).
static bool IRAM_ATTR i2s_done_callback(i2s_chan_handle_t handle, i2s_event_data_t *event, void *user_ctx)
{
	slot_t buf[I2S_FRAME_NUM];
	#if DMA_DEBUG
	size_t bytes_written;
	#endif
	portENTER_CRITICAL_ISR(&spinlock);
	if (aidx < asize) {
		uint32_t idx = aidx;
		uint32_t size = (!cyclic && asize < I2S_FRAME_NUM) ? asize : I2S_FRAME_NUM;
		aidx =  cyclic ? (aidx + size) % asize : aidx + size;
		portEXIT_CRITICAL_ISR(&spinlock);
		int i;
		for (i = 0; i < size; i++) buf[i] = (abase[(idx+i)%asize])*volume/PERCENT;
		while (i < I2S_FRAME_NUM) buf[i++] = 0; // pad to block size with silence
		#if DMA_DEBUG
		i2s_channel_write(handle, buf, DMA_DESC_SZ, /*NULL*/ &bytes_written, 0);
		if (bytes_written != DMA_DESC_SZ) missed_dat += DMA_DESC_SZ-bytes_written;
		#else
		i2s_channel_write(handle, buf, DMA_DESC_SZ, NULL /*&bytes_written*/, 0);
		#endif
	} else if (dcnt) {
		dcnt--; // add silence to DMA buffer when done
		portEXIT_CRITICAL_ISR(&spinlock);
		memset(buf, 0, sizeof(buf));
		#if DMA_DEBUG
		i2s_channel_write(handle, buf, DMA_DESC_SZ, /*NULL*/ &bytes_written, 0);
		if (bytes_written != DMA_DESC_SZ) missed_sil += DMA_DESC_SZ-bytes_written;
		#else
		i2s_channel_write(handle, buf, DMA_DESC_SZ, NULL /*&bytes_written*/, 0);
		#endif
		if (!dcnt) i2s_channel_disable(i2s_handle);
	} else {
		portEXIT_CRITICAL_ISR(&spinlock);
	}
	return false; // no high priority task awoken
}

// Preload audio data into a TX DMA buffer.
// Must be called before i2s_channel_enable().
static void i2s_tx_preload(i2s_chan_handle_t handle)
{
	slot_t buf[I2S_FRAME_NUM];
	size_t bytes_written;
	portENTER_CRITICAL_ISR(&spinlock);
	if (aidx < asize) {
		uint32_t idx = aidx;
		uint32_t size = (!cyclic && asize < I2S_FRAME_NUM) ? asize : I2S_FRAME_NUM;
		aidx =  cyclic ? (aidx + size) % asize : aidx + size;
		portEXIT_CRITICAL_ISR(&spinlock);
		int i;
		for (i = 0; i < size; i++) buf[i] = (abase[(idx+i)%asize])*volume/PERCENT;
		while (i < I2S_FRAME_NUM) buf[i++] = 0; // pad to block size with silence
		i2s_channel_preload_data(handle, buf, DMA_DESC_SZ, &bytes_written);
		#if DMA_DEBUG
		if (bytes_written != DMA_DESC_SZ) missed_dat += DMA_DESC_SZ-bytes_written;
		#endif
	} else if (dcnt) {
		dcnt--; // add silence to DMA buffer when done
		portEXIT_CRITICAL_ISR(&spinlock);
		memset(buf, 0, sizeof(buf));
		i2s_channel_preload_data(handle, buf, DMA_DESC_SZ, &bytes_written);
		#if DMA_DEBUG
		if (bytes_written != DMA_DESC_SZ) missed_sil += DMA_DESC_SZ-bytes_written;
		#endif
		if (!dcnt) i2s_channel_disable(i2s_handle);
	} else {
		portEXIT_CRITICAL_ISR(&spinlock);
	}
}

// Initialize the sound driver. Must be called before using sound.
// May be called again to change sample rate.
// sample_hz: sample rate in Hz to playback audio.
// Return zero if successful, or non-zero otherwise.
int32_t sound_init(uint32_t sample_hz)
{
	sound_set_volume(SOUND_VOLUME_DEFAULT);

	/* * * * * * * * * * Sound Config * * * * * * * * * */
	ESP_LOGI(TAG, "Configure I2S channel in Standard mode, Philips format, %lu Hz", sample_hz);
	i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
	chan_cfg.dma_desc_num = DMA_DESC_NUM;
	chan_cfg.dma_frame_num = I2S_FRAME_NUM;
	chan_cfg.auto_clear = false;
	ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &i2s_handle, NULL));
	i2s_std_config_t std_cfg = {
		.clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(sample_hz),
		.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
		.gpio_cfg = {
			.mclk = I2S_GPIO_UNUSED,
			.bclk = HW_I2S_BCLK,
			.ws   = HW_I2S_WS,
			.dout = HW_I2S_DOUT,
			.din  = I2S_GPIO_UNUSED,
			.invert_flags = {
				.mclk_inv = false,
				.bclk_inv = false,
				.ws_inv   = false,
			},
		},
	};
	ESP_ERROR_CHECK(i2s_channel_init_std_mode(i2s_handle, &std_cfg));

	// Register the callback for asynchronous writing
	i2s_event_callbacks_t cbs = {
		.on_recv = NULL,
		.on_recv_q_ovf = NULL,
		.on_sent = i2s_done_callback,
		.on_send_q_ovf = NULL,
	};
	ESP_ERROR_CHECK(i2s_channel_register_event_callback(i2s_handle, &cbs, NULL));

	return 0;
}

// Free resources used for sound (DAC, etc.).
// Return zero if successful, or non-zero otherwise.
int32_t sound_deinit(void)
{
	ESP_LOGI(TAG, "Stop I2S channel");
	while (aidx < asize || dcnt); // busy
	ESP_ERROR_CHECK(i2s_del_channel(i2s_handle));
	return 0;
}

// Start playing the sound immediately. Play the audio buffer once.
// audio: a pointer to an array of unsigned audio data.
// size: the size of the array in bytes.
// wait: if true, block until done playing, otherwise return straight away.
void sound_start(const void *audio, uint32_t size, bool wait)
{
	bool enable;

	portENTER_CRITICAL(&spinlock);
	enable = !(aidx < asize || dcnt);
	abase = audio;
	asize = size/sizeof(abase[0]);
	aidx = 0;
	cyclic = false;
	dcnt = DMA_DESC_NUM;
	portEXIT_CRITICAL(&spinlock);
	if (enable) {
		i2s_tx_preload(i2s_handle); // preload two DMA buffers
		i2s_tx_preload(i2s_handle);
		#if DMA_DEBUG
		printf("missed0:%u,%u\n", missed_dat, missed_sil);
		#endif
		i2s_channel_enable(i2s_handle);
	}
	#if DMA_DEBUG
	printf("missed1:%u,%u\n", missed_dat, missed_sil);
	#endif
	while (wait && aidx < asize)
		vTaskDelay(pdMS_TO_TICKS(POLL_DELAY));
	#if DMA_DEBUG
	printf("missed2:%u,%u\n", missed_dat, missed_sil);
	#endif
}

// Cyclically play samples from audio buffer until sound_stop() is called.
// audio: a pointer to an array of unsigned audio data.
// size: the size of the array in bytes.
void sound_cyclic(const void *audio, uint32_t size)
{
	bool enable;

	portENTER_CRITICAL(&spinlock);
	enable = !(aidx < asize || dcnt);
	abase = audio;
	asize = size/sizeof(abase[0]);
	aidx = 0;
	cyclic = true;
	dcnt = DMA_DESC_NUM;
	portEXIT_CRITICAL(&spinlock);
	if (enable) {
		i2s_tx_preload(i2s_handle); // preload two DMA buffers
		i2s_tx_preload(i2s_handle);
		#if DMA_DEBUG
		printf("missed0:%u,%u\n", missed_dat, missed_sil);
		#endif
		i2s_channel_enable(i2s_handle);
	}
}

// Return true if sound playing, otherwise return false.
bool sound_busy(void)
{
	return aidx < asize;
}

// Stop playing the sound.
void sound_stop(void)
{
	portENTER_CRITICAL(&spinlock);
	aidx = asize;
	portEXIT_CRITICAL(&spinlock);
}

// Set the volume.
// volume: 0-100% as an integer value.
// https://www.dr-lex.be/info-stuff/volumecontrols.html
// https://en.wikipedia.org/wiki/Audio_bit_depth
void sound_set_volume(uint32_t vol)
{
	// TODO: use an exponential volume control.
	volume = vol;
}

// Enable or disable the sound output device.
// enable: if true, enable sound, otherwise disable.
void sound_device(bool enable)
{
	// TODO: implement sound_device() enable, disable.
	// ESP_ERROR_CHECK(i2s_channel_disable(i2s_handle)); // only if enabled
	// ESP_ERROR_CHECK(i2s_channel_enable(i2s_handle)); // only if disabled
}
