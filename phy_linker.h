/**
 * Copyright (c) 2016 Wei-Lun Hsu. All Rights Reserved.
 */
/** @file phy_linker.h
 *
 * @author Wei-Lun Hsu
 * @version 0.1
 * @date 2016/03/01
 * @license
 * @description
 */

#ifndef __phy_linker_H_wM3BrPoP_ldSy_Hjx8_sV36_u0nCXgWLl6NB__
#define __phy_linker_H_wM3BrPoP_ldSy_Hjx8_sV36_u0nCXgWLl6NB__

#ifdef __cplusplus
extern "C" {
#endif

#include "phy_linker_desc.h"
//=============================================================================
//                Constant Definition
//=============================================================================
/**
 *  type of physical linker
 */
typedef enum pl_type
{
    PL_TYPE_UNKNOW,
    PL_TYPE_UART,
    PL_TYPE_UDP,
    PL_TYPE_TCP,

} pl_type_t;
//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================
/**
 *  init info
 */
struct pl_init_info;
typedef struct pl_init_info
{
    pl_type_t       pl_type;

    void    (*reset_descriptor)(pl_desc_t **ppUser_desc, struct pl_init_info *pInit_info);
    void    *pTunnel_infop[1];

} pl_init_info_t;

/**
 *  trans info
 */
typedef struct pl_trans_info
{
    int         target_id;
    void        *pData;
} pl_trans_info_t;

/**
 *  recv info
 */
typedef struct pl_recv_info
{
    int         target_id;
    void        *pData;
} pl_recv_info_t;

/**
 *  ctrl info
 */
typedef struct pl_ctrl_info
{
    int         target_id;

} pl_ctrl_info_t;

/**
 *  handle of physical linker
 */
typedef struct pl_handle
{
    pl_type_t       pl_type;
} pl_handle_t;
//=============================================================================
//                Global Data Definition
//=============================================================================

//=============================================================================
//                Private Function Definition
//=============================================================================

//=============================================================================
//                Public Function Definition
//=============================================================================
int
pl_create_handle(
	pl_handle_t	    **ppHPhy_linker);


int
pl_destroy_handle(
	pl_handle_t	    **ppHPhy_linker);


int
pl_init(
	pl_handle_t	    *pHPhy_linker,
    pl_init_info_t  *pInit_info);


int
pl_deinit(
	pl_handle_t	    *pHPhy_linker);


int
pl_trans(
	pl_handle_t	    *pHPhy_linker,
    pl_trans_info_t *pTrans_info);


int
pl_recv(
	pl_handle_t	    *pHPhy_linker,
    pl_recv_info_t  *pRecv_info);


#ifdef __cplusplus
}
#endif

#endif
