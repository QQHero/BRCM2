#ifndef _TD_IE_HELPER_H_
#define _TD_IE_HELPER_H_

#include "td_private_ie.h"

#include "td_ie_manage.h"
#include <linux/kernel.h>

/*
 * append data to IE in TLV format
 * 
 * pos:     the buffer to write this item
 * type:    type to indentify a TLV item
 * len:     length of value
 * value:   data to copy
 * 
 * return the updated buffer to write the next item
 */
static inline uint8_t* td_ie_add_data(uint8_t* pos, uint8_t type, 
                                    uint8_t len, void* value)
{
    *pos++ = type;
    *pos++ = len;
    memcpy(pos, value, len);

    return pos + len;
}

static inline uint8_t* td_ie_add_byte(uint8_t* pos, uint8_t type, uint8_t value)
{
    *pos++ = type;
    *pos++ = 1;
    *pos = value;

    return pos + 1;
}

static inline uint8_t* td_ie_add_int32(uint8_t* pos, uint8_t type, int32_t value)
{
    *pos++ = type;
    *pos++ = 4;
    *(int32_t*)pos = cpu_to_be32(value);

    return pos + 4;
}

#define td_ie_add_mac(pos, type, value) \
td_ie_add_data(pos, type, MAC_ADDR_LEN, value)

/*
 * return the value specified by `type` or NULL if it doesn't exist
 */
static inline uint8_t* td_ie_get_data(uint8_t* data, uint8_t data_len, uint8_t type)
{
    uint8_t *end = data + data_len;
    while ((data + 2) < end) {
        if (data[0] == type) {
            return data + 2;
        }
        data += (data[1] + 2);
    }

    return NULL;
}

static inline uint8_t td_ie_get_byte(uint8_t* data, uint8_t data_len, uint8_t type)
{
    uint8_t *end = data + data_len;
    while ((data + 2) < end) {
        if (data[0] == type) {
            return data[2];
        }
        data += (data[1] + 2);
    }

    return 0;
}

static inline int32_t td_ie_get_int32(uint8_t* data, uint8_t data_len, uint8_t type)
{
    uint8_t *end = data + data_len;
    while ((data + 2) < end) {
        if (data[0] == type) {
            return be32_to_cpu(*(int32_t*)(data + 2));
        }
        data += (data[1] + 2);
    }

    return 0;
}

#define td_ie_get_mac(data, data_len, type) \
td_ie_get_data(data, data_len, type)

/*
 * iterate all the TLV items in the element
 * 
 * data:        the unsigned char* for the data
 * data_len:    length of the data
 * pos:         a unsigned char* used as a cursor
 * type:        a unsigned char variable as the type of current TLV item
 * len:         a unsigned char variable as the length of current TLV item
 * value:       a unsigned char* variable as the value of current TLV item
 */
#define td_ie_for_each(data, data_len, pos, type, len, value) \
for (pos = (data);\
    ((pos) + 2 < (data) + (data_len)) && \
    ({type = (pos)[0];len = (pos)[1];value = &(pos)[2];1;});\
    pos += (len + 2))

#endif /* _TD_IE_HELPER_H_ */
