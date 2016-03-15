/**
 * Copyright (c) 2016 Wei-Lun Hsu. All Rights Reserved.
 */
/** @file phy_linker_desc.h
 *
 * @author Wei-Lun Hsu
 * @version 0.1
 * @date 2016/03/01
 * @license
 * @description
 */

#ifndef __phy_linker_desc_H_wREzMNlI_lvQl_HnRi_sPl3_u96NalSTijGg__
#define __phy_linker_desc_H_wREzMNlI_lvQl_HnRi_sPl3_u96NalSTijGg__

#ifdef __cplusplus
extern "C" {
#endif


//=============================================================================
//                Constant Definition
//=============================================================================

//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================
/**
 *  interrnal manager of physical linker 
 */
typedef struct pl_mgr
{
    void        *pPriv_info;
} pl_mgr_t;

/**
 *  arguments to set physical linker
 */
typedef struct pl_args
{
    int     reserved;
    
    union {
        struct {
            void    *pt;
        } def;
    } u;
} pl_args_t;
 
/**
 *  descriptor of physical linker
 */
struct pl_desc;
typedef struct pl_desc
{
    char                    *name;
    struct pl_desc          *next;
    unsigned int            id;
    
    int     (*init)(pl_mgr_t *pMgr, pl_args_t *pArgs);
    int     (*deinit)(pl_mgr_t *pMgr);

    int     (*trans)(pl_mgr_t *pMgr, pl_args_t *pArgs);
    int     (*recv)(pl_mgr_t *pMgr, pl_args_t *pArgs);

    int     (*ctrl)(pl_mgr_t *pMgr, pl_args_t *pArgs);
} pl_desc_t;
//=============================================================================
//                Global Data Definition
//=============================================================================

//=============================================================================
//                Private Function Definition
//=============================================================================

//=============================================================================
//                Public Function Definition
//=============================================================================

#ifdef __cplusplus
}
#endif

#endif
