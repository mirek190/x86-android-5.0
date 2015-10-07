#ifndef CAMERACAL_OTP_DECODER_H_
#define CAMERACAL_OTP_DECODER_H_

#include <cstdint>

enum BayerOrder {
    BGGR = 0,
    GRBG = 1,
    RGGB = 2,
    GBRG = 3
};

struct NVM_input {
    uint8_t *nvm_data; /*!<nvm_data pointer to nvm data from sensor */
    int len; /*!<len nvm data length */
    int bayer_order;
    int width; /*!<width the decoded lsc table output width */
    int height; /*!<height the decoded lsc table output height */
    int table_num; /*!<table_num number of the tables in the NVM data */
};

struct Lsc_table
{
    int lsc_color_temperature;  /*!< color temperature of shading table calibrate. as ct/100 */
    uint16_t *lsc_tables[4]; /*!< NVM LSC table for Ch1, Ch2, Ch3 and Ch4. */
};

/** @brief DecoderNVMDataTable
 * @param[out] table pointer to lsc tables in some bayer order
 * @param[in] nvm struct of nvm data in google format
 * @return error status
 *
 * This function decodes the compressed NVM data from sensor.
 */
int DecoderNVMDataTable(NVM_input &nvm,  Lsc_table *table);

#endif  // CAMERACAL_OTP_DECODER_H_
