#pragma once

// Pin and ADC channel definitions
#define cl_analog ADC1_CHANNEL_0
#define bl_analog ADC1_CHANNEL_3
#define piste_analog ADC1_CHANNEL_6
#define cr_analog ADC1_CHANNEL_7
#define br_analog ADC1_CHANNEL_4

#define al_driver 33
#define bl_driver 21
#define cl_driver 23
#define ar_driver 25
#define br_driver 05
#define cr_driver 18
#define piste_driver 19

// I/O Direction and Value macros
#define IODirection_ar_br 231
#define IODirection_ar_cr 215
#define IODirection_ar_piste 183
#define IODirection_ar_bl 245
#define IODirection_ar_cl 243
#define IODirection_al_br 238
#define IODirection_al_cr 222
#define IODirection_al_piste 190
#define IODirection_al_bl 252
#define IODirection_al_cl 250
#define IODirection_br_cr 207
#define IODirection_br_bl 237
#define IODirection_br_cl 235
#define IODirection_br_piste 175
#define IODirection_bl_cl 249
#define IODirection_bl_piste 189
#define IODirection_bl_cr 221
#define IODirection_cr_piste 159
#define IODirection_cr_cl 219
#define IODirection_cl_piste 187
#define IODirection_cr_bl 221

#define IOValues_ar_br 8
#define IOValues_ar_cr 8
#define IOValues_ar_piste 8
#define IOValues_ar_bl 8
#define IOValues_ar_cl 8
#define IOValues_al_br 1
#define IOValues_al_cr 1
#define IOValues_al_piste 1
#define IOValues_al_bl 1
#define IOValues_al_cl 1
#define IOValues_br_cr 16
#define IOValues_br_bl 16
#define IOValues_br_cl 16
#define IOValues_br_piste 16
#define IOValues_bl_cl 2
#define IOValues_bl_piste 2
#define IOValues_bl_cr 2
#define IOValues_cr_piste 32
#define IOValues_cr_cl 32
#define IOValues_cl_piste 4
#define IOValues_cr_bl 32