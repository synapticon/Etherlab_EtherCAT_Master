/*
 * slave.c
 *
 * Frank Jeschke <fjeschke@synapticon.com>
 *
 * 2016-11-30 Synapticon GmbH
 */

#include "slave.h"
#include "ethercat_wrapper_slave.h"
#include "ethercat_wrapper.h"

#include <string.h>
#include <errno.h>

/* list of supported ETherCAT slaves */
static const Device_type_map_t type_map[] = { { 0x22d2, 0x201, 0x0a000002,
    SLAVE_TYPE_CIA402_DRIVE }, { 0x22d2, 0x202, 0, SLAVE_TYPE_DIGITAL_IO }, {
    0x22d2, 0x203, 0, SLAVE_TYPE_ENDEFFECTOR_IO }, { 0 } };

/*
 * The direct up-/download of SDOs is only possible in non cyclic mode!
 */

int sdo_read_value(Sdo_t *sdo)
{
  switch (sdo->entry_type) {
    case ENTRY_TYPE_BOOLEAN:
    case ENTRY_TYPE_INTEGER8:
      sdo->value = EC_READ_S8(ecrt_sdo_request_data(sdo->request));
      break;
    case ENTRY_TYPE_INTEGER16:
      sdo->value = EC_READ_S16(ecrt_sdo_request_data(sdo->request));
      break;
    case ENTRY_TYPE_INTEGER32:
      sdo->value = EC_READ_S32(ecrt_sdo_request_data(sdo->request));
      break;
    case ENTRY_TYPE_UNSIGNED8:
      sdo->value = EC_READ_U8(ecrt_sdo_request_data(sdo->request));
      break;
    case ENTRY_TYPE_UNSIGNED16:
      sdo->value = EC_READ_U16(ecrt_sdo_request_data(sdo->request));
      break;
    case ENTRY_TYPE_UNSIGNED32:
      sdo->value = EC_READ_U32(ecrt_sdo_request_data(sdo->request));
      break;
    case ENTRY_TYPE_REAL32:
      sdo->value = EC_READ_U32(ecrt_sdo_request_data(sdo->request));
      break;
    case ENTRY_TYPE_VISIBLE_STRING:
      memmove(sdo->value_string, ecrt_sdo_request_data(sdo->request),
              ecrt_sdo_request_data_size(sdo->request));
      break;
    case ENTRY_TYPE_OCTET_STRING:
    case ENTRY_TYPE_UNICODE_STRING:
    case ENTRY_TYPE_TIME_OF_DAY:
    case ENTRY_TYPE_NONE:
    default:
      return ECW_ERROR_SDO_UNSUPORTED_ENTRY_TYPE;
      break;
  }
  return 0;
}

static int sdo_write_value(Sdo_t *sdo)
{
  switch (sdo->entry_type) {
    case ENTRY_TYPE_BOOLEAN:
    case ENTRY_TYPE_INTEGER8:
      EC_WRITE_S8(ecrt_sdo_request_data(sdo->request),
                  (uint8_t )(sdo->value & 0xff));
      break;
    case ENTRY_TYPE_INTEGER16:
      EC_WRITE_S16(ecrt_sdo_request_data(sdo->request),
                   (uint16_t )(sdo->value & 0xffff));
      break;
    case ENTRY_TYPE_INTEGER32:
      EC_WRITE_S32(ecrt_sdo_request_data(sdo->request),
                   (uint32_t )(sdo->value & 0xffffffff));
      break;
    case ENTRY_TYPE_UNSIGNED8:
      EC_WRITE_U8(ecrt_sdo_request_data(sdo->request),
                  (uint8_t )(sdo->value & 0xff));
      break;
    case ENTRY_TYPE_UNSIGNED16:
      EC_WRITE_U16(ecrt_sdo_request_data(sdo->request),
                   (uint16_t )(sdo->value & 0xffff));
      break;
    case ENTRY_TYPE_UNSIGNED32:
      EC_WRITE_U32(ecrt_sdo_request_data(sdo->request),
                   (uint32_t )(sdo->value & 0xffffffff));
      break;
    case ENTRY_TYPE_REAL32:
      EC_WRITE_U32(ecrt_sdo_request_data(sdo->request),
                   (uint32_t )(sdo->value & 0xffffffff));
      break;
    case ENTRY_TYPE_VISIBLE_STRING:
      memmove(ecrt_sdo_request_data(sdo->request), sdo->value_string,
              ecrt_sdo_request_data_size(sdo->request));
      break;
    case ENTRY_TYPE_OCTET_STRING:
    case ENTRY_TYPE_UNICODE_STRING:
    case ENTRY_TYPE_TIME_OF_DAY:
      EC_WRITE_U64(ecrt_sdo_request_data(sdo->request),
                   (uint32_t )(sdo->value & 0xffffffffffff));
      break;
    case ENTRY_TYPE_NONE:
    default:
      return ECW_ERROR_SDO_UNSUPORTED_ENTRY_TYPE;
      break;
  }
  return 0;
}

static int slave_sdo_upload_request(Ethercat_Slave_t *s, Sdo_t *sdo)
{
  int ret = ECW_ERROR_UNKNOWN;

  sdo->request_state = ecrt_sdo_request_state(sdo->request);
  switch (sdo->request_state) {
    case EC_REQUEST_UNUSED:
      // here I can schedule
      ecrt_sdo_request_read(sdo->request);
      sdo->read_request = 1;
      ret = ECW_ERROR_SDO_REQUEST_BUSY;
      break;
    case EC_REQUEST_BUSY:
      // wait
      ret = ECW_ERROR_SDO_REQUEST_BUSY;
      break;
    case EC_REQUEST_SUCCESS:
      if (sdo->read_request == 1) {
        // the request is finished and the data can be read
        ret = sdo_read_value(sdo);
        if (ret != 0) {
          return ret;
        }
        sdo->read_request = 0;
        ret = ECW_SUCCESS;
      } else {
        // this is a new request
        ecrt_sdo_request_read(sdo->request);
        sdo->read_request = 1;
        ret = ECW_ERROR_SDO_REQUEST_BUSY;
      }
      break;
    case EC_REQUEST_ERROR:
      // request failed, what a pity
      sdo->read_request = 0;
      ret = ECW_ERROR_SDO_REQUEST_ERROR;
      break;
  }

  return ret;
}

static int slave_sdo_download_request(Ethercat_Slave_t *s, Sdo_t *sdo)
{
  int ret = ECW_ERROR_UNKNOWN;

  sdo->request_state = ecrt_sdo_request_state(sdo->request);
  switch (sdo->request_state) {
    case EC_REQUEST_UNUSED:
      // here I can schedule
      ret = sdo_write_value(sdo);
      if (ret != 0) {
        return ret;
      }
      ecrt_sdo_request_write(sdo->request);
      ret = ECW_ERROR_SDO_REQUEST_BUSY;
      break;
    case EC_REQUEST_BUSY:
      //wait
      ret = ECW_ERROR_SDO_REQUEST_BUSY;
      break;
    case EC_REQUEST_SUCCESS:
      // we also schedule here for the next request
      ret = sdo_write_value(sdo);
      if (ret != 0) {
        return ret;
      }
      ecrt_sdo_request_write(sdo->request);
      ret = ECW_SUCCESS;
      break;
    case EC_REQUEST_ERROR:
      //error, abort
      ret = ECW_ERROR_SDO_REQUEST_ERROR;
      break;
  }

  return ret;
}

static int slave_sdo_upload_direct(Ethercat_Slave_t *s, Sdo_t *sdo)
{
  size_t result_size = 0;
  uint32_t abort_code = 0;

  switch (sdo->entry_type) {
    case ENTRY_TYPE_BOOLEAN:
    case ENTRY_TYPE_INTEGER8:
    case ENTRY_TYPE_INTEGER16:
    case ENTRY_TYPE_INTEGER32:
    case ENTRY_TYPE_UNSIGNED8:
    case ENTRY_TYPE_UNSIGNED16:
    case ENTRY_TYPE_UNSIGNED32:
    case ENTRY_TYPE_REAL32: {
      int value = 0;
      size_t valuesize = sizeof(value);

      ecrt_master_sdo_upload(s->master, s->info->position, sdo->index,
                             sdo->subindex, (uint8_t *) &value, valuesize,
                             &result_size, &abort_code);

      if (abort_code == 0) {
        sdo->value = value;
      }
      break;
    }
    case ENTRY_TYPE_VISIBLE_STRING: {
      char value_string[ECW_MAX_VISIBLE_STRING_LENGTH];
      size_t valuesize = ECW_MAX_VISIBLE_STRING_LENGTH;

      ecrt_master_sdo_upload(s->master, s->info->position, sdo->index,
                             sdo->subindex, value_string, valuesize,
                             &result_size, &abort_code);

      if (abort_code == 0) {
        memmove(sdo->value_string, value_string, valuesize);
      }
      break;
    }
    case ENTRY_TYPE_OCTET_STRING:
    case ENTRY_TYPE_UNICODE_STRING:
      // NOT IMPLEMENTED
      break;
    case ENTRY_TYPE_TIME_OF_DAY:
      // DO NOTHING - write only
      break;
    case ENTRY_TYPE_NONE:
    default:
      // ERROR - UNKONWN ENTRY TYPE
      return -EINVAL;
      break;
  }

  return abort_code;
}

static int slave_sdo_download_direct(Ethercat_Slave_t *s, Sdo_t *sdo)
{
  uint32_t abort_code = 0;

  switch (sdo->entry_type) {
    case ENTRY_TYPE_BOOLEAN:
    case ENTRY_TYPE_INTEGER8:
    case ENTRY_TYPE_INTEGER16:
    case ENTRY_TYPE_INTEGER32:
    case ENTRY_TYPE_UNSIGNED8:
    case ENTRY_TYPE_UNSIGNED16:
    case ENTRY_TYPE_UNSIGNED32:
    case ENTRY_TYPE_REAL32: {
      int value = sdo->value;
      size_t valuesize = sizeof(value);

      ecrt_master_sdo_download(s->master, s->info->position, sdo->index,
                               sdo->subindex, (const uint8_t *) &value,
                               valuesize, &abort_code);
      break;
    }
    case ENTRY_TYPE_VISIBLE_STRING: {
      ecrt_master_sdo_download(s->master, s->info->position, sdo->index,
                                     sdo->subindex, sdo->value_string,
                                     ECW_MAX_VISIBLE_STRING_LENGTH, &abort_code);
      break;
    }
    case ENTRY_TYPE_OCTET_STRING:
    case ENTRY_TYPE_UNICODE_STRING:
      // NOT IMPLEMENTED
      break;
    case ENTRY_TYPE_TIME_OF_DAY:
      // DO NOTHING - write only
      break;
    case ENTRY_TYPE_NONE:
    default:
      // ERROR - UNKONWN ENTRY TYPE
      return -EINVAL;
      break;
  }

  return abort_code;
}

/*
 * SDO upload and download to slaves
 *
 * If at least one lease is in op mode the master is in the cyclic
 * operation and the direct SDO up-/download may hang up the kernel
 * module. So in cyclic operation the schedule SDO request must be
 * used to be safe.
 */
int slave_sdo_upload(Ethercat_Slave_t *s, Sdo_t *sdo)
{
  ec_master_state_t link_state;
  ecrt_master_state(s->master, &link_state);
  if (link_state.link_up != 1) {
    return ECW_ERROR_LINK_UP;
  }

  if (s->cyclic_mode || (link_state.al_states & 0x8)) {
    return slave_sdo_upload_request(s, sdo);
  }

  return slave_sdo_upload_direct(s, sdo);
}

int slave_sdo_download(Ethercat_Slave_t *s, Sdo_t *sdo)
{
  ec_master_state_t link_state;
  ecrt_master_state(s->master, &link_state);
  if (link_state.link_up != 1) {
    return ECW_ERROR_LINK_UP;
  }

  if (s->cyclic_mode || (link_state.al_states & 0x8)) {
    int ret = slave_sdo_download_request(s, sdo);
    return ret;
  }

  return slave_sdo_download_direct(s, sdo);
}

/*
 * API functions
 */

Ethercat_Slave_t *ecw_slave_init(void)
{
  Ethercat_Slave_t *s = malloc(sizeof(Ethercat_Slave_t));

  return s;
}

void ecw_slave_release(Ethercat_Slave_t *s)
{
  free(s);
}

int ecw_slave_scan(Ethercat_Slave_t *s)
{
  /* rescan baby */
  return -1;
}

#if 0
int ecw_slave_set_pdo(Ethercat_Slave_t *s, size_t pdoindex, pdo_t *value)
{
  s->output_values[pdoindex] = value;
}

pdo_t *ecw_slave_get_pdo(Ethercat_Slave_t *s, size_t pdoindex)
{
  return s->input_values[pdoindex];
}
#endif

int ecw_slave_get_slaveid(Ethercat_Slave_t *s)
{
  return s->info->position;
}

enum eSlaveType ecw_slave_get_type(Ethercat_Slave_t *s)
{
  return s->type;
}

/*
 * PDO handler
 */

int ecw_slave_set_out_value(Ethercat_Slave_t *s, size_t pdoindex, int value)
{
  pdo_t *pdo = ecw_slave_get_outpdo(s, pdoindex);
  pdo->value = value;

  return 0;
}

int ecw_slave_get_in_value(Ethercat_Slave_t *s, size_t pdoindex)
{
  pdo_t *pdo = ecw_slave_get_inpdo(s, pdoindex);
  return pdo->value;
}

int ecw_slave_set_inpdo(Ethercat_Slave_t *s, size_t pdoindex, pdo_t *pdo)
{
  if (pdo->value != (s->input_values + pdoindex)->value
      || pdo->type != (s->input_values + pdoindex)->type
      || pdo->offset != (s->input_values + pdoindex)->offset) {
    memmove((s->input_values + pdoindex), pdo, sizeof(pdo_t));
  }

  return 0;
}

pdo_t *ecw_slave_get_inpdo(Ethercat_Slave_t *s, size_t pdoindex)
{
  return (s->input_values + pdoindex);
}

int ecw_slave_set_outpdo(Ethercat_Slave_t *s, size_t pdoindex, pdo_t *pdo)
{
  if (pdo->value != (s->output_values + pdoindex)->value
      || pdo->type != (s->output_values + pdoindex)->type
      || pdo->offset != (s->output_values + pdoindex)->offset) {
    memmove((s->output_values + pdoindex), pdo, sizeof(pdo_t));
  }

  return 0;
}

pdo_t *ecw_slave_get_outpdo(Ethercat_Slave_t *s, size_t pdoindex)
{
  return (s->output_values + pdoindex);
}

/*
 * SDO handling
 */

size_t ecw_slave_get_sdo_count(Ethercat_Slave_t *s)
{
  return (size_t) s->sdo_count;
}

Sdo_t *ecw_slave_get_sdo(Ethercat_Slave_t *s, int index, int subindex)
{
  for (int i = 0; i < s->sdo_count; i++) {
    Sdo_t *current = s->dictionary + i;
    if (current->index == index && current->subindex == subindex) {
      Sdo_t *sdo = malloc(sizeof(Sdo_t));
      memmove(sdo, current, sizeof(Sdo_t));
      return sdo;
    }
  }

  return NULL;
}

Sdo_t *ecw_slave_get_sdo_index(Ethercat_Slave_t *s, size_t sdoindex)
{
  if (sdoindex >= s->sdo_count) {
    return NULL;
  }

  Sdo_t *sdo = malloc(sizeof(Sdo_t));
  Sdo_t *od = s->dictionary + sdoindex;
  memmove(sdo, od, sizeof(Sdo_t));

  return sdo;
}

int ecw_slave_set_sdo_int_value(Ethercat_Slave_t *s, int index, int subindex,
                                int value)
{
  Sdo_t *current = NULL;

  for (int i = 0; i < s->sdo_count; i++) {
    current = s->dictionary + i;
    if (current->index == index && current->subindex == subindex) {
      current->value = value;
      int err = slave_sdo_download(s, current);
      return err;
    }
  }

  return ECW_ERROR_SDO_NOT_FOUND; /* not found */
}

int ecw_slave_set_sdo_string_value(Ethercat_Slave_t *s, int index, int subindex,
                                   const char *value)
{
  Sdo_t *current = NULL;

  for (int i = 0; i < s->sdo_count; i++) {
    current = s->dictionary + i;
    if (current->index == index && current->subindex == subindex) {
      memmove(current->value_string, value, ECW_MAX_VISIBLE_STRING_LENGTH);
      int err = slave_sdo_download(s, current);
      return err;
    }
  }

  return ECW_ERROR_SDO_NOT_FOUND; /* not found */
}

int ecw_slave_get_sdo_int_value(Ethercat_Slave_t *s, int index, int subindex,
                            int *value)
{
  Sdo_t *sdo = NULL;  //ecw_slave_get_sdo(s, index, subindex);
  for (int i = 0; i < s->sdo_count; i++) {
    Sdo_t *current = s->dictionary + i;
    if (current->index == index && current->subindex == subindex) {
      sdo = current;
      break;
    }
  }

  if (sdo == NULL) {
    return ECW_ERROR_SDO_NOT_FOUND; /* Not found */
  }

  int err = slave_sdo_upload(s, sdo);
  if (err == 0) {
    *value = sdo->value;
  }

  return err;
}

int ecw_slave_get_sdo_string_value(Ethercat_Slave_t *s, int index, int subindex,
                                   char *value)
{
  Sdo_t *sdo = NULL;
  for (int i = 0; i < s->sdo_count; i++) {
    Sdo_t *current = s->dictionary + i;
    if (current->index == index && current->subindex == subindex) {
      sdo = current;
      break;
    }
  }

  if (sdo == NULL) {
    return ECW_ERROR_SDO_NOT_FOUND; /* Not found */
  }

  int err = slave_sdo_upload(s, sdo);
  if (err == 0) {
    memmove(value, sdo->value_string, ECW_MAX_VISIBLE_STRING_LENGTH);
  }

  return err;
}

int ecw_slave_get_info(Ethercat_Slave_t *s, Ethercat_Slave_Info_t *info)
{
  if (info == NULL || s == NULL) {
    return -1;
  }

  info->position = s->info->position;
  info->vendor_id = s->info->vendor_id;
  info->product_code = s->info->product_code;
  info->revision_number = s->info->revision_number;
  info->serial_number = s->info->serial_number;
  info->sync_manager_count = s->info->sync_count;
  info->sdo_count = s->info->sdo_count;
  strncpy(info->name, s->info->name, EC_MAX_STRING_LENGTH);

  return 0;
}

char *ecw_slave_type_string(enum eSlaveType type)
{
  char *typestring;

  switch (type) {
    case SLAVE_TYPE_CIA402_DRIVE:
      typestring = "CiA402 Drive";
      break;
    case SLAVE_TYPE_DIGITAL_IO:
    case SLAVE_TYPE_ECATIO: /* FIXME remove because it's DEPRECATED */
      typestring = "Digital I/O";
      break;
    case SLAVE_TYPE_UNKNOWN:
    default:
      typestring = "Unknown";
      break;
  }

  return typestring;
}

enum eSlaveType type_map_get_type(uint32_t vendor, uint32_t product)
{
  for (int i = 0; type_map[i].vendor_id != 0; i++) {
    if (type_map[i].vendor_id == vendor
        && type_map[i].product_code == product) {
      return type_map[i].type;
    }
  }

  return SLAVE_TYPE_UNKNOWN;
}

enum eALState ecw_slave_get_current_state(Ethercat_Slave_t *s)
{
  unsigned int raw = s->state.al_state;
  enum eALState state = ALSTATE_INIT;

  switch (raw) {
    case 1:
      state = ALSTATE_INIT;
      break;
    case 2:
      state = ALSTATE_PREOP;
      break;
    case 3:
      state = ALSTATE_BOOT;
      break;
    case 4:
      state = ALSTATE_SAFEOP;
      break;
    case 8:
      state = ALSTATE_OP;
      break;
  }

  return state;
}
