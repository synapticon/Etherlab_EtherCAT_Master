/*
 * main.c
 *
 * Test the libsdoinfo
 */

#include "sdoinfo.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/****************************************************************************/

#include "ecrt.h"

/****************************************************************************/

#define SOMANET_POS       0, 0
#define SOMANET_DEVICE    0x000022d2, 0x00000201

/****************************************************************************/

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    ec_master_t *master = NULL;
    ec_slave_config_t *slave0 = NULL;

    master = ecrt_request_master(0);
    if (!master)
        return -1;

    if (!(slave0 = ecrt_master_slave_config(
                    master, SOMANET_POS, SOMANET_DEVICE))) {
        fprintf(stderr, "Failed to get slave configuration.\n");
        return -1;
    }

    ec_slave_info_t slave0_info;
    if (ecrt_master_get_slave(master, 0, &slave0_info)) {
        fprintf(stderr, "Faiiled to get slave config\n");
        return -1;
    }

    struct _sdo_info sdo;

    for (int i = 0; i < slave0_info.sdo_count; i++) {
        sdoinfo_get_sdo(master, &sdo, 0, i);
 
        printf("Received SDO (0x%0x) with: index 0x%04x, max subindexes: %d ('%s')\n",
            i, sdo.index, sdo.maxindex, sdo.name);

        struct _sdo_entry entry;

        for (int k = 0; k <= sdo.maxindex; k++) {
            sdoinfo_get_entry(master, &entry, 0, sdo.index, k);
            printf("  [%d] data type: %d, bit length: %d, description: '%s'\n",
                    k, entry.data_type, entry.bit_length, entry.description);
        }
    }

    return 0;
}

/****************************************************************************/
