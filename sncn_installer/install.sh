#!/bin/bash

set -o errexit
set -o verbose

# Setup parameters
ETHERCAT_USER_GROUP="$(whoami)"
UDEV_RULES_FILE="/etc/udev/rules.d/99-EtherCAT.rules"
SCRIPT_DIR="$(cd "$( dirname "$0" )" && pwd)"
WORK_DIR="${SCRIPT_DIR}/.."
ETHERCAT_SYSCONFIG="/etc/sysconfig/ethercat"
ETHERCAT_INSTALL_PREFIX=""
ETHERCAT_START_PREFIX="/opt/etherlab"
CONFIGURE_FLAGS="--enable-sii-assign --disable-8139too --enable-hrtimer --enable-cycles"

do_configure() {
  cd ${WORK_DIR}
  ./bootstrap
  ./configure ${CONFIGURE_FLAGS} --prefix=${ETHERCAT_INSTALL_PREFIX}

  if [[ -f "${ETHERCAT_INSTALL_PREFIX}/etc/init.d/ethercat" ]]; then
    sudo ${ETHERCAT_INSTALL_PREFIX}/etc/init.d/ethercat stop
  elif [[ -f "${ETHERCAT_INSTALL_PREFIX}/etc/init.d/ethercat" ]]; then
    sudo ${ETHERCAT_INSTALL_PREFIX}/etc/init.d/ethercat stop
  fi
}

do_compile () {
  make clean
  make all modules
}

do_install () {
  sudo make modules_install install
  sudo ldconfig
  sudo depmod
}

do_setup_interfaces () {
  # Add udev rule
  sudo rm -rf ${UDEV_RULES_FILE}
  echo "KERNEL==\"EtherCAT[0-9]*\", MODE=\"0664\", GROUP=\"${ETHERCAT_USER_GROUP}\"" | sudo tee --append ${UDEV_RULES_FILE}

  # Add detected interfaces and delete old ones
  sudo sed -i '/MASTER._DEVICE/d' /etc/sysconfig/ethercat
  interfaces=$(ifconfig | grep -e "^e[tn][a-z0-9]*" -o)

  # Intel Up square 2
  # 2nd ethernet is used for this board
  if [[ -n $(lscpu | sed '13q;d' | grep "Intel(R) Pentium(R) CPU N4200 @ 1.10GHz") ]];then
    MAC=$(cat /sys/class/net/enp3s0/address)
    echo "MASTER0_DEVICE=\"${MAC}\"" | sudo tee --append ${ETHERCAT_SYSCONFIG}
  else
  # Standard laptop
    if [[ -n "${1}" ]];then
      # Use interface passed as parameter if it is passed
      iface="${1}"
      MAC=$(cat /sys/class/net/${iface}/address)
      echo "MASTER0_DEVICE=\"${MAC}\"" | sudo tee --append ${ETHERCAT_SYSCONFIG}
    else
      # Takes the first interface randomly if not
      iface=$(${interfaces} | cut -d " " -f1)
      MAC=$(cat /sys/class/net/${iface}/address)
      echo "MASTER0_DEVICE=\"${MAC}\"" | sudo tee --append ${ETHERCAT_SYSCONFIG}
    fi
  fi
  sudo sed -i 's/DEVICE_MODULES=\"\"/DEVICE_MODULES=\"generic\"/g' ${ETHERCAT_SYSCONFIG}
}

do_start() {
  if [[ -f "${ETHERCAT_INSTALL_PREFIX}/etc/init.d/ethercat" ]]; then
    sudo ${ETHERCAT_INSTALL_PREFIX}/etc/init.d/ethercat start
  elif [[ -f "${ETHERCAT_INSTALL_PREFIX}/etc/init.d/ethercat" ]]; then
    sudo ${ETHERCAT_INSTALL_PREFIX}/etc/init.d/ethercat start
  fi
}

do_configure
do_compile
do_install
do_setup_interfaces $1
do_start

echo "Done"
