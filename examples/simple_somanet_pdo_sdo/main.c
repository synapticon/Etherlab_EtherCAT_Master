/*****************************************************************************
 *
 *  $Id$
 *
 *  Copyright (C) 2007-2009  Florian Pose, Ingenieurgemeinschaft IgH
 *
 *  This file is part of the IgH EtherCAT Master.
 *
 *  The IgH EtherCAT Master is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License version 2, as
 *  published by the Free Software Foundation.
 *
 *  The IgH EtherCAT Master is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *  Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with the IgH EtherCAT Master; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  ---
 *
 *  The license mentioned above concerns the source code only. Using the
 *  EtherCAT technology and brand is only permitted in compliance with the
 *  industrial property and similar rights of Beckhoff Automation GmbH.
 *
 ****************************************************************************/

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

// Application parameters
#define FREQUENCY 1000
#define PRIORITY 1

// Optional features
#define SDO_ACCESS      1

/****************************************************************************/

// EtherCAT
static ec_master_t *master = NULL;
static ec_master_state_t master_state = {};

static ec_domain_t *domain1 = NULL;
static ec_domain_state_t domain1_state = {};

static ec_slave_config_t *sc_data_in = NULL;
static ec_slave_config_state_t sc_data_in_state = {};

// Timer
static unsigned int sig_alarms = 0;
static unsigned int user_alarms = 0;

/****************************************************************************/

// process data
static uint8_t *domain1_pd = NULL;

#define DEVICE_POSITION     0, 0
#define SOMANET_ID          0x000022d2, 0x00000201

// offsets for PDO entries

/* outputs */
static unsigned int off_pdo_controlword;
static unsigned int off_pdo_opmode;
static unsigned int off_pdo_target_torque;
static unsigned int off_pdo_target_position;
static unsigned int off_pdo_target_velocity;
static unsigned int off_pdo_torque_offset;
static unsigned int off_pdo_tuning_command;
static unsigned int off_pdo_digital_output_1;
static unsigned int off_pdo_digital_output_2;
static unsigned int off_pdo_digital_output_3;
static unsigned int off_pdo_digital_output_4;
static unsigned int off_pdo_user_mosi;
static unsigned int off_pdo_velocity_offset;

/* inputs */
static unsigned int off_pdo_statusword;
static unsigned int off_pdo_opmode_display;
static unsigned int off_pdo_position_value;
static unsigned int off_pdo_velocity_value;
static unsigned int off_pdo_torque_value;
static unsigned int off_pdo_secondary_position_value;
static unsigned int off_pdo_secondary_velocity_value;
static unsigned int off_pdo_analog_input_1;
static unsigned int off_pdo_analog_input_2;
static unsigned int off_pdo_analog_input_3;
static unsigned int off_pdo_analog_input_4;
static unsigned int off_pdo_tuning_status;
static unsigned int off_pdo_digital_input_1;
static unsigned int off_pdo_digital_input_2;
static unsigned int off_pdo_digital_input_3;
static unsigned int off_pdo_digital_input_4;
static unsigned int off_pdo_user_miso;
static unsigned int off_pdo_timestamp;
static unsigned int off_pdo_position_demand;
static unsigned int off_pdo_velocity_demand;
static unsigned int off_pdo_torque_demand;

const static ec_pdo_entry_reg_t domain1_regs[] = {
    // outputs
    {DEVICE_POSITION, SOMANET_ID, 0x6040, 0, &off_pdo_controlword,     NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x6060, 0, &off_pdo_opmode,          NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x6071, 0, &off_pdo_target_torque,   NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x607a, 0, &off_pdo_target_position, NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x60ff, 0, &off_pdo_target_velocity, NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x60B2, 0, &off_pdo_torque_offset,   NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x2701, 0, &off_pdo_tuning_command,  NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x2601, 0, &off_pdo_digital_output_1,NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x2602, 0, &off_pdo_digital_output_2,NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x2603, 0, &off_pdo_digital_output_3,NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x2604, 0, &off_pdo_digital_output_4,NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x2703, 0, &off_pdo_user_mosi,       NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x60B1, 0, &off_pdo_velocity_offset, NULL},
    // inouts
    {DEVICE_POSITION, SOMANET_ID, 0x6041, 0, &off_pdo_statusword,      NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x6061, 0, &off_pdo_opmode_display,  NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x6064, 0, &off_pdo_position_value,  NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x606c, 0, &off_pdo_velocity_value,  NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x6077, 0, &off_pdo_torque_value,    NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x230A, 0, &off_pdo_secondary_position_value,NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x230B, 0, &off_pdo_secondary_velocity_value,NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x2401, 0, &off_pdo_analog_input_1,  NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x2402, 0, &off_pdo_analog_input_2,  NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x2403, 0, &off_pdo_analog_input_3,  NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x2404, 0, &off_pdo_analog_input_4,  NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x2702, 0, &off_pdo_tuning_status,   NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x2501, 0, &off_pdo_digital_input_1, NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x2502, 0, &off_pdo_digital_input_2, NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x2503, 0, &off_pdo_digital_input_3, NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x2504, 0, &off_pdo_digital_input_4, NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x2704, 0, &off_pdo_user_miso,       NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x20F0, 0, &off_pdo_timestamp,       NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x60FC, 0, &off_pdo_position_demand, NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x606B, 0, &off_pdo_velocity_demand, NULL},
    {DEVICE_POSITION, SOMANET_ID, 0x6074, 0, &off_pdo_torque_demand,   NULL},
    {0}
};

/*****************************************************************************/

/* Master 0, Slave 0
 * Vendor ID:       0x000022d2
 * Product code:    0x00000201
 * Revision number: 0x0a000002
 */

ec_pdo_entry_info_t somanet_pdo_entries[] = {
    {0x6040, 0x00, 16},
    {0x6060, 0x00, 8},
    {0x6071, 0x00, 16},
    {0x607a, 0x00, 32},
    {0x60ff, 0x00, 32},
    {0x60b2, 0x00, 16},
    {0x2701, 0x00, 32},
    {0x2601, 0x00, 1},
    {0x2007, 0x00, 7},
    {0x2602, 0x00, 1},
    {0x2007, 0x00, 7},
    {0x2603, 0x00, 1},
    {0x2007, 0x00, 7},
    {0x2604, 0x00, 1},
    {0x2007, 0x00, 7},
    {0x2703, 0x00, 32},
    {0x60b1, 0x00, 32},
    {0x6041, 0x00, 16},
    {0x6061, 0x00, 8},
    {0x6064, 0x00, 32},
    {0x606c, 0x00, 32},
    {0x6077, 0x00, 16},
    {0x230a, 0x00, 32},
    {0x230b, 0x00, 32},
    {0x2401, 0x00, 16},
    {0x2402, 0x00, 16},
    {0x2403, 0x00, 16},
    {0x2404, 0x00, 16},
    {0x2702, 0x00, 32},
    {0x2501, 0x00, 1},
    {0x2007, 0x00, 7},
    {0x2502, 0x00, 1},
    {0x2007, 0x00, 7},
    {0x2503, 0x00, 1},
    {0x2007, 0x00, 7},
    {0x2504, 0x00, 1},
    {0x2007, 0x00, 7},
    {0x2704, 0x00, 32},
    {0x20f0, 0x00, 32},
    {0x60fc, 0x00, 32},
    {0x606b, 0x00, 32},
    {0x6074, 0x00, 16},
};

ec_pdo_info_t somanet_pdos[] = {
    {0x1600, 7, somanet_pdo_entries + 0}, /* RxPDO Mapping 1 */
    {0x1601, 8, somanet_pdo_entries + 7}, /* RxPDO Mapping 2 */
    {0x1602, 2, somanet_pdo_entries + 15}, /* RxPDO Mapping 3 */
    {0x1a00, 5, somanet_pdo_entries + 17}, /* TxPDO Mapping 1 */
    {0x1a01, 7, somanet_pdo_entries + 22}, /* TxPDO Mapping 2 */
    {0x1a02, 8, somanet_pdo_entries + 29}, /* TxPDO Mapping 3 */
    {0x1a03, 5, somanet_pdo_entries + 37}, /* TxPDO Mapping 4 */
};

ec_sync_info_t somanet_syncs[] = {
    {0, EC_DIR_OUTPUT, 0, NULL, EC_WD_DISABLE},
    {1, EC_DIR_INPUT, 0, NULL, EC_WD_DISABLE},
    {2, EC_DIR_OUTPUT, 3, somanet_pdos + 0, EC_WD_DISABLE},
    {3, EC_DIR_INPUT, 4, somanet_pdos + 3, EC_WD_DISABLE},
    {0xff}
};

/*****************************************************************************/

#if SDO_ACCESS
static ec_sdo_request_t *dc_voltage_sdo_request;
static ec_sdo_request_t *core_temperature_sdo_request;
static ec_sdo_request_t *drive_temperature_sdo_request;
static ec_sdo_request_t *motor_rated_current_sdo_request;
#endif

/*****************************************************************************/

void check_domain1_state(void)
{
    ec_domain_state_t ds;

    ecrt_domain_state(domain1, &ds);

    if (ds.working_counter != domain1_state.working_counter)
        printf("Domain1: WC %u.\n", ds.working_counter);
    if (ds.wc_state != domain1_state.wc_state)
        printf("Domain1: State %u.\n", ds.wc_state);

    domain1_state = ds;
}

/*****************************************************************************/

void check_master_state(void)
{
    ec_master_state_t ms;

    ecrt_master_state(master, &ms);

    if (ms.slaves_responding != master_state.slaves_responding)
        printf("%u slave(s).\n", ms.slaves_responding);
    if (ms.al_states != master_state.al_states)
        printf("AL states: 0x%02X.\n", ms.al_states);
    if (ms.link_up != master_state.link_up)
        printf("Link is %s.\n", ms.link_up ? "up" : "down");

    master_state = ms;
}

/*****************************************************************************/

void check_slave_config_states(void)
{
    ec_slave_config_state_t s;

    ecrt_slave_config_state(sc_data_in, &s);

    if (s.al_state != sc_data_in_state.al_state)
        printf("SOMANET: State 0x%02X.\n", s.al_state);
    if (s.online != sc_data_in_state.online)
        printf("SOMANET: %s.\n", s.online ? "online" : "offline");
    if (s.operational != sc_data_in_state.operational)
        printf("SOMANET: %soperational.\n",
                s.operational ? "" : "Not ");

    sc_data_in_state = s;
}

/*****************************************************************************/

#if SDO_ACCESS
void read_sdo(void)
{
  switch (ecrt_sdo_request_state(motor_rated_current_sdo_request)) {
          case EC_REQUEST_UNUSED: // request was not used yet
              ecrt_sdo_request_read(motor_rated_current_sdo_request); // trigger first read
              break;
          case EC_REQUEST_BUSY:
                  fprintf(stderr, "Motor rated current SDO still busy...\n");
              break;
          case EC_REQUEST_SUCCESS:
              fprintf(stderr, "Motor rated current SDO value: %d\n",
                      EC_READ_U32(ecrt_sdo_request_data(motor_rated_current_sdo_request)));
              ecrt_sdo_request_read(motor_rated_current_sdo_request); // trigger next read
              break;
          case EC_REQUEST_ERROR:
              fprintf(stderr, "Failed to read the Motor rated current SDO!\n");
              ecrt_sdo_request_read(motor_rated_current_sdo_request); // retry reading
              break;
      }

    switch (ecrt_sdo_request_state(dc_voltage_sdo_request)) {
        case EC_REQUEST_UNUSED: // request was not used yet
            ecrt_sdo_request_read(dc_voltage_sdo_request); // trigger first read
            break;
        case EC_REQUEST_BUSY:
            fprintf(stderr, "DC voltage SDO still busy...\n");
            break;
        case EC_REQUEST_SUCCESS:
            fprintf(stderr, "DC voltage SDO value: %d\n",
                    EC_READ_U32(ecrt_sdo_request_data(dc_voltage_sdo_request)));
            ecrt_sdo_request_read(dc_voltage_sdo_request); // trigger next read
            break;
        case EC_REQUEST_ERROR:
            fprintf(stderr, "Failed to read the DC voltage SDO!\n");
            ecrt_sdo_request_read(dc_voltage_sdo_request); // retry reading
            break;
    }

    switch (ecrt_sdo_request_state(core_temperature_sdo_request)) {
        case EC_REQUEST_UNUSED: // request was not used yet
            ecrt_sdo_request_read(core_temperature_sdo_request); // trigger first read
            break;
        case EC_REQUEST_BUSY:
            fprintf(stderr, "Core temperature SDO still busy...\n");
            break;
        case EC_REQUEST_SUCCESS:
            fprintf(stderr, "Core temperature SDO value: %d\n",
                    EC_READ_U32(ecrt_sdo_request_data(core_temperature_sdo_request)));
            ecrt_sdo_request_read(core_temperature_sdo_request); // trigger next read
            break;
        case EC_REQUEST_ERROR:
            fprintf(stderr, "Failed to read the core temperature SDO!\n");
            ecrt_sdo_request_read(core_temperature_sdo_request); // retry reading
            break;
    }

    switch (ecrt_sdo_request_state(drive_temperature_sdo_request)) {
        case EC_REQUEST_UNUSED: // request was not used yet
            ecrt_sdo_request_read(drive_temperature_sdo_request); // trigger first read
            break;
        case EC_REQUEST_BUSY:
                fprintf(stderr, "Drive temperature SDO still busy...\n");
            break;
        case EC_REQUEST_SUCCESS:
            fprintf(stderr, "Drive temperature SDO value: %d\n",
                    EC_READ_U32(ecrt_sdo_request_data(drive_temperature_sdo_request)));
            ecrt_sdo_request_read(drive_temperature_sdo_request); // trigger next read
            break;
        case EC_REQUEST_ERROR:
            fprintf(stderr, "Failed to read the drive temperature SDO!\n");
            ecrt_sdo_request_read(drive_temperature_sdo_request); // retry reading
            break;
    }
}
#endif

/****************************************************************************/

void cyclic_task()
{
    // receive process data
    ecrt_master_receive(master);
    ecrt_domain_process(domain1);

    // check process data state (optional)
//    check_domain1_state();

//    if (counter) {
//        counter--;
//    } else { // do this at 1 Hz
//        counter = FREQUENCY;

        // check for master state (optional)
//        check_master_state();
//
//        // check for islave configuration state(s) (optional)
//        check_slave_config_states();

#if SDO_ACCESS
        // read process data SDO
        read_sdo();
#endif

//    }

    // send process data
    ecrt_domain_queue(domain1);
    ecrt_master_send(master);
}

/****************************************************************************/

void signal_handler(int signum) {
    switch (signum) {
        case SIGALRM:
            sig_alarms++;
            break;
    }
}

/****************************************************************************/

int main(int argc, char **argv)
{
    struct sigaction sa;
    struct itimerval tv;

    master = ecrt_request_master(0);
    if (!master)
        return -1;

    domain1 = ecrt_master_create_domain(master);
    if (!domain1)
        return -1;

    if (!(sc_data_in = ecrt_master_slave_config(
                    master, DEVICE_POSITION, SOMANET_ID))) {
        fprintf(stderr, "Failed to get slave configuration.\n");
        return -1;
    }

#if SDO_ACCESS
    fprintf(stderr, "Creating SDO requests...\n");

    if (!(motor_rated_current_sdo_request = ecrt_slave_config_create_sdo_request(sc_data_in, 0x6075, 0, 4))) {
        fprintf(stderr, "Failed to create the motor rated current SDO request.\n");
        return -1;
    }
    ecrt_sdo_request_timeout(motor_rated_current_sdo_request, 500); // ms

    if (!(dc_voltage_sdo_request = ecrt_slave_config_create_sdo_request(sc_data_in, 0x6079, 0, 4))) {
        fprintf(stderr, "Failed to create the DC voltage SDO request.\n");
        return -1;
    }
    ecrt_sdo_request_timeout(dc_voltage_sdo_request, 500); // ms

    if (!(core_temperature_sdo_request = ecrt_slave_config_create_sdo_request(sc_data_in, 0x2030, 1, 4))) {
        fprintf(stderr, "Failed to create the core temperature SDO request.\n");
        return -1;
    }
    ecrt_sdo_request_timeout(core_temperature_sdo_request, 500); // ms

    if (!(drive_temperature_sdo_request = ecrt_slave_config_create_sdo_request(sc_data_in, 0x2031, 1, 4))) {
        fprintf(stderr, "Failed to create the drive temperature SDO request.\n");
        return -1;
    }
    ecrt_sdo_request_timeout(drive_temperature_sdo_request, 500); // ms
#endif

    printf("Configuring PDOs...\n");
    if (ecrt_slave_config_pdos(sc_data_in, EC_END, somanet_syncs)) {
        fprintf(stderr, "Failed to configure PDOs.\n");
        return -1;
    }

    if (ecrt_domain_reg_pdo_entry_list(domain1, domain1_regs)) {
        fprintf(stderr, "PDO entry registration failed!\n");
        return -1;
    }

    printf("Activating master...\n");
    if (ecrt_master_activate(master))
        return -1;

    if (!(domain1_pd = ecrt_domain_data(domain1))) {
        return -1;
    }

#if PRIORITY
    pid_t pid = getpid();
    if (setpriority(PRIO_PROCESS, pid, -19))
        fprintf(stderr, "Warning: Failed to set priority: %s\n",
                strerror(errno));
#endif

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGALRM, &sa, 0)) {
        fprintf(stderr, "Failed to install signal handler!\n");
        return -1;
    }

    printf("Starting timer...\n");
    tv.it_interval.tv_sec = 0;
    tv.it_interval.tv_usec = 1000000 / FREQUENCY;
    tv.it_value.tv_sec = 0;
    tv.it_value.tv_usec = 1000;
    if (setitimer(ITIMER_REAL, &tv, NULL)) {
        fprintf(stderr, "Failed to start timer: %s\n", strerror(errno));
        return 1;
    }

    printf("Started.\n");
    while (1) {
        pause();

#if 0
        struct timeval t;
        gettimeofday(&t, NULL);
        printf("%u.%06u\n", t.tv_sec, t.tv_usec);
#endif

        while (sig_alarms != user_alarms) {
            cyclic_task();
            user_alarms++;
        }
    }

    return 0;
}

/****************************************************************************/
