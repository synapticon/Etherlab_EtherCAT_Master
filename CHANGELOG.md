# Changelog
All notable changes to this project made by SYNAPTICON will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/).

##[Unreleased]
### EtherCAT wrapper library
#### Added
- Enable reading/writing of string SDO values
- Enable handling 64-bit integer values and setting the time of day
- Check if the SDO request pointers are valid on upload/download
- Add a function for bus rescanning
- Add a function for preemptive bus rescanning
- Enable preemptive slave vendor ID and product code fetching
- Add a Synapticon changelog

#### Fixed
- Always null-terminate the string SDO values
- Fix the domain registration during master initialization
- Free the master domain in the proper place, when stopping the master
- Fix segmentation fault on ecw_master_stop

### EtherCAT library
#### Added
- Add a function to trigger bus rescanning

#### Fixed
- Suport Linux kernel 4.15.7
- Fix: sncn_installer uses right sysconfig

## [1.5.2-sncn-6] - 2018-10-23
### EtherCAT wrapper library (v0.6)
#### Added
- Add version for libethercat_wrapper to distribution
- Use syslog for logging
- Add the object name to the SDO structure
- Enable fetching some slave information before EtherCAT master init
- Enable the wrapper to update slave states (ecw_master_scan)
- Define code errors

#### Changed
- Change order of PDOs in domain reg - input, then output
- General refactor
- Improve slave clean up
- Define the SDO entry type as an enum
- Process SDOs according to their entry type on direct upload

#### Removed
- Remove the master info pointer from the wrapper master struct
- Remove the unused and redundant alias variable from the slave struct

#### Fixed
- Always refresh the number of responding slaves
- Fix the slave ID check
- Fix SDO read/write while in OP state
- Fix getting/setting of the slave state - add BOOT state
- Fix: Return NULL if the SDO is not found
- Various test fixes
- Fix boolean SDO read/write
- Fix the usage of slave aliases

### EtherCAT library
#### Added
- Add custom install script
- Support 4.13 and 4.15 kernels
- Add gitlog tracking to startup kernel logging

#### Changed
- Disable 8139too by default
- Reduce waiting period for SDOs

#### Fixed
- Fix for vm_fault struct change in 4.10 kernel
- Fix for overlapping state machine operations
- Fix single retries variable when multiple slaves exist

## [1.5.2-sncn-5] - 2017-04-20
### EtherCAT wrapper library (v0.5)
#### Added
- Add setting of slave operational state
- Add usage of AL state enum for state request

#### Changed
- Improve AL state handling

#### Removed
- Remove initial upload of entry values

#### Fixed
- Fix domain registration issues
- Fix byte alignement handling for PDOs
- Fix master start and stop
- Fix setup of SDO requests
- Fix type warnings for PDO mappings

### EtherCAT library
#### Changed
- Improve AL state handling

## [1.5.2-sncn-4] - 2017-02-14
### EtherCAT wrapper library
#### Added
- Add and integrate into build process

### EtherCAT library
#### Added
- Add object code to SDO information object

### EtherCAT drivers
#### Fixed
- Fix e1000e update script

## [1.5.2-sncn-3] - 2016-12-16
### EtherCAT wrapper library
#### Added
- Add SDO Info access to libethercat

### Infrastructure
#### Fixed
- Fix interface name parsing

## [1.5.2-sncn-2] - 2016-11-15
### EtherCAT library
#### Changed
- Add BOOT state to exception (only FoE allowed)

#### Fixed
- Fix interface detection in installer script
- Fixed fragmented SoE write request
- Prevent CCAT auto-loading

## [1.5.2-sncn-1] - 2016-05-04
### EtherCAT library
#### Added
- Add additional README on how to build a release
- Add construction of Synapticon EtherCAT master installer
- Add ccat driver 0.14
- Add 3.18 driver for EtherCAT

#### Changed
- Change version gathering from 'hg id' to 'git describe'
- Increased timeout value to be able to initialize AX5206

#### Removed
- Remove e1000e driver from configuration

#### Fixed
- Updated the driver to cope with NVM checksum problem
- Fix TIMED UNMATCH warning
- Fix compilation on kernels > 4.2.0
- Fix FoE timeout calculation bug

[Unreleased]: https://github.com/synapticon/Etherlab_EtherCAT_Master/compare/v1.5.2-sncn-6...HEAD
[1.5.2-sncn-6]: https://github.com/synapticon/Etherlab_EtherCAT_Master/compare/v1.5.2-sncn-5...v1.5.2-sncn-6
[1.5.2-sncn-5]: https://github.com/synapticon/Etherlab_EtherCAT_Master/compare/v1.5.2-sncn-4...v1.5.2-sncn-5
[1.5.2-sncn-4]: https://github.com/synapticon/Etherlab_EtherCAT_Master/compare/v1.5.2-sncn-3...v1.5.2-sncn-4
[1.5.2-sncn-3]: https://github.com/synapticon/Etherlab_EtherCAT_Master/compare/v1.5.2-sncn-2...v1.5.2-sncn-3
[1.5.2-sncn-2]: https://github.com/synapticon/Etherlab_EtherCAT_Master/compare/v1.5.2-sncn-1...v1.5.2-sncn-2
[1.5.2-sncn-1]: https://github.com/synapticon/Etherlab_EtherCAT_Master/compare/796d3f112f485ad20b5ed67a8f0ef02111227ef3...v1.5.2-sncn-1
