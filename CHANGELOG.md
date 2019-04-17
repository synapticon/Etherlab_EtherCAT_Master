# Changelog
All notable changes to this project made by SYNAPTICON will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/).

##[Unreleased]
#### Added
- Add a function to trigger bus rescanning

#### Fixed
- Suport Linux kernel 4.15.7
- Fix: sncn_installer uses right sysconfig

## [1.5.2-sncn-6] - 2018-10-23
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
#### Changed
- Improve AL state handling

## [1.5.2-sncn-4] - 2017-02-14
#### Added
- Add object code to SDO information object

#### Fixed
- Fix e1000e update script

## [1.5.2-sncn-3] - 2016-12-16
#### Fixed
- Fix interface name parsing

## [1.5.2-sncn-2] - 2016-11-15
#### Changed
- Add BOOT state to exception (only FoE allowed)

#### Fixed
- Fix interface detection in installer script
- Fixed fragmented SoE write request
- Prevent CCAT auto-loading

## [1.5.2-sncn-1] - 2016-05-04
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
