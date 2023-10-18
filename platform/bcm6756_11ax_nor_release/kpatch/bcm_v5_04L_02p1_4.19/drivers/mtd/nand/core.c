// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017 Free Electrons
 *
 * Authors:
 *    Boris Brezillon <boris.brezillon@free-electrons.com>
 *    Peter Pan <peterpandong@micron.com>
 */

#define pr_fmt(fmt)    "nand: " fmt

#include <linux/module.h>
#include <linux/mtd/nand.h>
#if defined(CONFIG_TENDA_PRIVATE_BSP) && defined(CONFIG_MTD_NAND)
#include <linux/mtd/flashchip.h>

/**
 * nand_release_device - [GENERIC] release chip
 * @mtd: MTD device structure
 *
 * Release chip lock and wake up anyone waiting on the device.
 */
static void nanddev_release_device(struct mtd_info *mtd)
{
    struct nand_device *chip = mtd_to_nanddev(mtd);

    /* Release the controller and the chip */
    spin_lock(&chip->bbt.controller->lock);
    chip->bbt.controller->active = NULL;
    chip->bbt.state = FL_READY;
    wake_up(&chip->bbt.controller->wq);
    spin_unlock(&chip->bbt.controller->lock);
}

/**
 * nand_block_checkbad - [GENERIC] Check if a block is marked bad
 * @mtd: MTD device structure
 * @ofs: offset from device start
 * @allowbbt: 1, if its allowed to access the bbt area
 *
 * Check, if the block is bad. Either by reading the bad block table or
 * calling of the scan function.
 */
static int nanddev_block_checkbad(struct mtd_info *mtd, loff_t ofs, int allowbbt)
{
    struct nand_device *chip = mtd_to_nanddev(mtd);

    if (!chip->bbt.bbt)
        return chip->bbt.block_bad(mtd, ofs);

    /* Return info from the table */
    return nanddev_isbad_bbt(mtd, ofs, allowbbt);
}

/**
 * nand_get_device - [GENERIC] Get chip for selected access
 * @mtd: MTD device structure
 * @new_state: the state which is requested
 *
 * Get the device and lock it for exclusive access
 */
static int
nanddev_get_device(struct mtd_info *mtd, int new_state)
{
    struct nand_device *nand = mtd_to_nanddev(mtd);
    spinlock_t *lock = &nand->bbt.controller->lock;
    wait_queue_head_t *wq = &nand->bbt.controller->wq;
    DECLARE_WAITQUEUE(wait, current);
retry:
    spin_lock(lock);

    /* Hardware controller shared among independent devices */
    if (!nand->bbt.controller->active)
        nand->bbt.controller->active = nand;

    if (nand->bbt.controller->active == nand && nand->bbt.state == FL_READY) {
        nand->bbt.state = new_state;
        spin_unlock(lock);
        return 0;
    }

    if (new_state == FL_PM_SUSPENDED) {
        if (nand->bbt.controller->active->bbt.state == FL_PM_SUSPENDED) {
            nand->bbt.state = FL_PM_SUSPENDED;
            spin_unlock(lock);
            return 0;
        }
    }

    set_current_state(TASK_UNINTERRUPTIBLE);
    add_wait_queue(wq, &wait);
    spin_unlock(lock);
    schedule();
    remove_wait_queue(wq, &wait);
    goto retry;
}
#endif

/**
 * nanddev_isbad() - Check if a block is bad
 * @nand: NAND device
 * @pos: position pointing to the block we want to check
 *
 * Return: true if the block is bad, false otherwise.
 */
bool nanddev_isbad(struct nand_device *nand, const struct nand_pos *pos)
{
#if !(defined(CONFIG_TENDA_PRIVATE_BSP) && defined(CONFIG_MTD_NAND))
    if (nanddev_bbt_is_initialized(nand)) {
        unsigned int entry;
        int status;

        entry = nanddev_bbt_pos_to_entry(nand, pos);
        status = nanddev_bbt_get_block_status(nand, entry);
        /* Lazy block status retrieval */
        if (status == NAND_BBT_BLOCK_STATUS_UNKNOWN) {
            if (nand->ops->isbad(nand, pos))
                status = NAND_BBT_BLOCK_FACTORY_BAD;
            else
                status = NAND_BBT_BLOCK_GOOD;

            nanddev_bbt_set_block_status(nand, entry, status);
        }

        if (status == NAND_BBT_BLOCK_WORN ||
            status == NAND_BBT_BLOCK_FACTORY_BAD)
            return true;

        return false;
    }
#endif
    return nand->ops->isbad(nand, pos);
}
EXPORT_SYMBOL_GPL(nanddev_isbad);

/**
 * nanddev_markbad() - Mark a block as bad
 * @nand: NAND device
 * @pos: position of the block to mark bad
 *
 * Mark a block bad. This function is updating the BBT if available and
 * calls the low-level markbad hook (nand->ops->markbad()).
 *
 * Return: 0 in case of success, a negative error code otherwise.
 */
int nanddev_markbad(struct nand_device *nand, const struct nand_pos *pos)
{
    struct mtd_info *mtd = nanddev_to_mtd(nand);
    unsigned int entry;
    int ret = 0;

    if (nanddev_isbad(nand, pos))
        return 0;

    ret = nand->ops->markbad(nand, pos);
    if (ret)
        pr_warn("failed to write BBM to block @%llx (err = %d)\n",
            nanddev_pos_to_offs(nand, pos), ret);

    if (!nanddev_bbt_is_initialized(nand))
        goto out;

    entry = nanddev_bbt_pos_to_entry(nand, pos);
    ret = nanddev_bbt_set_block_status(nand, entry, NAND_BBT_BLOCK_WORN);
    if (ret)
        goto out;

    ret = nanddev_bbt_update(nand);

out:
    if (!ret)
        mtd->ecc_stats.badblocks++;

    return ret;
}
EXPORT_SYMBOL_GPL(nanddev_markbad);

/**
 * nanddev_isreserved() - Check whether an eraseblock is reserved or not
 * @nand: NAND device
 * @pos: NAND position to test
 *
 * Checks whether the eraseblock pointed by @pos is reserved or not.
 *
 * Return: true if the eraseblock is reserved, false otherwise.
 */
bool nanddev_isreserved(struct nand_device *nand, const struct nand_pos *pos)
{
#if defined(CONFIG_TENDA_PRIVATE_BSP) && defined(CONFIG_MTD_NAND)
    if (!nand->bbt.bbt) {
        return false;
    }
    return nanddev_isreserved_bbt(nand, pos->target);
#else
    unsigned int entry;
    int status;

    if (!nanddev_bbt_is_initialized(nand))
        return false;

    /* Return info from the table */
    entry = nanddev_bbt_pos_to_entry(nand, pos);
    status = nanddev_bbt_get_block_status(nand, entry);
    return status == NAND_BBT_BLOCK_RESERVED;
#endif
}
EXPORT_SYMBOL_GPL(nanddev_isreserved);

/**
 * nanddev_erase() - Erase a NAND portion
 * @nand: NAND device
 * @pos: position of the block to erase
 *
 * Erases the block if it's not bad.
 *
 * Return: 0 in case of success, a negative error code otherwise.
 */
int nanddev_erase(struct nand_device *nand, const struct nand_pos *pos)
{
    if (nanddev_isbad(nand, pos) || nanddev_isreserved(nand, pos)) {
        pr_warn("attempt to erase a bad/reserved block @%llx\n",
            nanddev_pos_to_offs(nand, pos));
#if defined(CONFIG_TENDA_PRIVATE_BSP) && defined(CONFIG_MTD_NAND)
        return EIO;
#else
        return -EIO;
#endif
    }

    return nand->ops->erase(nand, pos);
}
EXPORT_SYMBOL_GPL(nanddev_erase);

/**
 * nanddev_mtd_erase() - Generic mtd->_erase() implementation for NAND devices
 * @mtd: MTD device
 * @einfo: erase request
 *
 * This is a simple mtd->_erase() implementation iterating over all blocks
 * concerned by @einfo and calling nand->ops->erase() on each of them.
 *
 * Note that mtd->_erase should not be directly assigned to this helper,
 * because there's no locking here. NAND specialized layers should instead
 * implement there own wrapper around nanddev_mtd_erase() taking the
 * appropriate lock before calling nanddev_mtd_erase().
 *
 * Return: 0 in case of success, a negative error code otherwise.
 */
int nanddev_mtd_erase(struct mtd_info *mtd, struct erase_info *einfo)
{
    struct nand_device *nand = mtd_to_nanddev(mtd);
    struct nand_pos pos, last;
    int ret;

    nanddev_offs_to_pos(nand, einfo->addr, &pos);
    nanddev_offs_to_pos(nand, einfo->addr + einfo->len - 1, &last);
    while (nanddev_pos_cmp(&pos, &last) <= 0) {
        ret = nanddev_erase(nand, &pos);
#if defined(CONFIG_TENDA_PRIVATE_BSP) && defined(CONFIG_MTD_NAND)
        if (EIO == ret) {
            nanddev_pos_next_eraseblock(nand, &pos);
            continue;
        } else if (ret) {
#else
        if (ret) {
#endif
            einfo->fail_addr = nanddev_pos_to_offs(nand, &pos);

            return ret;
        }

        nanddev_pos_next_eraseblock(nand, &pos);
    }

    return 0;
}
EXPORT_SYMBOL_GPL(nanddev_mtd_erase);

/**
 * nanddev_init() - Initialize a NAND device
 * @nand: NAND device
 * @ops: NAND device operations
 * @owner: NAND device owner
 *
 * Initializes a NAND device object. Consistency checks are done on @ops and
 * @nand->memorg. Also takes care of initializing the BBT.
 *
 * Return: 0 in case of success, a negative error code otherwise.
 */
int nanddev_init(struct nand_device *nand, const struct nand_ops *ops,
         struct module *owner)
{
    struct mtd_info *mtd = nanddev_to_mtd(nand);
    struct nand_memory_organization *memorg = nanddev_get_memorg(nand);

    if (!nand || !ops)
        return -EINVAL;

    if (!ops->erase || !ops->markbad || !ops->isbad)
        return -EINVAL;

    if (!memorg->bits_per_cell || !memorg->pagesize ||
        !memorg->pages_per_eraseblock || !memorg->eraseblocks_per_lun ||
        !memorg->planes_per_lun || !memorg->luns_per_target ||
        !memorg->ntargets)
        return -EINVAL;

    nand->rowconv.eraseblock_addr_shift =
                    fls(memorg->pages_per_eraseblock - 1);
    nand->rowconv.lun_addr_shift = fls(memorg->eraseblocks_per_lun - 1) +
                       nand->rowconv.eraseblock_addr_shift;

    nand->ops = ops;

    mtd->type = memorg->bits_per_cell == 1 ?
            MTD_NANDFLASH : MTD_MLCNANDFLASH;
    mtd->flags = MTD_CAP_NANDFLASH;
    mtd->erasesize = memorg->pagesize * memorg->pages_per_eraseblock;
    mtd->writesize = memorg->pagesize;
    mtd->writebufsize = memorg->pagesize;
    mtd->oobsize = memorg->oobsize;
    mtd->size = nanddev_size(nand);
    mtd->owner = owner;

    return nanddev_bbt_init(nand);
}
EXPORT_SYMBOL_GPL(nanddev_init);

/**
 * nanddev_cleanup() - Release resources allocated in nanddev_init()
 * @nand: NAND device
 *
 * Basically undoes what has been done in nanddev_init().
 */
void nanddev_cleanup(struct nand_device *nand)
{
    if (nanddev_bbt_is_initialized(nand))
        nanddev_bbt_cleanup(nand);
}
EXPORT_SYMBOL_GPL(nanddev_cleanup);

MODULE_DESCRIPTION("Generic NAND framework");
MODULE_AUTHOR("Boris Brezillon <boris.brezillon@free-electrons.com>");
MODULE_LICENSE("GPL v2");
