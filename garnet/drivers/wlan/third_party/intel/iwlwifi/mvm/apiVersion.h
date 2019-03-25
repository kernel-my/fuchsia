/******************************************************************************
 *
 * Copyright(c) 2012 - 2014 Intel Corporation. All rights reserved.
 * Copyright(c) 2013 - 2015 Intel Mobile Communications GmbH
 * Copyright(c) 2016 - 2017 Intel Deutschland GmbH
 * Copyright(c) 2018        Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
#ifndef _API_VERSION_H_
#define _API_VERSION_H_

// Do some macro trickery so that structs that aren't really supported for a specific product
// will get a version number of 99. This is needed for the COMMANDS_VERSIONS_LIST TLV.
#define NOT_EMPTY(ver) ver
#define ITS_EMPTY(ver) 99
#define SECOND(a, b, ...) b
#define IS_PROBE(args...) SECOND(args, NOT_EMPTY)
#define PROBE() ~, ITS_EMPTY
#define _EMPTY_ PROBE()
#define SELF_IF_NOT_EMPTY(ver) IS_PROBE(_EMPTY_ ## ver)(ver)
#define __API_VERSION(name, ver) name ## _VER_ ## ver
#define _API_VERSION(name, ver) __API_VERSION(name, ver)
#define API_VERSION(name, ver) _API_VERSION(name, SELF_IF_NOT_EMPTY(ver))


#define CHANNEL_WIDTH_MSK_API_D                                API_VERSION(CHANNEL_WIDTH_MSK_API_D, 2)

#define RATE_MCS_DUP_MSK_API_D                                 API_VERSION(RATE_MCS_DUP_MSK_API_D, 2)

#define RATE_MCS_DUP_POS_API_D                                 API_VERSION(RATE_MCS_DUP_POS_API_D, 2)

#define RATE_MCS_FAT_MSK_API_D                                 API_VERSION(RATE_MCS_FAT_MSK_API_D, 2)

#define MAX_SUPP_MCS_API_D                                     API_VERSION(MAX_SUPP_MCS_API_D, 1)

#define MAX_LEGAL_MCS_API_D                                    API_VERSION(MAX_LEGAL_MCS_API_D, 1)

#define MAX_TX_MCS_API_D                                       API_VERSION(MAX_TX_MCS_API_D, 1)

#define POWER_TABLE_NUM_CCK_ENTRIES_API_D                      API_VERSION(POWER_TABLE_NUM_CCK_ENTRIES_API_D, 1)

#define POWER_TABLE_NUM_HT_OFDM_ENTRIES_API_D                  API_VERSION(POWER_TABLE_NUM_HT_OFDM_ENTRIES_API_D, 1)

#define POWER_TABLE_TOTAL_ENTRIES_API_D                        API_VERSION(POWER_TABLE_TOTAL_ENTRIES_API_D, 1)

#define MCS_API_E                                              API_VERSION(MCS_API_E, 1)

#define RATE_MCS_BITS_API_S                                    API_VERSION(RATE_MCS_BITS_API_S, 3)

#define RATE_MCS_API_S                                         API_VERSION(RATE_MCS_API_S, 1)

#define RATE_MCS_API_U                                         API_VERSION(RATE_MCS_API_U, 1)

#define IS_RATE_CCK_API_M                                      API_VERSION(IS_RATE_CCK_API_M, 3)

#define IS_RATE_OFDM_API_M                                     API_VERSION(IS_RATE_OFDM_API_M, 2)

#define IS_RATE_OFDM_LEGACY_API_M                              API_VERSION(IS_RATE_OFDM_LEGACY_API_M, 2)

#define IS_RATE_OFDM_HT_API_M                                  API_VERSION(IS_RATE_OFDM_HT_API_M, 2)

#define IS_RATE_OFDM_VHT_API_M                                 API_VERSION(IS_RATE_OFDM_VHT_API_M, 3)

#define IS_RATE_OFDM_HT_VHT_API_M                              API_VERSION(IS_RATE_OFDM_HT_VHT_API_M, 3)

#define GET_MIMO_INDEX_API_M                                   API_VERSION(GET_MIMO_INDEX_API_M, 3)

#define IS_RATE_OFDM_HT_MIMO_API_M                             API_VERSION(IS_RATE_OFDM_HT_MIMO_API_M, 2)

#define IS_RATE_OFDM_HT_FAT_API_M                              API_VERSION(IS_RATE_OFDM_HT_FAT_API_M, 2)

#define IS_BAD_OFDM_HT_RATE_API_M                              API_VERSION(IS_BAD_OFDM_HT_RATE_API_M, 3)

#define GET_ANT_CHAIN_API_M                                    API_VERSION(GET_ANT_CHAIN_API_M, 1)

#define GET_ANT_CHAIN_NUM_API_M                                API_VERSION(GET_ANT_CHAIN_NUM_API_M, 1)

#define IS_RATE_STBC_PRESENT_API_M                             API_VERSION(IS_RATE_STBC_PRESENT_API_M, 1)

#define GET_NUM_OF_STBC_SS_API_M                               API_VERSION(GET_NUM_OF_STBC_SS_API_M, 1)

#define GET_BW_INDEX_API_M                                     API_VERSION(GET_BW_INDEX_API_M, 2)

#define GET_DUP_INDEX_API_M                                    API_VERSION(GET_DUP_INDEX_API_M, 2)

#define GET_CHANNEL_WIDTH_INDEX_API_M                          API_VERSION(GET_CHANNEL_WIDTH_INDEX_API_M, 2)

#define GET_GI_INDEX_API_M                                     API_VERSION(GET_GI_INDEX_API_M, 1)

#define GET_HT_MIMO_INDEX_API_M                                API_VERSION(GET_HT_MIMO_INDEX_API_M, 1)

#define GET_NUM_OF_HT_SS_API_M                                 API_VERSION(GET_NUM_OF_HT_SS_API_M, 1)

#define IS_RATE_OFDM_HT2x2MIMO_API_M                           API_VERSION(IS_RATE_OFDM_HT2x2MIMO_API_M, 1)

#define IS_RATE_OFDM_HT3x3MIMO_API_M                           API_VERSION(IS_RATE_OFDM_HT3x3MIMO_API_M, 1)

#define IS_RATE_OFDM_HT4x4MIMO_API_M                           API_VERSION(IS_RATE_OFDM_HT4x4MIMO_API_M, 1)

#define GET_HT_RATE_CODE_API_M                                 API_VERSION(GET_HT_RATE_CODE_API_M, 1)

#define IS_RATE_HT_STBC_SINGLE_SS_API_M                        API_VERSION(IS_RATE_HT_STBC_SINGLE_SS_API_M, 1)

#define GET_NUM_OF_HT_EXT_LTF_API_M                            API_VERSION(GET_NUM_OF_HT_EXT_LTF_API_M, 1)

#define GET_NUM_OF_HT_SPACE_TIME_STREAMS_API_M                 API_VERSION(GET_NUM_OF_HT_SPACE_TIME_STREAMS_API_M, 1)

#define IS_RATE_HT_HIGH_RATE_API_M                             API_VERSION(IS_RATE_HT_HIGH_RATE_API_M, 1)

#define GET_VHT_MIMO_INDX_API_M                                API_VERSION(GET_VHT_MIMO_INDX_API_M, 1)

#define GET_NUM_OF_VHT_SS_API_M                                API_VERSION(GET_NUM_OF_VHT_SS_API_M, 1)

#define GET_VHT_RATE_CODE_API_M                                API_VERSION(GET_VHT_RATE_CODE_API_M, 1)

#define GET_HT_VHT_RATE_CODE_API_M                             API_VERSION(GET_HT_VHT_RATE_CODE_API_M, 1)

#define IS_VHT_STBC_PRESENT_API_M                              API_VERSION(IS_VHT_STBC_PRESENT_API_M, 1)

#define GET_NUM_OF_VHT_SPACE_TIME_STREAMS_API_M                API_VERSION(GET_NUM_OF_VHT_SPACE_TIME_STREAMS_API_M, 1)

#define IS_RATE_OFDM_LDPC_API_M                                API_VERSION(IS_RATE_OFDM_LDPC_API_M, 2)

#define GET_BF_INDEX_API_M                                     API_VERSION(GET_BF_INDEX_API_M, 1)

#define IS_RATE_VHT_MU_API_M                                   API_VERSION(IS_RATE_VHT_MU_API_M, 1)


#define TLC_MNG_CONFIG_PARAMS_CMD_API_S                        API_VERSION(TLC_MNG_CONFIG_PARAMS_CMD_API_S, 2)

#define TLC_MNG_CONFIG_CMD_API_S                               API_VERSION(TLC_MNG_CONFIG_CMD_API_S, 2)

#endif // _API_VERSION_H_
