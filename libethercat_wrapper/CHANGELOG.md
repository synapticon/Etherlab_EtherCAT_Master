# Changelog
All notable changes to this project made by SYNAPTICON will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/).

##[Unreleased]
### Added
- Enable reading/writing of string SDO values
- Enable handling 64-bit integer values and setting the time of day
- Check if the SDO request pointers are valid on upload/download
- Add a function for bus rescanning
- Add a function for preemptive bus rescanning
- Enable preemptive slave vendor ID and product code fetching
- Add a Synapticon changelog

### Fixed
- Always null-terminate the string SDO values
- Fix the domain registration during master initialization
- Free the master domain in the proper place, when stopping the master
- Fix segmentation fault on ecw_master_stop

## [1.5.2-sncn-6] - 2018-10-23
### Added
- Use syslog for logging
- Add the object name to the SDO structure
- Enable fetching some slave information before EtherCAT master init
- Allow an update of the slave states (ecw_master_scan)
- Define code errors

### Changed
- Change order of PDOs in domain reg - input, then output
- General refactor
- Improve slave clean up
- Define the SDO entry type as an enum
- Process SDOs according to their entry type on direct upload

### Removed
- Remove the master info pointer from the master struct
- Remove the unused and redundant alias variable from the slave struct

### Fixed
- Always refresh the number of responding slaves
- Fix the slave ID check
- Fix SDO read/write while in OP state
- Fix getting/setting of the slave state - add BOOT state
- Fix: Return NULL if the SDO is not found
- Various test fixes
- Fix boolean SDO read/write
- Fix the usage of slave aliases

## [1.5.2-sncn-5] - 2017-04-20
### Added
- Add setting of slave operational state
- Add usage of AL state enum for state request

### Changed
- Improve AL state handling

### Removed
- Remove initial upload of entry values

### Fixed
- Fix domain registration issues
- Fix byte alignement handling for PDOs
- Fix master start and stop
- Fix setup of SDO requests
- Fix type warnings for PDO mappings

## [1.5.2-sncn-4] - 2017-02-14
### Added
- Add and integrate into build process

## [1.5.2-sncn-3] - 2016-12-16
### Added
- Add SDO Info access to libethercat

[Unreleased]: https://github.com/synapticon/Etherlab_EtherCAT_Master/compare/v1.5.2-sncn-6...HEAD
[1.5.2-sncn-6]: https://github.com/synapticon/Etherlab_EtherCAT_Master/compare/v1.5.2-sncn-5...v1.5.2-sncn-6
[1.5.2-sncn-5]: https://github.com/synapticon/Etherlab_EtherCAT_Master/compare/v1.5.2-sncn-4...v1.5.2-sncn-5
[1.5.2-sncn-4]: https://github.com/synapticon/Etherlab_EtherCAT_Master/compare/v1.5.2-sncn-3...v1.5.2-sncn-4
[1.5.2-sncn-3]: https://github.com/synapticon/Etherlab_EtherCAT_Master/compare/v1.5.2-sncn-2...v1.5.2-sncn-3
