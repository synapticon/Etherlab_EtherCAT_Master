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
/*
 * Adaption to Synapticon SOMANET by Frank Jeschke <fjeschke@synapticon.com>
 *
 * for Synapticon GmbH
 */

#include "commons.h" /* for timediff and mean */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>

/****************************************************************************/

#include "ecrt.h"

/* new interface */
#include "ethercat_wrapper.h"
#include "ethercat_wrapper_slave.h"

#include "slave.h"  /* this is a internal header - just for testing included here */

/****************************************************************************/

#define MAXDBGLVL  3

// Application parameters
#define FREQUENCY 1000

/****************************************************************************/

/* application global definitions */
static int g_dbglvl = 1;
static int g_running = 1;

// Timer
static unsigned int sig_alarms = 0;
static unsigned int user_alarms = 0;

/****************************************************************************/

static void logmsg(int lvl, const char *format, ...);

/****************************************************************************/

void signal_handler(int signum)
{
  switch (signum) {
    case SIGINT:
    case SIGTERM:
      g_running = 0;
      break;
    case SIGALRM:
      sig_alarms++;
      break;
  }
}

/****************************************************************************/

static void set_priority(void)
{
  if (getuid() != 0) {
    logmsg(0, "Warning, be root to get higher priority\n");
    return;
  }

  pid_t pid = getpid();
  if (setpriority(PRIO_PROCESS, pid, -19))
    fprintf(stderr, "Warning: Failed to set priority: %s\n", strerror(errno));
}

static void setup_signal_handler(struct sigaction *sa)
{
  /* setup signal handler */
  sa->sa_handler = signal_handler;
  sigemptyset(&(sa->sa_mask));
  sa->sa_flags = 0;
  if (sigaction(SIGALRM, sa, 0)) {
    fprintf(stderr, "Failed to install signal handler!\n");
    exit(-1);
  }

  if (sigaction(SIGTERM, sa, 0)) {
    fprintf(stderr, "Failed to install signal handler!\n");
    exit(-1);
  }

  if (sigaction(SIGINT, sa, 0)) {
    fprintf(stderr, "Failed to install signal handler!\n");
    exit(-1);
  }
}

static void stop_timer(struct itimerval *tv)
{
  tv->it_interval.tv_sec = 0;
  tv->it_interval.tv_usec = 0;
  tv->it_value.tv_sec = 0;
  tv->it_value.tv_usec = 0;
  if (setitimer(ITIMER_REAL, tv, NULL)) {
    fprintf(stderr, "Failed to start timer: %s\n", strerror(errno));
    exit(-1);
  }
}

static void setup_timer(struct itimerval *tv)
{
  /* setup timer */
  logmsg(1, "Starting timer...\n");
  tv->it_interval.tv_sec = 0;
  tv->it_interval.tv_usec = 1000000 / FREQUENCY;
  tv->it_value.tv_sec = 0;
  tv->it_value.tv_usec = 1000;
  if (setitimer(ITIMER_REAL, tv, NULL)) {
    fprintf(stderr, "Failed to start timer: %s\n", strerror(errno));
    exit(-1);
  }
}

/* debugging functions */
static void get_master_information(Ethercat_Master_t *master)
{
  if (g_dbglvl < 2)
    return;

  ec_master_info_t *master_info = NULL;

  if (master_info == NULL) {
    fprintf(stderr, "[%s] Error retrieve master informations.\n", __func__);
    return;
  }

  printf("Master Info:\n");
  printf("  Slave Count ... : %d\n", master_info->slave_count);
  printf("  Link Up     ... : %s\n",
         (master_info->link_up == 0) ? "false" : "true");
  printf("  Scan Busy   ... : %s\n",
         (master_info->scan_busy == 0) ? "not scanning" : "scanning");
  printf("  ... end of info\n\n");
}

static char *get_watchdog_mode_string(ec_watchdog_mode_t watchdog_mode)
{
  switch (watchdog_mode) {
    case EC_WD_DEFAULT:
      return "default";
    case EC_WD_ENABLE:
      return "enable";
    case EC_WD_DISABLE:
      return "disable";
    default:
      return "Error: Unknown watchdog mode!";
  }

  return "none";
}

static char *get_direction_string(ec_direction_t dir)
{
  switch (dir) {
    case EC_DIR_INVALID:
      return "invalid";
      break;
    case EC_DIR_OUTPUT:
      return "output";
      break;
    case EC_DIR_INPUT:
      return "input";
      break;
    case EC_DIR_COUNT:
      return "count";
      break;
    default:
      return "Error: Unknown direction string!";
      break;
  }

  return "nothing";
}

static const char *al_state_string(ec_al_state_t state)
{
  switch (state) {
    case EC_AL_STATE_INIT:
      return "Init";
    case EC_AL_STATE_PREOP:
      return "Pre OP";
    case EC_AL_STATE_SAFEOP:
      return "Safe OP";
    case EC_AL_STATE_OP:
      return "OP";
    default:
      break;
  }

  return "Unknown";
}

static void get_slave_information(Ethercat_Master_t *master, int slaveid)
{
  Ethercat_Slave_t *slave = ecw_slave_get(master, slaveid);  //(master->slave + slaveid);
  Ethercat_Slave_Info_t slaveinfo;

  ecw_slave_get_info(slave, &slaveinfo);

  if (slave == NULL) {
    fprintf(stderr, "Error reading information for slave %d\n", slaveid);
    return;
  }

  printf("General slave information:\n");
  printf("  Vendor ID: ...........  0x%x\n", slaveinfo.vendor_id);
  printf("  Product Code..........  0x%x\n", slaveinfo.product_code);
  printf("  Slave ID/Position: .... %d (%d)\n", slaveinfo.position, slaveid);
  printf("  Number of SDOs: ....... %d\n", slaveinfo.sdo_count);
  printf("  AL State: ............. %s\n",
         al_state_string(slave->info->al_state)); /* FIXME */
  printf("  Slave Name: ........... %s\n", slaveinfo.name);

  uint8_t sync_count = slaveinfo.sync_manager_count;
  printf("  Sync Manager Count: ... %d\n", sync_count);

  for (uint8_t i = 0; i < sync_count; i++) {
    ec_sync_info_t *syncman_info = (slave->sm_info) + i;

    /* FIMXE check if this still holds */
    printf("    Sync Manager:    %d\n", syncman_info->index);
    printf("    | direction:     %s\n",
           get_direction_string(syncman_info->dir));
    printf("    | number of PDO: %d\n", syncman_info->n_pdos);
    printf("    | watchdog mode: %s\n",
           get_watchdog_mode_string(syncman_info->watchdog_mode));
    printf("\n");
  }
}

static void logmsg(int lvl, const char *format, ...)
{
  if (lvl > g_dbglvl)
    return;

  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);
}

static inline const char *_basename(const char *prog)
{
  const char *p = prog;
  const char *i = p;
  for (i = p; *i != '\0'; i++) {
    if (*i == '/')
      p = i + 1;
  }

  return p;
}

static void printversion(const char *prog)
{
  printf("%s %s\n", _basename(prog), ecw_master_get_version());
}

static void printhelp(const char *prog)
{
  printf("Usage: %s [-h] [-v] [-l <level>]\n", _basename(prog));
  printf("\n");
  printf("  -h             print this help and exit\n");
  printf("  -v             print version and exit\n");
  printf("  -l <level>     set log level (0..3)\n");
}

static void cmdline(int argc, char **argv, char **target_file)
{
  (void) target_file;
  int opt;

  const char *options = "hvl:s:";

  while ((opt = getopt(argc, argv, options)) != -1) {
    switch (opt) {
      case 'v':
        printversion(argv[0]);
        exit(0);
        break;

      case 'l':
        g_dbglvl = atoi(optarg);
        if (g_dbglvl < 0 || g_dbglvl > MAXDBGLVL) {
          fprintf(stderr, "Error unsupported debug level %d.\n", g_dbglvl);
          exit(1);
        }
        break;

      case 'h':
      default:
        printhelp(argv[0]);
        exit(1);
        break;
    }
  }
}

/*
 * Debug output for object dictionary
 */
static void read_object_dictionary_slave(Ethercat_Master_t *master, int slaveid)
{
  Ethercat_Slave_t *slave = ecw_slave_get(master, slaveid);

  for (int j = 0; j < ecw_slave_get_sdo_count(slave); j++) {
    Sdo_t *sdo = ecw_slave_get_sdo_index(slave, j);

    printf("  Sdo (%d)\n", j);
    printf("    0x%04x:%d length: %d (%d/%d) '%s'\n", sdo->index, sdo->subindex,
           sdo->bit_length, sdo->object_type, sdo->entry_type, sdo->name);

    free(sdo);
  }
}

static void read_object_dictionary(Ethercat_Master_t *master)
{
  int slave_count = ecw_master_slave_count(master);

  for (int i = 0; i < slave_count; i++) {
    printf("Object Dictionary for Slave %d\n", i);
    read_object_dictionary_slave(master, i);
  }
}

int main(int argc, char **argv)
{
  cmdline(argc, argv, NULL);

  //ec_slave_config_t *sc;
  struct sigaction sa;
  struct itimerval tv;

  Ethercat_Master_t *master = ecw_master_init(0 /* master id */);

  if (master == NULL) {
    fprintf(stderr, "[ERROR %s] Cannot initialize master\n", __func__);
    return 1;
  }

  if (g_dbglvl >= 3) {
    printf("\nPrint bus topology\n");
    ecw_print_topology(master);
    printf("\nprint domain registries\n");
    ecw_print_domain_regs(master);
    printf("\nPrint all slaves object dictionary\n");
    ecw_print_all_slave_od(master);

    /* master information */
    get_master_information(master);
    for (int i = 0; i < ecw_master_slave_count(master); i++) {
      get_slave_information(master, i);
    }

    read_object_dictionary(master);
  }

  /*
   * Activate master and start operation
   */

  int master_running = 0;
#if LIBINTERNAL_CYCLIC_HANDLING
#  warning Not implemented cyclic handling is used.
  master_running = ecw_master_start_cyclic(master);
#else
  master_running = ecw_master_start(master);
#endif
  if (master_running) {
    fprintf(stderr, "Master not started, aborting operation\n");
    return 1;
  }

  set_priority();
  setup_signal_handler(&sa);
  setup_timer(&tv);
  logmsg(0, "Started.\n");

  int slave_count = ecw_master_slave_count(master);
  logmsg(0, "Number of slaves: %d\n", slave_count);

  struct timespec time_start;
  struct timespec time_end;
  struct timespec time_diff;
  double time_mean = 0;
  size_t time_counter = 0;

  //g_running = 0;
  while (g_running) {
    pause();

    /* wait for the timer to rise alarm */
    while (sig_alarms != user_alarms) {
      user_alarms++;

#if 0
      ecw_master_receive_pdo(master);
      ecw_master_send_pdo(master);
#endif
      clock_gettime(CLOCK_MONOTONIC, &time_start);
      ecw_master_cyclic_function(master);

      clock_gettime(CLOCK_MONOTONIC, &time_end);

      timespec_diff(&time_start, &time_end, &time_diff);
      time_mean = calc_mean(time_mean, (double) time_diff.tv_nsec,
                            time_counter++);

//    printf("[DEBUG] set target position %d on node 0\n", outbound[0]);

      Ethercat_Slave_t *slave = ecw_slave_get(master, 0);
      int value = ecw_slave_get_in_value(slave, 0);
//    printf("< 0x%0x ", value);
      //value = (~value) & 0xffff;
      value = (value + 1) & 0xffff;
//    printf("> 0x%0x\n", value);
      ecw_slave_set_out_value(slave, 0, value);
    }
  }

  ecw_master_stop(master);
  ecw_master_release(master);

  printf("Summary mean runtime of ecw_master_cyclic_function(): %f ns\n",
         time_mean);

  return 0;
}
