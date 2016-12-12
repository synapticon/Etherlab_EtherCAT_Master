/*
 * sdoinfo.h
 *
 * Access the SDO Info service information.
 *
 * Frnak Jeschke <fjeschke@synapticon.com>
 * Copyright Synpaticon 2016
 */

#ifndef SDOINFO_H
#define SDOINFO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ecrt.h>
#include <stdint.h>

enum {
    SDO_ENTRY_ACCESS_PREOP
    ,SDO_ENTRY_ACCESS_SAFEOP
    ,SDO_ENTRY_ACCESS_OP
    ,SDO_ENTRY_ACCESS_COUNT
};

struct _sdo_info {
    uint16_t index;
    uint8_t  maxindex;
    char     name[EC_MAX_STRING_LENGTH];
};

struct _sdo_entry {
    uint16_t data_type;
    uint16_t bit_length;
    uint8_t  read_access[SDO_ENTRY_ACCESS_COUNT];
    uint8_t  write_access[SDO_ENTRY_ACCESS_COUNT];
    char     description[EC_MAX_STRING_LENGTH];
};

int sdoinfo_get_sdo(ec_master_t *master, struct _sdo_info *sdo, uint16_t slave_pos, uint16_t sdo_position);

int sdoinfo_get_entry(ec_master_t *master, struct _sdo_entry *entry, uint16_t slave_pos, int index, uint8_t subindex);

#ifdef __cplusplus__
}
#endif

#endif /* SDOINFO_H */
