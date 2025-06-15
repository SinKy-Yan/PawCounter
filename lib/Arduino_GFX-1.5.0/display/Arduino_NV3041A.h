#ifndef _ARDUINO_NV3041A_H_
#define _ARDUINO_NV3041A_H_

#include "../Arduino_GFX.h"
#include "../Arduino_TFT.h"

#define NV3041A_TFTWIDTH 480
#define NV3041A_TFTHEIGHT 272

#define NV3041A_RST_DELAY 120    ///< delay ms wait for reset finish
#define NV3041A_SLPIN_DELAY 120  ///< delay ms wait for sleep in finish
#define NV3041A_SLPOUT_DELAY 120 ///< delay ms wait for sleep out finish

#define NV3041A_NOP 0x00
#define NV3041A_SWRESET 0x01

#define NV3041A_SLPIN 0x10
#define NV3041A_SLPOUT 0x11

#define NV3041A_INVOFF 0x20
#define NV3041A_INVON 0x21
#define NV3041A_DISPOFF 0x28
#define NV3041A_DISPON 0x29

#define NV3041A_CASET 0x2A
#define NV3041A_RASET 0x2B
#define NV3041A_RAMWR 0x2C

#define NV3041A_MADCTL 0x36
#define NV3041A_COLMOD 0x3A

#define NV3041A_MADCTL_MY 0x80
#define NV3041A_MADCTL_MX 0x40
#define NV3041A_MADCTL_MV 0x20
#define NV3041A_MADCTL_ML 0x10
#define NV3041A_MADCTL_RGB 0x00

static const uint8_t nv3041a_init_operations[] = {
    BEGIN_WRITE,

    WRITE_C8_D8, 0xff, 0xa5,
    WRITE_C8_D8, 0xE7, 0x10,
    WRITE_C8_D8, 0x35, 0x01,
    WRITE_C8_D8, 0x3A, 0x01,  // 00---666, 01--565
    WRITE_C8_D8, 0x40, 0x01,  // 01:IPS, 00:TN
    WRITE_C8_D8, 0x41, 0x01,  // 01--8bit, 03--16bit
    WRITE_C8_D8, 0x55, 0x01,
    WRITE_C8_D8, 0x44, 0x15,  // VBP
    WRITE_C8_D8, 0x45, 0x15,  // VFP
    WRITE_C8_D8, 0x7d, 0x03,  // vdds_trim[2:0]

    WRITE_C8_D8, 0xc1, 0xab,  // avdd_clp_en avdd_clp[1:0] avcl_clp_en avcl_clp[1:0]
    WRITE_C8_D8, 0xc2, 0x17,
    WRITE_C8_D8, 0xc3, 0x10,
    WRITE_C8_D8, 0xc6, 0x3a,
    WRITE_C8_D8, 0xc7, 0x25,
    WRITE_C8_D8, 0xc8, 0x11,
    WRITE_C8_D8, 0x7a, 0x49,
    WRITE_C8_D8, 0x6f, 0x2f,
    WRITE_C8_D8, 0x78, 0x4b,
    WRITE_C8_D8, 0x73, 0x08,
    WRITE_C8_D8, 0x74, 0x12,
    WRITE_C8_D8, 0xc9, 0x00,
    WRITE_C8_D8, 0x67, 0x11,

    // gate_ed
    WRITE_C8_D8, 0x51, 0x20,
    WRITE_C8_D8, 0x52, 0x7c,
    WRITE_C8_D8, 0x53, 0x1c,
    WRITE_C8_D8, 0x54, 0x77,

    // source
    WRITE_C8_D8, 0x46, 0x0a,
    WRITE_C8_D8, 0x47, 0x2a,
    WRITE_C8_D8, 0x48, 0x0a,
    WRITE_C8_D8, 0x49, 0x1a,
    WRITE_C8_D8, 0x56, 0x43,
    WRITE_C8_D8, 0x57, 0x42,
    WRITE_C8_D8, 0x58, 0x3c,
    WRITE_C8_D8, 0x59, 0x64,
    WRITE_C8_D8, 0x5a, 0x41,
    WRITE_C8_D8, 0x5b, 0x3c,
    WRITE_C8_D8, 0x5c, 0x02,
    WRITE_C8_D8, 0x5d, 0x3c,
    WRITE_C8_D8, 0x5e, 0x1f,
    WRITE_C8_D8, 0x60, 0x80,
    WRITE_C8_D8, 0x61, 0x3f,
    WRITE_C8_D8, 0x62, 0x21,
    WRITE_C8_D8, 0x63, 0x07,
    WRITE_C8_D8, 0x64, 0xe0,
    WRITE_C8_D8, 0x65, 0x01,

    WRITE_C8_D8, 0xca, 0x20,
    WRITE_C8_D8, 0xcb, 0x52,
    WRITE_C8_D8, 0xcc, 0x10,
    WRITE_C8_D8, 0xcD, 0x42,
    WRITE_C8_D8, 0xD0, 0x20,
    WRITE_C8_D8, 0xD1, 0x52,
    WRITE_C8_D8, 0xD2, 0x10,
    WRITE_C8_D8, 0xD3, 0x42,
    WRITE_C8_D8, 0xD4, 0x0a,
    WRITE_C8_D8, 0xD5, 0x32,
    WRITE_C8_D8, 0xe5, 0x06,
    WRITE_C8_D8, 0xe6, 0x00,
    WRITE_C8_D8, 0x6e, 0x14,

    // gamma
    WRITE_C8_D8, 0x80, 0x04,
    WRITE_C8_D8, 0xA0, 0x00,
    WRITE_C8_D8, 0x81, 0x07,
    WRITE_C8_D8, 0xA1, 0x05,
    WRITE_C8_D8, 0x82, 0x06,
    WRITE_C8_D8, 0xA2, 0x04,
    WRITE_C8_D8, 0x83, 0x39,
    WRITE_C8_D8, 0xA3, 0x39,
    WRITE_C8_D8, 0x84, 0x3a,
    WRITE_C8_D8, 0xA4, 0x3a,
    WRITE_C8_D8, 0x85, 0x3f,
    WRITE_C8_D8, 0xA5, 0x3f,
    WRITE_C8_D8, 0x86, 0x2c,
    WRITE_C8_D8, 0xA6, 0x2a,
    WRITE_C8_D8, 0x87, 0x43,
    WRITE_C8_D8, 0xA7, 0x47,
    WRITE_C8_D8, 0x88, 0x08,
    WRITE_C8_D8, 0xA8, 0x08,
    WRITE_C8_D8, 0x89, 0x0f,
    WRITE_C8_D8, 0xA9, 0x0f,
    WRITE_C8_D8, 0x8a, 0x17,
    WRITE_C8_D8, 0xAa, 0x17,
    WRITE_C8_D8, 0x8b, 0x10,
    WRITE_C8_D8, 0xAb, 0x10,
    WRITE_C8_D8, 0x8c, 0x16,
    WRITE_C8_D8, 0xAc, 0x16,
    WRITE_C8_D8, 0x8d, 0x14,
    WRITE_C8_D8, 0xAd, 0x14,
    WRITE_C8_D8, 0x8e, 0x11,
    WRITE_C8_D8, 0xAe, 0x11,
    WRITE_C8_D8, 0x8f, 0x14,
    WRITE_C8_D8, 0xAf, 0x14,
    WRITE_C8_D8, 0x90, 0x06,
    WRITE_C8_D8, 0xB0, 0x06,
    WRITE_C8_D8, 0x91, 0x0f,
    WRITE_C8_D8, 0xB1, 0x0f,
    WRITE_C8_D8, 0x92, 0x16,
    WRITE_C8_D8, 0xB2, 0x16,

    WRITE_C8_D8, 0xff, 0x00,

    WRITE_C8_D8, 0x11, 0x00,
    END_WRITE,

    DELAY, 200,

    BEGIN_WRITE,
    WRITE_C8_D8, 0x29, 0x00,
    END_WRITE,

    DELAY, 120
};

class Arduino_NV3041A : public Arduino_TFT
{
public:
  Arduino_NV3041A(
      Arduino_DataBus *bus, int8_t rst = GFX_NOT_DEFINED, uint8_t r = 0,
      bool ips = false, int16_t w = NV3041A_TFTWIDTH, int16_t h = NV3041A_TFTHEIGHT,
      uint8_t col_offset1 = 0, uint8_t row_offset1 = 0, uint8_t col_offset2 = 0, uint8_t row_offset2 = 0);

  bool begin(int32_t speed = GFX_NOT_DEFINED) override;

  void setRotation(uint8_t r) override;

  void writeAddrWindow(int16_t x, int16_t y, uint16_t w, uint16_t h) override;

  void invertDisplay(bool) override;
  void displayOn() override;
  void displayOff() override;

protected:
  void tftInit() override;

private:
};

#endif
