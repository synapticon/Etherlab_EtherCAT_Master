/*
 * ethercat_wrapper_slave.h
 *
 * Module to handle a single ethercat slave.
 */

#ifndef ETHERCAT_WRAPPER_SLAVE_H
#define ETHERCAT_WRAPPER_SLAVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ecrt.h"
#include <stdlib.h>
#include <stdint.h>

/**
 * Get the string length from the ecrt library
 */
#define ECW_MAX_STRING_LENGTH           EC_MAX_STRING_LENGTH

#define ECW_MAX_VISIBLE_STRING_LENGTH   50

/**
 * Get the size of SDO access rights field from the ecrt library
 */
#define ECW_SDO_ENTRY_ACCESS_COUNTER   EC_SDO_ENTRY_ACCESS_COUNTER

/**
 * \brief Supported slave types enumerator
 *
 * \see ecw_slave_get_type
 */
enum eSlaveType {
  SLAVE_TYPE_UNKNOWN = 0, /**< default unknown type */
  SLAVE_TYPE_CIA402_DRIVE, /**< Somanet motor drive with CiA402 protocol */
  SLAVE_TYPE_DIGITAL_IO, /**< SomanetDigital I/O board */
  SLAVE_TYPE_ENDEFFECTOR_IO, /**< MABI Endeffector board */
  SLAVE_TYPE_ECATIO /* FIXME remove because it's DEPRECATED */
};

/**
 * \brief Object types for object dictionary entries
 */
enum eObjectType {
  OBJECT_TYPE_UNDEF = 0, /**< unknown/undefined type */
  OBJECT_TYPE_DOMAIN = 2,
  OBJECT_TYPE_DEFTYPE = 5,
  OBJECT_TYPE_DEFSTRUCT = 6,
  OBJECT_TYPE_VAR = 7, /**< Type for simple variable */
  OBJECT_TYPE_ARRAY = 8, /**< Complex type array */
  OBJECT_TYPE_RECORD = 9/**< Complex type record */
};

enum eEntryType {
  ENTRY_TYPE_NONE = 0,
  ENTRY_TYPE_BOOLEAN,           // 1: BOOL/BIT - Boolean - 1 bit
  ENTRY_TYPE_INTEGER8,          // 2: SINT - Short integer - 8 bits
  ENTRY_TYPE_INTEGER16,         // 3: INT - Integer - 16 bits
  ENTRY_TYPE_INTEGER32,         // 4: DINT - Double integer - 32 bits
  ENTRY_TYPE_UNSIGNED8,         // 5: USINT - Unsigned short integer - 8 bits
  ENTRY_TYPE_UNSIGNED16,        // 6: UINT - Unsigned integer - 16 bits
  ENTRY_TYPE_UNSIGNED32,        // 7: UDINT - Unsigned double integer - 32 bits
  ENTRY_TYPE_REAL32,            // 8: REAL32 - Floating point - 32 bits
  ENTRY_TYPE_VISIBLE_STRING,    // 9: STRING(n) - Visible string - 8^n bits
  ENTRY_TYPE_OCTET_STRING,      // 10: ARRAY[0..n] OF BYTE - Sequence of octets - 8^(n+1) bits
  ENTRY_TYPE_UNICODE_STRING,    // 11: ARRAY[0..n] OF UINT - Sequence of UINT - 16^(n+1) bits
  ENTRY_TYPE_TIME_OF_DAY        // 12: Time of day - 48 bits
};

/**
 * \brief Application layer states
 *
 * FIXME according to the API of the libethercat the BOOTSTRAP mode is not
 * represented here. If the bootloader is not used with the motion master
 * this is not a problem.
 */
enum eALState {
  ALSTATE_INIT = 1,
  ALSTATE_PREOP,
  ALSTATE_BOOT,
  ALSTATE_SAFEOP,
  ALSTATE_OP
};

/**
 * \brief EtherCAT slave type
 */
typedef struct _ecw_slave_t Ethercat_Slave_t;

/**
 * \brief Type for PDO values
 */
typedef struct {
  int value;
  unsigned int offset;
  unsigned int bit_offset;
  enum eEntryType type;
} pdo_t;

/**
 * \brief Type for SDO values
 *
 * Every entry in the slaves object dictionary has at least this type
 */
typedef struct _sdo_t {
  uint16_t index;
  uint8_t subindex;
  uint64_t value;
  char value_string[ECW_MAX_VISIBLE_STRING_LENGTH];
  int bit_length;
  enum eObjectType object_type;
  enum eEntryType entry_type;
  char name[EC_MAX_STRING_LENGTH];
  char object_name[EC_MAX_STRING_LENGTH];
  uint8_t read_access[EC_SDO_ENTRY_ACCESS_COUNTER];
  uint8_t write_access[EC_SDO_ENTRY_ACCESS_COUNTER];

  ec_sdo_request_t *request;
  ec_request_state_t request_state;
} Sdo_t;

/**
 * \brief Slave information type
 */
typedef struct {
  int position; /**< Position of the slave on the bus */
  uint32_t vendor_id; /**< Vendor ID of the device (assigned by ETG) */
  uint32_t product_code; /**< Product code of the device */
  uint32_t revision_number; /**< Revision number of the device */
  uint32_t serial_number; /**< Serial number of the device */
  uint8_t sync_manager_count; /**< Number of Sync Manager */
  uint8_t sdo_count; /**< number of SDO objects */
  char name[EC_MAX_STRING_LENGTH]; /**< String name of the device */
} Ethercat_Slave_Info_t;

/**
 * \brief A device is described by this type
 *
 * A device is described by the Vendor ID and the Product Code.
 *
 * \see ecw_slave_get_type
 */
typedef struct {
  uint32_t vendor_id; /**< Vendor ID of the device (assigned by ETG) */
  uint32_t product_code; /**< Product code of the device */
  uint32_t revision_number; /**< Revision number of the device */
  enum eSlaveType type; /**< Type of the device (if available) */
} Device_type_map_t;

/* FIXME this seem to be unused / unnecessary */
Ethercat_Slave_t *ecw_slave_init(void);
void ecw_slave_release(Ethercat_Slave_t *);

/**
 * \brief Request the slave id
 *
 * The slave id used here is the position of the slave on the bus.
 *
 * \param slave   Slave object to request
 * \return the id of the slave
 */
int ecw_slave_get_slaveid(const Ethercat_Slave_t *s);

/**
 * \brief Request the type of the slave
 *
 * \param slave   Slave object to request
 * \return the type of the slave \see eSlaveType
 */
enum eSlaveType ecw_slave_get_type(const Ethercat_Slave_t *s);

/**
 * \brief Request the current application layer state of the slave
 *
 * This function is useful in cyclic operation to monitor the current
 * state of the slaves on the bus.
 *
 * \param s   slave object to request
 * \return Current state of the slave \see eALState
 */
enum eALState ecw_slave_get_current_state(const Ethercat_Slave_t *s);

/*
 * FoE
 */

int ecw_slave_read_file(const Ethercat_Slave_t *s, const char* file_name,
                        uint8_t *content, size_t *size);

int ecw_slave_write_file(const Ethercat_Slave_t *s, const char* file_name,
                         const uint8_t *content, size_t size);

/*
 * SII
 */
int ecw_slave_write_sii(const Ethercat_Slave_t *s, const uint8_t *content,
                        size_t size);

/*
 * PDO access functions
 */

/**
 * \brief Set the value of a specific PDO
 *
 * \param slave     slave the PDO is associated
 * \param pdoindex  index of the PDO in the buffer
 * \param value     value to set
 * \return 0 on success
 */
int ecw_slave_set_out_value(const Ethercat_Slave_t *s, size_t pdoindex,
                            int value);

/**
 * \brief Get the value of a specific PDO
 *
 * \param slave     slave the PDO is associated
 * \param pdoindex  index of the PDO in the buffer
 * \return the value of the requested PDO
 */
int ecw_slave_get_in_value(const Ethercat_Slave_t *s, size_t pdoindex);

/*
 * SDO Access Functions
 */

/**
 * \brief Set the (number) value of the given object
 *
 * \param slave    Slave to request
 * \param index    Index of the object
 * \param subindex Subindex of the object
 * \param value    Value to write to the object
 * \return 0 on success
 */
int ecw_slave_set_sdo_int_value(const Ethercat_Slave_t *s, int index,
                                int subindex, uint64_t value);

/**
 * \brief Set the string value of the given object
 *
 * \param slave    Slave to request
 * \param index    Index of the object
 * \param subindex Subindex of the object
 * \param value    Value to write to the object
 * \return 0 on success
 */
int ecw_slave_set_sdo_string_value(const Ethercat_Slave_t *s, int index,
                                   int subindex, const char *value);

/**
 * \brief Request the current value of one (number) object
 *
 * \param slave    Slave to request
 * \param index    Index of the object
 * \param subindex Subindex of the object
 * \param *value   Pointer to variable to store requested value
 * \return 0 on success
 */
int ecw_slave_get_sdo_int_value(const Ethercat_Slave_t *s, int index,
                                int subindex, int *value);

/**
 * \brief Request the current value of one (string) object
 *
 * \param slave    Slave to request
 * \param index    Index of the object
 * \param subindex Subindex of the object
 * \param *value   Pointer to variable to store requested value
 * \return 0 on success
 */
int ecw_slave_get_sdo_string_value(const Ethercat_Slave_t *s, int index,
                                   int subindex, char *value);

/**
 * \brief Set the (number) value of the given object
 *
 * \param slave    Slave to request
 * \param sdo      SDO to use
 * \param value    Value to write to the object
 * \return 0 on success
 */
int ecw_slave_set_int_value(const Ethercat_Slave_t *s, Sdo_t *sdo,
                            uint64_t value);

/**
 * \brief Set the string value of the given object
 *
 * \param slave    Slave to request
 * \param sdo      SDO to use
 * \param value    Value to write to the object
 * \return 0 on success
 */
int ecw_slave_set_string_value(const Ethercat_Slave_t *s, Sdo_t *sdo,
                               const char *value);

/**
 * \brief Request the current value of one (number) object
 *
 * \param slave    Slave to request
 * \param sdo      SDO to use
 * \param *value   Pointer to variable to store requested value
 * \return 0 on success
 */
int ecw_slave_get_int_value(const Ethercat_Slave_t *s, Sdo_t *sdo,
                            int *value);

/**
 * \brief Request the current value of one (string) object
 *
 * \param slave    Slave to request
 * \param sdo      SDO to use
 * \param *value   Pointer to variable to store requested value
 * \return 0 on success
 */
int ecw_slave_get_string_value(const Ethercat_Slave_t *s, Sdo_t *sdo,
                               char *value);

/**
 * \brief Get object with index and subindex from object dictionary
 *
 * Create a copy of the object dictionary object at position `sdoindex` and
 * return this to the user.
 *
 * IMPORTANT: The calling application must take care to clean up this object.
 *
 * \param slave    Slave to request
 * \param index    Index of the requested object
 * \param subindex Subindex of the requested object
 * \return NULL if not available or copy of object \see Sdo_t
 */
Sdo_t *ecw_slave_get_sdo(const Ethercat_Slave_t *s, int index, int subindex);

/**
 * \brief Get object of the object dictionary position sdoindex
 *
 * Create a copy of the object dictionary object ad position `sdoindex` and
 * return this to the user.
 *
 * IMPORTANT: The calling application must take care to clean up this object.
 *
 * \param slave     Slave to request
 * \param sdoindex  index of the SDO in the object dictionary
 * \return NULL if not available or copy of object \see Sdo_t
 */
Sdo_t *ecw_slave_get_sdo_index(const Ethercat_Slave_t *s, size_t sdoindex);

/**
 * \brief Get a pointer to the original object of the object dictionary position sdoindex
 *
 * \param slave     Slave to request
 * \param sdoindex  index of the SDO in the object dictionary
 * \return NULL if not available or a pointer to an object \see Sdo_t
 */
Sdo_t *ecw_slave_get_sdo_pointer(const Ethercat_Slave_t *s, size_t sdoindex);

/**
 * \brief low level access to write SDO value to slave
 *
 * WARNING: this function does not automatically update the value of the SDO in
 * the local object dictionary.
 *
 * \param slave  Slave to request
 * \param sdo    SDO object with new value to write to device
 * \return 0 on success
 */
int slave_sdo_download(const Ethercat_Slave_t *s, Sdo_t *sdo);

/**
 * \brief low level access to read SDO value to slave
 *
 * WARNING: this function does not automatically update the value of the SDO in
 * the local object dictionary.
 *
 * \param slave  Slave to request
 * \param sdo    SDO object to update the value
 * \return 0 on success
 */
int slave_sdo_upload(const Ethercat_Slave_t *s, Sdo_t *sdo);

/* deprecated or not used */
int ecw_slave_get_sdo_list(Ethercat_Slave_t *s, int *index_list);

/**
 * \brief Request the number of all SDOs in the slaves object dictionary
 *
 * \param slave   Slave to request
 * \return number of all objects in the object dictionary
 */
size_t ecw_slave_get_sdo_count(const Ethercat_Slave_t *s);

/**
 * \brief Get slave information
 *
 * \param slave  Slave to request
 * \param info   reference to slave information \see Ethercat_Slave_Info_t
 * \return 0, != 0 on error
 */
int ecw_slave_get_info(const Ethercat_Slave_t *slave,
                       Ethercat_Slave_Info_t *info);

/*
 * \brief Get slave type string
 *
 * \param type  the type of the slave \see type_map_get_type
 * \return c string of the requested type
 */
char *ecw_slave_type_string(enum eSlaveType type);

/*
 * \brief Get the slave type
 *
 * Get the type of the slave with the given vendor id and product id.
 *
 * \param vendor    the vendor id of the slave
 * \param product   the product number of the slave
 * \return the type of the slave \see eSlaveType
 */
enum eSlaveType type_map_get_type(uint32_t vendor, uint32_t product);

#ifdef __cplusplus
}
#endif

#endif /* ETHERCAT_WRAPPER_SLAVE_H */
