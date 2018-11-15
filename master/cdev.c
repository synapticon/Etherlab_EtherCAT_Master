/******************************************************************************
 *
 *  $Id$
 *
 *  Copyright (C) 2006-2012  Florian Pose, Ingenieurgemeinschaft IgH
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
 *****************************************************************************/

/**
   \file
   EtherCAT master character device.
*/

/*****************************************************************************/

#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>

#include "cdev.h"
#include "master.h"
#include "slave_config.h"
#include "voe_handler.h"
#include "ethernet.h"
#include "ioctl.h"

/** Set to 1 to enable device operations debugging.
 */
#define DEBUG 0

/*****************************************************************************/

static int eccdev_open(struct inode *, struct file *);
static int eccdev_release(struct inode *, struct file *);
static long eccdev_ioctl(struct file *, unsigned int, unsigned long);
static int eccdev_mmap(struct file *, struct vm_area_struct *);

/** This is the kernel version from which the .fault member of the
 * vm_operations_struct is usable.
 */
#define PAGE_FAULT_VERSION KERNEL_VERSION(2, 6, 23)
#define VM_FAULT_CHANGE_VERSION KERNEL_VERSION(4, 10, 0)

#if LINUX_VERSION_CODE >= PAGE_FAULT_VERSION
#if LINUX_VERSION_CODE >= VM_FAULT_CHANGE_VERSION
static int eccdev_vma_fault(struct vm_fault *);
#else
static int eccdev_vma_fault(struct vm_area_struct *, struct vm_fault *);
#endif /* LINUX_VERSION_CODE >= VM_FAULT_CHANGE_VERSION */

#else
    static struct page *eccdev_vma_nopage(
        struct vm_area_struct *, unsigned long, int *);
#endif /* LINUX_VERSION_CODE >= PAGE_FAULT_VERSION */

/*****************************************************************************/

/** File operation callbacks for the EtherCAT character device.
 */
static struct file_operations eccdev_fops = {
    .owner          = THIS_MODULE,
    .open           = eccdev_open,
    .release        = eccdev_release,
    .unlocked_ioctl = eccdev_ioctl,
    .mmap           = eccdev_mmap
};

/** Callbacks for a virtual memory area retrieved with ecdevc_mmap().
 */
struct vm_operations_struct eccdev_vm_ops = {
#if LINUX_VERSION_CODE >= PAGE_FAULT_VERSION
    .fault = eccdev_vma_fault
#else
    .nopage = eccdev_vma_nopage
#endif /* LINUX_VERSION_CODE >= PAGE_FAULT_VERSION */
};

/*****************************************************************************/

/** Private data structure for file handles.
 */
typedef struct {
    ec_cdev_t *cdev; /**< Character device. */
    ec_ioctl_context_t ctx; /**< Context. */
} ec_cdev_priv_t;

/*****************************************************************************/

/** Constructor.
 *
 * \return 0 in case of success, else < 0
 */
int ec_cdev_init(
        ec_cdev_t *cdev, /**< EtherCAT master character device. */
        ec_master_t *master, /**< Parent master. */
        dev_t dev_num /**< Device number. */
        )
{
    int ret;

    cdev->master = master;

    cdev_init(&cdev->cdev, &eccdev_fops);
    cdev->cdev.owner = THIS_MODULE;

    ret = cdev_add(&cdev->cdev,
            MKDEV(MAJOR(dev_num), master->index), 1);
    if (ret) {
        EC_MASTER_ERR(master, "Failed to add character device!\n");
    }

    return ret;
}

/*****************************************************************************/

/** Destructor.
 */
void ec_cdev_clear(ec_cdev_t *cdev /**< EtherCAT XML device */)
{
    cdev_del(&cdev->cdev);
}

/******************************************************************************
 * File operations
 *****************************************************************************/

/** Called when the cdev is opened.
 */
int eccdev_open(struct inode *inode, struct file *filp)
{
    ec_cdev_t *cdev = container_of(inode->i_cdev, ec_cdev_t, cdev);
    ec_cdev_priv_t *priv;

    priv = kmalloc(sizeof(ec_cdev_priv_t), GFP_KERNEL);
    if (!priv) {
        EC_MASTER_ERR(cdev->master,
                "Failed to allocate memory for private data structure.\n");
        return -ENOMEM;
    }

    priv->cdev = cdev;
    priv->ctx.writable = (filp->f_mode & FMODE_WRITE) != 0;
    priv->ctx.requested = 0;
    priv->ctx.process_data = NULL;
    priv->ctx.process_data_size = 0;

    filp->private_data = priv;

#if DEBUG
    EC_MASTER_DBG(cdev->master, 0, "File opened.\n");
#endif
    return 0;
}

/*****************************************************************************/

/** Called when the cdev is closed.
 */
int eccdev_release(struct inode *inode, struct file *filp)
{
    ec_cdev_priv_t *priv = (ec_cdev_priv_t *) filp->private_data;
    ec_master_t *master = priv->cdev->master;

    if (priv->ctx.requested) {
        ecrt_release_master(master);
    }

    if (priv->ctx.process_data) {
        vfree(priv->ctx.process_data);
    }

#if DEBUG
    EC_MASTER_DBG(master, 0, "File closed.\n");
#endif

    kfree(priv);
    return 0;
}

/*****************************************************************************/

/** Called when an ioctl() command is issued.
 */
long eccdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    ec_cdev_priv_t *priv = (ec_cdev_priv_t *) filp->private_data;

#if DEBUG
    EC_MASTER_DBG(priv->cdev->master, 0,
            "ioctl(filp = 0x%p, cmd = 0x%08x (0x%02x), arg = 0x%lx)\n",
            filp, cmd, _IOC_NR(cmd), arg);
#endif

    return ec_ioctl(priv->cdev->master, &priv->ctx, cmd, (void __user *) arg);
}

/*****************************************************************************/

#ifndef VM_DONTDUMP
/** VM_RESERVED disappeared in 3.7.
 */
#define VM_DONTDUMP VM_RESERVED
#endif

/** Memory-map callback for the EtherCAT character device.
 *
 * The actual mapping will be done in the eccdev_vma_nopage() callback of the
 * virtual memory area.
 *
 * \return Always zero (success).
 */
int eccdev_mmap(
        struct file *filp,
        struct vm_area_struct *vma
        )
{
    ec_cdev_priv_t *priv = (ec_cdev_priv_t *) filp->private_data;

    EC_MASTER_DBG(priv->cdev->master, 1, "mmap()\n");

    vma->vm_ops = &eccdev_vm_ops;
    vma->vm_flags |= VM_DONTDUMP; /* Pages will not be swapped out */
    vma->vm_private_data = priv;

    return 0;
}

/*****************************************************************************/

#if LINUX_VERSION_CODE >= PAGE_FAULT_VERSION

/** Page fault callback for a virtual memory area.
 *
 * Called at the first access on a virtual-memory area retrieved with
 * ecdev_mmap().
 *
 * \return Zero on success, otherwise a negative error code.
 */
static int eccdev_vma_fault(
#if (LINUX_VERSION_CODE < VM_FAULT_CHANGE_VERSION)
        struct vm_area_struct *vma, /**< Virtual memory area. */
#endif /* (LINUX_VERSION_CODE < VM_FAULT_CHANGE_VERSION) */
        struct vm_fault *vmf /**< Fault data. */
        )
{
#if (LINUX_VERSION_CODE < VM_FAULT_CHANGE_VERSION)
    ec_cdev_priv_t *priv = (ec_cdev_priv_t *) vma->vm_private_data;
#else
    ec_cdev_priv_t *priv = (ec_cdev_priv_t *) vmf->vma->vm_private_data;
#endif /* (LINUX_VERSION_CODE < VM_FAULT_CHANGE_VERSION) */
    unsigned long offset = vmf->pgoff << PAGE_SHIFT;
    struct page *page;

    if (offset >= priv->ctx.process_data_size) {
        return VM_FAULT_SIGBUS;
    }

    page = vmalloc_to_page(priv->ctx.process_data + offset);
    if (!page) {
        return VM_FAULT_SIGBUS;
    }

    get_page(page);
    vmf->page = page;

    EC_MASTER_DBG(priv->cdev->master, 1, "Vma fault, virtual_address = %p,"
#if (LINUX_VERSION_CODE >= VM_FAULT_CHANGE_VERSION)
            " offset = %lu, page = %p\n", (void*)vmf->address, offset, page);
#else
            " offset = %lu, page = %p\n", vmf->virtual_address, offset, page);
#endif /* (LINUX_VERSION_CODE >= VM_FAULT_CHANGE_VERSION) */

    return 0;
}

#else

/** Nopage callback for a virtual memory area.
 *
 * Called at the first access on a virtual-memory area retrieved with
 * ecdev_mmap().
 */
struct page *eccdev_vma_nopage(
        struct vm_area_struct *vma, /**< Virtual memory area initialized by
                                      the kernel. */
        unsigned long address, /**< Requested virtual address. */
        int *type /**< Type output parameter. */
        )
{
    unsigned long offset;
    struct page *page = NOPAGE_SIGBUS;
    ec_cdev_priv_t *priv = (ec_cdev_priv_t *) vma->vm_private_data;
    ec_master_t *master = priv->cdev->master;

    offset = (address - vma->vm_start) + (vma->vm_pgoff << PAGE_SHIFT);

    if (offset >= priv->ctx.process_data_size)
        return NOPAGE_SIGBUS;

    page = vmalloc_to_page(priv->ctx.process_data + offset);

    EC_MASTER_DBG(master, 1, "Nopage fault vma, address = %#lx,"
            " offset = %#lx, page = %p\n", address, offset, page);

    get_page(page);
    if (type)
        *type = VM_FAULT_MINOR;

    return page;
}

#endif

/*****************************************************************************/
