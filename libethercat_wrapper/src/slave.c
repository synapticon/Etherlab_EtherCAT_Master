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

/* list of supported EtherCAT slaves */
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
    case ENTRY_TYPE_VISIBLE_STRING: {
      size_t data_size = ecrt_sdo_request_data_size(sdo->request);

      memmove(sdo->value_string, ecrt_sdo_request_data(sdo->request),
              data_size);

      // Null-terminate the string
      sdo->value_string[data_size] = '\0';
      break;
    }
    case ENTRY_TYPE_OCTET_STRING: {
      size_t data_size = ecrt_sdo_request_data_size(sdo->request);

      memmove(sdo->value_string, ecrt_sdo_request_data(sdo->request),
              data_size);

      // Null-terminate the string
      sdo->value_string[data_size] = '\0';
      break;
    }
    case ENTRY_TYPE_UNICODE_STRING:
    case ENTRY_TYPE_TIME_OF_DAY:
    case ENTRY_TYPE_NONE:
    default:
      return ECW_ERROR_SDO_UNSUPPORTED_ENTRY_TYPE;
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
      memmove(ecrt_sdo_request_data(sdo->request), sdo->value_string,
                    ecrt_sdo_request_data_size(sdo->request));
      break;
    case ENTRY_TYPE_UNICODE_STRING:
    case ENTRY_TYPE_TIME_OF_DAY:
      EC_WRITE_U64(ecrt_sdo_request_data(sdo->request),
                   sdo->value & 0xffffffffffff);
      break;
    case ENTRY_TYPE_NONE:
    default:
      return ECW_ERROR_SDO_UNSUPPORTED_ENTRY_TYPE;
      break;
  }
  return 0;
}

static int slave_sdo_upload_request(Sdo_t *sdo)
{
  // Check if the request is a valid pointer
  if (!sdo->request) {
    return ECW_ERROR_SDO_REQUEST_INVALID;
  }

  // If the previous request succeeded or failed, schedule a new one
  if (sdo->request_state == EC_REQUEST_SUCCESS
      || sdo->request_state == EC_REQUEST_ERROR) {
    ecrt_sdo_request_read(sdo->request);
    sdo->request_state = EC_REQUEST_BUSY;
    return ECW_ERROR_SDO_REQUEST_BUSY;
  }

  int ret = ECW_ERROR_UNKNOWN;

  sdo->request_state = ecrt_sdo_request_state(sdo->request);
  switch (sdo->request_state) {
    case EC_REQUEST_UNUSED:
      // here I can schedule
      ecrt_sdo_request_read(sdo->request);
      ret = ECW_ERROR_SDO_REQUEST_BUSY;
      break;
    case EC_REQUEST_BUSY:
      // wait
      ret = ECW_ERROR_SDO_REQUEST_BUSY;
      break;
    case EC_REQUEST_SUCCESS:
      // the request is finished and the data can be read
      ret = sdo_read_value(sdo);
      if (ret != 0) {
        return ret;
      }
      ret = ECW_SUCCESS;
      break;
    case EC_REQUEST_ERROR:
      // request failed, what a pity
      ret = ECW_ERROR_SDO_REQUEST_ERROR;
      break;
  }

  return ret;
}

static int slave_sdo_download_request(Sdo_t *sdo)
{
  // Check if the request is a valid pointer
  if (!sdo->request) {
    return ECW_ERROR_SDO_REQUEST_INVALID;
  }

  // If the previous request succeeded or failed, schedule a new one first
  if (sdo->request_state == EC_REQUEST_SUCCESS
      || sdo->request_state == EC_REQUEST_ERROR) {
    int ret = sdo_write_value(sdo);
    if (ret != 0) {
      return ret;
    }
    ecrt_sdo_request_write(sdo->request);
    sdo->request_state = EC_REQUEST_BUSY;
    return ECW_ERROR_SDO_REQUEST_BUSY;
  }

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
      ret = ECW_SUCCESS;
      break;
    case EC_REQUEST_ERROR:
      ret = ECW_ERROR_SDO_REQUEST_ERROR;
      break;
  }

  return ret;
}

static int slave_sdo_upload_direct(const Ethercat_Slave_t *s, Sdo_t *sdo)
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
      size_t value_size = sizeof(value);

      ecrt_master_sdo_upload(s->master, s->info->position, sdo->index,
                             sdo->subindex, (uint8_t *) &value, value_size,
                             &result_size, &abort_code);

      if (abort_code == 0) {
        sdo->value = value;
      }
      break;
    }
    case ENTRY_TYPE_VISIBLE_STRING: {
      char value_string[ECW_MAX_STRING_LENGTH];
      size_t value_size = sdo->bit_length / 8;

      ecrt_master_sdo_upload(s->master, s->info->position, sdo->index,
                             sdo->subindex, (uint8_t *) value_string, value_size,
                             &result_size, &abort_code);

      if (abort_code == 0) {
        memmove(sdo->value_string, value_string, result_size);

        // Null-terminate the string
        sdo->value_string[result_size] = '\0';
      }
      break;
    }
    case ENTRY_TYPE_OCTET_STRING: {
      char value_string[ECW_MAX_STRING_LENGTH];
      size_t value_size = sdo->bit_length / 8;

      ecrt_master_sdo_upload(s->master, s->info->position, sdo->index,
                             sdo->subindex, (uint8_t *) value_string, value_size,
                             &result_size, &abort_code);

      if (abort_code == 0) {
        memmove(sdo->value_string, value_string, result_size);

        // Null-terminate the string
        sdo->value_string[result_size] = '\0';
      }
      break;
    }
    case ENTRY_TYPE_UNICODE_STRING:
      // NOT IMPLEMENTED
      break;
    case ENTRY_TYPE_TIME_OF_DAY:
      // DO NOTHING - write only
      break;
    case ENTRY_TYPE_NONE:
    default:
      // ERROR - UNKNOWN ENTRY TYPE
      return -EINVAL;
      break;
  }

  return abort_code;
}

static int slave_sdo_download_direct(const Ethercat_Slave_t *s, Sdo_t *sdo)
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
      size_t value_size = sizeof(value);

      ecrt_master_sdo_download(s->master, s->info->position, sdo->index,
                               sdo->subindex, (const uint8_t *) &value,
                               value_size, &abort_code);
      break;
    }
    case ENTRY_TYPE_VISIBLE_STRING: {
      size_t value_size = sdo->bit_length / 8;
      ecrt_master_sdo_download(s->master,
                               s->info->position,
                               sdo->index,
                               sdo->subindex,
                               (const uint8_t *) sdo->value_string,
                               value_size,
                               &abort_code);
      break;
    }
    case ENTRY_TYPE_OCTET_STRING: {
      size_t value_size = sdo->bit_length / 8;
      ecrt_master_sdo_download(s->master,
                               s->info->position,
                               sdo->index,
                               sdo->subindex,
                               (const uint8_t *)sdo->value_string,
                               value_size,
                               &abort_code);
      break;
    }
    case ENTRY_TYPE_UNICODE_STRING:
      // NOT IMPLEMENTED
      break;
    case ENTRY_TYPE_TIME_OF_DAY: {
      uint64_t value = sdo->value;
      size_t value_size = sizeof(value);

      ecrt_master_sdo_download(s->master, s->info->position, sdo->index,
                               sdo->subindex, (const uint8_t *) &value,
                               value_size, &abort_code);
      break;
    }
    case ENTRY_TYPE_NONE:
    default:
      // ERROR - UNKNOWN ENTRY TYPE
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
int slave_sdo_upload(const Ethercat_Slave_t *s, Sdo_t *sdo)
{
  ec_master_state_t link_state;
  ecrt_master_state(s->master, &link_state);
  if (link_state.link_up != 1) {
    return ECW_ERROR_LINK_UP;
  }

  if (s->cyclic_mode || (link_state.al_states & 0x8)) {
    return slave_sdo_upload_request(sdo);
  }

  return slave_sdo_upload_direct(s, sdo);
}

int slave_sdo_download(const Ethercat_Slave_t *s, Sdo_t *sdo)
{
  ec_master_state_t link_state;
  ecrt_master_state(s->master, &link_state);
  if (link_state.link_up != 1) {
    return ECW_ERROR_LINK_UP;
  }

  if (s->cyclic_mode || (link_state.al_states & 0x8)) {
    int ret = slave_sdo_download_request(sdo);
    return ret;
  }

  return slave_sdo_download_direct(s, sdo);
}

/*
 * API functions
 */
int ecw_slave_get_slave_id(const Ethercat_Slave_t *s)
{
  return s->info->position;
}

enum eSlaveType ecw_slave_get_type(const Ethercat_Slave_t *s)
{
  return s->type;
}

/*
 * PDO handler
 */

int ecw_slave_set_out_value(const Ethercat_Slave_t *s, size_t pdo_index,
                            int value)
{
  pdo_t *pdo = ecw_slave_get_out_pdo(s, pdo_index);
  pdo->value = value;

  return 0;
}

int ecw_slave_get_in_value(const Ethercat_Slave_t *s, size_t pdo_index)
{
  pdo_t *pdo = ecw_slave_get_in_pdo(s, pdo_index);
  return pdo->value;
}

int ecw_slave_set_in_pdo(const Ethercat_Slave_t *s, size_t pdo_index,
                         pdo_t *pdo)
{
    if (pdo->value != (s->input_values + pdo_index)->value ||
        pdo->type != (s->input_values + pdo_index)->type ||
        pdo->offset != (s->input_values + pdo_index)->offset) {
        memmove((s->input_values + pdo_index), pdo, sizeof(pdo_t));
    }

    return 0;
}

pdo_t *ecw_slave_get_in_pdo(const Ethercat_Slave_t *s, size_t pdo_index)
{
  return (s->input_values + pdo_index);
}

int ecw_slave_set_out_pdo(const Ethercat_Slave_t *s, size_t pdo_index,
                          pdo_t *pdo)
{
    if (pdo->value != (s->output_values + pdo_index)->value ||
        pdo->type != (s->output_values + pdo_index)->type ||
        pdo->offset != (s->output_values + pdo_index)->offset) {
        memmove((s->output_values + pdo_index), pdo, sizeof(pdo_t));
    }

    return 0;
}

pdo_t *ecw_slave_get_out_pdo(const Ethercat_Slave_t *s, size_t pdo_index)
{
  return (s->output_values + pdo_index);
}

/*
 * SDO handling
 */

size_t ecw_slave_get_sdo_count(const Ethercat_Slave_t *s)
{
  return (size_t) s->sdo_count;
}

Sdo_t *ecw_slave_get_sdo(const Ethercat_Slave_t *s, int index, int subindex)
{
  for (size_t i = 0; i < s->sdo_count; i++) {
    Sdo_t *current = s->dictionary + i;
    if (current->index == index && current->subindex == subindex) {
      Sdo_t *sdo = malloc(sizeof(Sdo_t));
      memmove(sdo, current, sizeof(Sdo_t));
      return sdo;
    }
  }

  return NULL;
}

Sdo_t *ecw_slave_get_sdo_index(const Ethercat_Slave_t *s, size_t sdo_index)
{
  if (sdo_index >= s->sdo_count) {
    return NULL;
  }

  Sdo_t *sdo = malloc(sizeof(Sdo_t));
  Sdo_t *od = s->dictionary + sdo_index;
  memmove(sdo, od, sizeof(Sdo_t));

  return sdo;
}

Sdo_t *ecw_slave_get_sdo_pointer(const Ethercat_Slave_t *s, size_t sdo_index)
{
  if (sdo_index >= s->sdo_count) {
    return NULL;
  }

  return s->dictionary + sdo_index;
}

/*
 * SDO value handling - using index/subindex
 */

int ecw_slave_set_sdo_int_value(const Ethercat_Slave_t *s, int index,
                                int subindex, uint64_t value)
{
  Sdo_t *sdo = NULL;
  for (size_t i = 0; i < s->sdo_count; i++) {
    Sdo_t *current = s->dictionary + i;
    if (current->index == index && current->subindex == subindex) {
      sdo = current;
      break;
    }
  }

  if (sdo == NULL) {
    return ECW_ERROR_SDO_NOT_FOUND; /* Not found */
  }

  sdo->value = value;
  int err = slave_sdo_download(s, sdo);

  return err;
}

int ecw_slave_set_sdo_string_value(const Ethercat_Slave_t *s, int index,
                                   int subindex, const char *value)
{
  Sdo_t *sdo = NULL;
  for (size_t i = 0; i < s->sdo_count; i++) {
    Sdo_t *current = s->dictionary + i;
    if (current->index == index && current->subindex == subindex) {
      sdo = current;
      break;
    }
  }

  if (sdo == NULL) {
    return ECW_ERROR_SDO_NOT_FOUND; /* Not found */
  }

  memmove(sdo->value_string, value, sdo->bit_length / 8);

  int err = slave_sdo_download(s, sdo);

  return err;
}

int ecw_slave_get_sdo_int_value(const Ethercat_Slave_t *s, int index,
                                int subindex, int *value)
{
  Sdo_t *sdo = NULL;  //ecw_slave_get_sdo(s, index, subindex);
  for (size_t i = 0; i < s->sdo_count; i++) {
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

int ecw_slave_get_sdo_string_value(const Ethercat_Slave_t *s, int index,
                                   int subindex, char *value)
{
  Sdo_t *sdo = NULL;
  for (size_t i = 0; i < s->sdo_count; i++) {
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
    memmove(value, sdo->value_string, sdo->bit_length / 8);
  }

  return err;
}

/*
 * SDO value handling - using SDO pointer
 */

int ecw_slave_set_int_value(const Ethercat_Slave_t *s, Sdo_t *sdo,
                                uint64_t value)
{
  if (sdo == NULL) {
    return ECW_ERROR_SDO_NOT_FOUND;
  }

  sdo->value = value;
  return slave_sdo_download(s, sdo);
}

int ecw_slave_set_string_value(const Ethercat_Slave_t *s, Sdo_t *sdo,
                                   const char *value)
{
  if (sdo == NULL) {
    return ECW_ERROR_SDO_NOT_FOUND;
  }

  memmove(sdo->value_string, value, sdo->bit_length / 8);

  return slave_sdo_download(s, sdo);
}

int ecw_slave_get_int_value(const Ethercat_Slave_t *s, Sdo_t *sdo,
                                int *value)
{
  if (sdo == NULL) {
    return ECW_ERROR_SDO_NOT_FOUND;
  }

  int err = slave_sdo_upload(s, sdo);
  if (err == 0) {
    *value = sdo->value;
  }

  return err;
}

int ecw_slave_get_string_value(const Ethercat_Slave_t *s, Sdo_t *sdo,
                                   char *value)
{
  if (sdo == NULL) {
    return ECW_ERROR_SDO_NOT_FOUND;
  }

  int err = slave_sdo_upload(s, sdo);
  if (err == 0) {
    memmove(value, sdo->value_string, sdo->bit_length / 8);
  }

  return err;
}

int ecw_slave_get_info(const Ethercat_Slave_t *s, Ethercat_Slave_Info_t *info)
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

enum eALState ecw_slave_get_current_state(const Ethercat_Slave_t *s)
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

int ecw_slave_read_file(const Ethercat_Slave_t *s, const char* file_name,
                        uint8_t *content, size_t *size)
{
  /**
   * IMPORTANT: The master code doesn't seem to allow reading larger files and
   * the offset is never used, so there is absolutely no possibility of reading
   * larger files even with sequential reading.
   */
  return ecrt_master_read_foe(s->master, s->relative_position, file_name,
                              content, size);
}

int ecw_slave_write_file(const Ethercat_Slave_t *s, const char* file_name,
                         const uint8_t *content, size_t size)
{
  return ecrt_master_write_foe(s->master, s->relative_position, file_name,
                               content, size);
}

int ecw_slave_write_sii(const Ethercat_Slave_t *s, const uint8_t *content,
                        size_t size)
{
  return ecrt_master_write_sii(s->master, s->relative_position, content, size);
}

