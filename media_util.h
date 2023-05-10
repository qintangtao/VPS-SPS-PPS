/***************************************************************************************
 *
 *  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
 *
 *  By downloading, copying, installing or using the software you agree to this license.
 *  If you do not agree to this license, do not download, install, 
 *  copy or use the software.
 *
 *  Copyright (C) 2010-2018, Happytimesoft Corporation, all rights reserved.
 *
 *  Redistribution and use in binary forms, with or without modification, are permitted.
 *
 *  Unless required by applicable law or agreed to in writing, software distributed 
 *  under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 *  CONDITIONS OF ANY KIND, either express or implied. See the License for the specific
 *  language governing permissions and limitations under the License.
 *
****************************************************************************************/

#ifndef MEDIA_UTIL_H
#define MEDIA_UTIL_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t  remove_emulation_bytes(uint8_t* to, uint32_t toMaxSize, uint8_t* from, uint32_t fromSize);
uint8_t * avc_find_startcode(uint8_t *p, uint8_t *end);

#ifdef __cplusplus
}
#endif

#endif



