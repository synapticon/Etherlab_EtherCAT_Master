/*
 * sdoinfo.c
 *
 * Access the SDO Info service information.
 *
 * Frnak Jeschke <fjeschke@synapticon.com>
 * Copyright Synpaticon 2016
 */

#include "sdoinfo.h"

#include <sys/ioctl.h>
#include <sys/types.h>
#include <string.h>
#include <stdint.h>

#include "../../lib/master.h"
#include "../../master/ioctl.h"

int sdoinfo_get_sdo(ec_master_t *master, struct _sdo_info *sdo, uint16_t slave_pos, uint16_t sdo_position)
{
    ec_ioctl_slave_sdo_t s;
    s.slave_position = slave_pos;
    s.sdo_position   = sdo_position;

    if (ioctl(master->fd, EC_IOCTL_SLAVE_SDO, &s)) {
        return -1;
    }

    sdo->index = s.sdo_index;
    sdo->maxindex = s.max_subindex;
    memmove(sdo->name, s.name, EC_IOCTL_STRING_SIZE);


    return 0;
}

int sdoinfo_get_entry(ec_master_t *master, struct _sdo_entry *entry, uint16_t slave_pos, int index, uint8_t subindex)
{
    ec_ioctl_slave_sdo_entry_t e;
    e.slave_position = slave_pos;
    e.sdo_spec = index; /* the name spec is a little bit misguiding, actually
                           if >0 it is the index of the object, <0 is the index position. */
    e.sdo_entry_subindex = subindex;

    if (ioctl(master->fd,  EC_IOCTL_SLAVE_SDO_ENTRY, &e)) {
        return -1;
    }

    entry->data_type = e.data_type;
    entry->bit_length = e.bit_length;
    memmove(entry->read_access, e.read_access, EC_SDO_ENTRY_ACCESS_COUNT);
    memmove(entry->write_access, e.write_access, EC_SDO_ENTRY_ACCESS_COUNT);
    memmove(entry->description, e.description, EC_IOCTL_STRING_SIZE);

    return 0;
}

