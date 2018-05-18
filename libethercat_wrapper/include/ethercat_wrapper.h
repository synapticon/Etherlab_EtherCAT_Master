/*
 * ethercat_wrapper.h
 *
 * Main header for the Synapticon EtherCAT library.
 *
 * 2016, Synapticon GmbH
 */

#ifndef ETHERCAT_WRAPPER_H
#define ETHERCAT_WRAPPER_H

#include <ethercat_wrapper_slave.h>
#include <stdio.h>
#include <unistd.h>

/* Error codes */
#define ECW_SUCCESS                          0
#define ECW_ERROR_LINK_UP                   -2
#define ECW_ERROR_UNKNOWN                   -128
#define ECW_ERROR_SDO_REQUEST_BUSY           1
#define ECW_ERROR_SDO_REQUEST_ERROR         -1
#define ECW_ERROR_SDO_NOT_FOUND             -3
#define ECW_ERROR_SDO_UNSUPORTED_BITLENGTH  -4

/* -> Ethercat_Master_t */
struct _ecw_master_t {
  int id;
  /* master information */
  ec_master_t *master;

  /* variables for data structures */
  ec_domain_t *domain;
  ec_pdo_entry_reg_t *domain_reg;
  uint8_t *processdata; /* FIXME are they needed here? */

  /* slaves */
  Ethercat_Slave_t *slave;  ///<< list of slaves
  size_t slave_count;

  /* diagnostic data structures */
  ec_master_state_t master_state;
  ec_domain_state_t domain_state;
};

typedef struct _ecw_master_t Ethercat_Master_t;
typedef struct _ecw_slave_t Ethercat_Slave_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Get version string fo this library
 */
const char *ecw_master_get_version(void);

/**
 * \brief Return the number of slaves without initializing or reserving the
 * master first
 *
 * \param master_id   id of the master to use, for single master use 0
 *
 * \return number of slaves, or -1 on error
 */
int ecw_preemptive_slave_count(int master_id);

/**
 * \brief Return the number of SDOs of a slave without initializing or reserving
 * the master first
 *
 * \param master_id     id of the master to use, for single master use 0
 * \param slave_index   index of the slave
 *
 * \return number of SDOs, or -1 on error
 */
int ecw_preemptive_slave_sdo_count(int master_id, int slave_index);

/**
 * \brief Return the current state of a slave without initializing or reserving
 * the master first
 *
 * \param master_id     id of the master to use, for single master use 0
 * \param slave_index   index of the slave
 *
 * \return the state of the slave, or -1 on error
 */
int ecw_preemptive_slave_state(int master_id, int slave_index);

/**
 * \brief Create ethercat master object and initialize
 *
 * The init process scans the bus and configures all slaves.
 *
 * \param master_id   id of the master to use, for single master use 0
 * \param log         pointer to file descriptor for logging
 * \return initialized master object or NULL on error
 */
Ethercat_Master_t *ecw_master_init(int master_id, FILE *log);

/**
 * \brief clean up master object
 *
 * All data structures are cleared.
 *
 * \param master   pointer to master to be cleared
 */
void ecw_master_release(Ethercat_Master_t *);

/**
 * \brief Set master in operation state
 *
 * Starts the master cyclic operation.
 */
int ecw_master_start(Ethercat_Master_t *);

/**
 * \brief Bring back master (and all slaves) back to preop state.
 */
int ecw_master_stop(Ethercat_Master_t *);

int ecw_master_scan(Ethercat_Master_t *);

#ifdef LIBINTERNAL_CYCLIC_HANDLING /* not recommended */
int ecw_master_start_cyclic(Ethercat_Master_t *);
int ecw_master_stop_cyclic(Ethercat_Master_t *master);
#else

/**
 * \brief This function has to be called in a real time context in a regular manner!
 *
 * \param master  the master to use
 * \return  0 no error != 0 something went wrong
 */
int ecw_master_cyclic_function(Ethercat_Master_t *);
#endif

/*
 * The following functions are necessary in the cyclic task/function to assure
 * the PDO data are exchanged with the kernel module.
 */

/**
 * \brief Exchange PDO values during cyclic operation
 *
 * This function basically calls \c ecw_master_receive_pdo() and
 * then \c ecw_master_send_pdo()
 *
 * \param master  master to request
 * \return 0 on success
 */
int ecw_master_pdo_exchange(Ethercat_Master_t *master);

/**
 * \brief Get the received PDO values from the kernel
 *
 * \param master  master to request
 *
 * \return 0 on success
 */
int ecw_master_receive_pdo(Ethercat_Master_t *master);

/**
 * \brief Update the  PDO values to the kernel
 *
 * \param master  master to request
 *
 * \return 0 on success
 */
int ecw_master_send_pdo(Ethercat_Master_t *master);

/**
 * \brief Return pointer to slave by index
 *
 * \param master   master to request
 * \param slaveid  id of the slave
 * \return reference to the slave by slaveid, NULL on error
 */
Ethercat_Slave_t *ecw_slave_get(Ethercat_Master_t *master, int slaveid);

/**
 * \brief Request AL state from slave
 *
 * \param master   master to request
 * \param slaveid  id of the slave
 * \param state    state to request slave to enter
 * \return  0 on success, negative on error
 */
int ecw_slave_set_state(Ethercat_Master_t *master, int slaveid,
                        enum eALState state);

/**
 * \brief Request the number of slaves the master has read
 *
 * \param master   master to request
 * \return number of slaves
 */
size_t ecw_master_slave_count(Ethercat_Master_t *);

/**
 * \brief Request the number of slaves responding on the bus
 *
 * If anything happens on the bus and a slave fails this count will
 * indicate the problem and the using instance can apply proper handling
 * of the situation.
 *
 * \param master   master to request
 * \return number of slaves responding
 */
size_t ecw_master_slave_responding(Ethercat_Master_t *);

/* DEBUG print function */

/**
 * \brief Debug print of topology
 */
void ecw_print_topology(Ethercat_Master_t *master);

/**
 * \brief Debug print of domain registration
 */
void ecw_print_domainregs(Ethercat_Master_t *master);

/**
 * \brief Debug print of all slaves object dictionary
 */
void ecw_print_allslave_od(Ethercat_Master_t *master);

/**
 * \brief Debug print master state
 *
 * \param master   master to request
 */
void ecw_print_master_state(Ethercat_Master_t *master);

/**
 * \brief Get error name from error number
 *
 * \param master   errnum to get
 * \return string name of error
 */
char *ecw_strerror(int errnum);

#ifdef __cplusplus
}
#endif

#endif /* ETHERCAT_WRAPPER_H */
