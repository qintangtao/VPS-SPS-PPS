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

#include "media_util.h"


uint32_t remove_emulation_bytes(uint8_t* to, uint32_t toMaxSize, uint8_t* from, uint32_t fromSize) 
{
	uint32_t toSize = 0;
	uint32_t i = 0;
	
	while (i < fromSize && toSize+1 < toMaxSize) 
	{
		if (i+2 < fromSize && from[i] == 0 && from[i+1] == 0 && from[i+2] == 3) 
		{
			to[toSize] = to[toSize+1] = 0;
			toSize += 2;
			i += 3;
		}
		else 
		{
			to[toSize] = from[i];
			toSize += 1;
			i += 1;
		}
	}

	return toSize;
}

static uint8_t * avc_find_startcode_internal(uint8_t *p, uint8_t *end)
{
    uint8_t *a = p + 4 - ((intptr_t)p & 3);

    for (end -= 3; p < a && p < end; p++) 
    {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
        {
        	return p;
		}          
    }

    for (end -= 3; p < end; p += 4) 
    {
        uint32_t x = *(const uint32_t*)p;
        
        if ((x - 0x01010101) & (~x) & 0x80808080) 
        { // generic
            if (p[1] == 0) 
            {
                if (p[0] == 0 && p[2] == 1)
                    return p;
                if (p[2] == 0 && p[3] == 1)
                    return p+1;
            }
            
            if (p[3] == 0) 
            {
                if (p[2] == 0 && p[4] == 1)
                    return p+2;
                if (p[4] == 0 && p[5] == 1)
                    return p+3;
            }
        }
    }

    for (end += 3; p < end; p++) 
    {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
        {
        	return p;
        }
    }

    return end + 3;
}

uint8_t * avc_find_startcode(uint8_t *p, uint8_t *end)
{
    uint8_t *out = avc_find_startcode_internal(p, end);
    if (p<out && out<end && !out[-1])
    {
    	out--;
    }
    
    return out;
}



