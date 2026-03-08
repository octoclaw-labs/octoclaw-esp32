# OctoClaw 全板型档位矩阵（自动生成）

> 本文件由 `scripts/gen_board_tier_matrix.py` 生成，请勿手工编辑。

- 板型总数：**131**
- 档位规则：Lite=ESP32/C3/C5/C6，Standard=ESP32S3，Pro=ESP32P4

## 芯片分布统计

| 芯片 | 数量 |
|---|---:|
| ESP32 | 6 |
| ESP32C3 | 9 |
| ESP32C5 | 3 |
| ESP32C6 | 6 |
| ESP32P4 | 12 |
| ESP32S3 | 95 |

## 全板型清单

| BOARD_TYPE 符号 | 芯片 | 档位 | 默认 FeatureMask |
|---|---|---|---|
| `BOARD_TYPE_BREAD_COMPACT_WIFI` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_BREAD_COMPACT_WIFI_LCD` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_BREAD_COMPACT_WIFI_CAM` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_BREAD_COMPACT_ML307` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_BREAD_COMPACT_NT26` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_BREAD_COMPACT_ESP32` | ESP32 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_BREAD_COMPACT_ESP32_LCD` | ESP32 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_XMINI_C3_V3` | ESP32C3 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_XMINI_C3_4G` | ESP32C3 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_XMINI_C3` | ESP32C3 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_ESP_KORVO2_V3` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ESP_KORVO2_V3_RNDIS` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ESP_SPARKBOT` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ESP_SENSAIRSHUTTLE` | ESP32C5 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_ESP_SPOT_S3` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ESP_SPOT_C5` | ESP32C5 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_ESP_HI` | ESP32C3 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_ESP_BOX_3` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ESP_BOX` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ESP_BOX_LITE` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ESP_P4_FUNCTION_EV_BOARD` | ESP32P4 | pro | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields | kFeatureEspNowAgent` |
| `BOARD_TYPE_ECHOEAR` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_KEVIN_BOX_2` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_KEVIN_C3` | ESP32C3 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_KEVIN_SP_V3_DEV` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_KEVIN_SP_V4_DEV` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_KEVIN_YUYING_313LCD` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_CGC` | ESP32 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_CGC_144` | ESP32 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_LICHUANG_DEV_S3` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_LICHUANG_DEV_C3` | ESP32C3 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_EDA_TV_PRO` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_EDA_ROBOT_PRO` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_EDA_SUPER_BEAR` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_DF_K10` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_DF_S3_AI_CAM` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_MAGICLICK_S3_2P4` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_MAGICLICK_S3_2P5` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_MAGICLICK_C3` | ESP32C3 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_MAGICLICK_C3_V2` | ESP32C3 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_M5STACK_CORE_S3` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_M5STACK_CORE_TAB5` | ESP32P4 | pro | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields | kFeatureEspNowAgent` |
| `BOARD_TYPE_M5STACK_ATOM_S3_ECHO_BASE` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_M5STACK_ATOM_S3R_ECHO_BASE` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_M5STACK_ATOM_S3R_CAM_M12_ECHO_BASE` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_M5STACK_ATOM_ECHOS3R` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_M5STACK_CARDPUTER_ADV` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_M5STACK_ATOM_MATRIX_ECHO_BASE` | ESP32 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_TOUCH_LCD_3_5` | ESP32 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_S3_AUDIO_BOARD` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_S3_TOUCH_AMOLED_1_8` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_S3_TOUCH_AMOLED_2_06` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_C6_TOUCH_AMOLED_2_06` | ESP32C6 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_S3_TOUCH_AMOLED_1_75` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_S3_TOUCH_AMOLED_1_75C` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_S3_TOUCH_LCD_1_83` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_S3_TOUCH_LCD_4B` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_S3_TOUCH_LCD_4_3C` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_S3_TOUCH_LCD_1_85C` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_S3_TOUCH_LCD_1_85` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_S3_TOUCH_LCD_1_46` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_C6_LCD_1_69` | ESP32C6 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_C6_TOUCH_LCD_1_83` | ESP32C6 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_C6_TOUCH_AMOLED_1_43` | ESP32C6 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_C6_TOUCH_AMOLED_1_32` | ESP32C6 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_C6_TOUCH_AMOLED_1_8` | ESP32C6 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_S3_TOUCH_AMOLED_1_32` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_S3_TOUCH_LCD_3_49` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_S3_TOUCH_LCD_3_5` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_S3_ePaper_1_54_v1` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_S3_ePaper_1_54_v2` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_S3_RLCD_4_2` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_S3_TOUCH_LCD_3_5B` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_WAVESHARE_ESP32_P4_NANO` | ESP32P4 | pro | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields | kFeatureEspNowAgent` |
| `BOARD_TYPE_WAVESHARE_ESP32_P4_WIFI6_TOUCH_LCD_4B` | ESP32P4 | pro | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields | kFeatureEspNowAgent` |
| `BOARD_TYPE_WAVESHARE_ESP32_P4_WIFI6_TOUCH_LCD_7B` | ESP32P4 | pro | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields | kFeatureEspNowAgent` |
| `BOARD_TYPE_WAVESHARE_ESP32_P4_WIFI6_TOUCH_LCD_3_4C` | ESP32P4 | pro | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields | kFeatureEspNowAgent` |
| `BOARD_TYPE_WAVESHARE_ESP32_P4_WIFI6_TOUCH_LCD_4C` | ESP32P4 | pro | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields | kFeatureEspNowAgent` |
| `BOARD_TYPE_WAVESHARE_ESP32_P4_WIFI6_TOUCH_LCD_7` | ESP32P4 | pro | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields | kFeatureEspNowAgent` |
| `BOARD_TYPE_WAVESHARE_ESP32_P4_WIFI6_TOUCH_LCD_8` | ESP32P4 | pro | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields | kFeatureEspNowAgent` |
| `BOARD_TYPE_WAVESHARE_ESP32_P4_WIFI6_TOUCH_LCD_10_1` | ESP32P4 | pro | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields | kFeatureEspNowAgent` |
| `BOARD_TYPE_TUDOUZI` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_LILYGO_T_CIRCLE_S3` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_LILYGO_T_CAMERAPLUS_S3_V1_0_V1_1` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_LILYGO_T_CAMERAPLUS_S3_V1_2` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_LILYGO_T_DISPLAY_S3_PRO_MVSRLORA` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_LILYGO_T_DISPLAY_S3_PRO_MVSRLORA_NO_BATTERY` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_LILYGO_T_DISPLAY_P4` | ESP32P4 | pro | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields | kFeatureEspNowAgent` |
| `BOARD_TYPE_MOVECALL_MOJI_ESP32S3` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_MOVECALL_MOJI2_ESP32C5` | ESP32C5 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_MOVECALL_CUICAN_ESP32S3` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ATK_DNESP32S3` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ATK_DNESP32S3_BOX` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ATK_DNESP32S3_BOX0` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ATK_DNESP32S3_BOX2_WIFI` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ATK_DNESP32S3_BOX2_4G` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ATK_DNESP32S3M_WIFI` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ATK_DNESP32S3M_4G` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_DU_CHATX` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_TAIJI_PI_S3` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_XINGZHI_CUBE_0_85TFT_WIFI` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_XINGZHI_CUBE_0_85TFT_ML307` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_XINGZHI_CUBE_0_96OLED_WIFI` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_XINGZHI_CUBE_0_96OLED_ML307` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_XINGZHI_CUBE_1_54TFT_WIFI` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_XINGZHI_CUBE_1_54TFT_ML307` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_XINGZHI_METAL_1_54_WIFI` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_XINGZHI_ABS_2_0` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_SEEED_STUDIO_SENSECAP_WATCHER` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_DOIT_S3_AIBOX` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_MIXGO_NOVA` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_GENJUTECH_S3_1_54TFT` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ESP_S3_LCD_EV_Board` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ESP_S3_LCD_EV_Board_2` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ZHENGCHEN_1_54TFT_WIFI` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ZHENGCHEN_1_54TFT_ML307` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ZHENGCHEN_CAM` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ZHENGCHEN_CAM_ML307` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_MINSI_K08_DUAL` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_SPOTPEAR_ESP32_S3_1_54_MUMA` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_SPOTPEAR_ESP32_S3_1_28_BOX` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_OTTO_ROBOT` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_ELECTRON_BOT` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_JIUCHUAN` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_LABPLUS_MPYTHON_V3` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_LABPLUS_LEDONG_V2` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_SURFER_C3_1_14TFT` | ESP32C3 | lite | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureMcpExtFields` |
| `BOARD_TYPE_YUNLIAO_S3` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_WIRELESS_TAG_WTP4C5MP07S` | ESP32P4 | pro | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields | kFeatureEspNowAgent` |
| `BOARD_TYPE_AIPI_LITE` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
| `BOARD_TYPE_HU_087` | ESP32S3 | standard | `kFeaturePolicyGuard | kFeatureReceiptCompensation | kFeatureLocalAgent | kFeatureMcpExtFields` |
