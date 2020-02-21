/*
 * slave.h
 *
 * Local definitions for the SNCN EtherCAT slaves.
 */

#ifndef _SLAVE_H
#define _SLAVE_H

#include <ecrt.h>
#include "ethercat_wrapper_slave.h"

struct _ecw_slave_t {
    uint16_t reference_alias;   /* keeps the last active alias, since it is
                                   different than in ec_slave_info_t */
    uint16_t relative_position; /* position relative to the last defined alias
                                   in the ring */

    enum eSlaveType type; /* type is determined by the vendor/product numbers */
    int cyclic_mode;      /* to mark when in cyclic mode */

    /* Information structures from libethercat */
    ec_slave_info_t *info;
    ec_sync_info_t *sm_info;
    ec_slave_config_t *config;
    ec_master_t
        *master; /* reference to the master interface this slave is connected */
    ec_slave_config_state_t state; /* read with ecrt_slave_config_state() */

    /*
     * PDO Handling
     */
    unsigned int *out_pdo_offset; /* list of value elements */
    unsigned int *in_pdo_offset;  /* list of value elements */
    size_t out_pdo_count;
    size_t in_pdo_count;

    pdo_t *output_values;
    pdo_t *input_values;

    /*
     * Object Dictionary for SDO exchange
     */

    Sdo_t *dictionary; /* SDOs for configuration */
    size_t
        sdo_count; /* number of all objects and sub-objects in the dictionary */
};

/*
 * set and get PDO values
 */

int ecw_slave_set_out_pdo(const Ethercat_Slave_t *s, size_t pdo_index,
                          pdo_t *pdo);
pdo_t *ecw_slave_get_out_pdo(const Ethercat_Slave_t *s, size_t pdo_index);

int ecw_slave_set_in_pdo(const Ethercat_Slave_t *s, size_t pdo_index,
                         pdo_t *pdo);
pdo_t *ecw_slave_get_in_pdo(const Ethercat_Slave_t *s, size_t pdo_index);

/*
 * Necessary special function to allow the cyclic function to get the result of
 * the SDO read request.
 */
int sdo_read_value(Sdo_t *sdo);

#endif /* _SLAVE_H */
