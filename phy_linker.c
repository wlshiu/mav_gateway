/**
 * Copyright (c) 2016 Wei-Lun Hsu. All Rights Reserved.
 */
/** @file phy_linker.c
 *
 * @author Wei-Lun Hsu
 * @version 0.1
 * @date 2016/03/01
 * @license
 * @description
 */


#include <stdlib.h>
#include "mleak_check.h"

#include "phy_linker.h"

#include "platform_def.h"
#include "err_code_def.h"
//=============================================================================
//                Constant Definition
//=============================================================================

//=============================================================================
//                Macro Definition
//=============================================================================

//=============================================================================
//                Structure Definition
//=============================================================================
typedef struct pl_dev
{
    pl_handle_t         pl_handle;

    pthread_mutex_t     pl_mutex;

    pl_type_t           pl_type;
    pl_desc_t           *pCur_desc;
    pl_mgr_t            pl_mgr;

} pl_dev_t;
//=============================================================================
//                Global Data Definition
//=============================================================================

//=============================================================================
//                Private Function Definition
//=============================================================================
DEFINE_REGISTER_TEMPLATE(pl_desc_t, pl_type_t);


static void
_register_all()
{
    static int bInitialized = 0;

    if( bInitialized )
        return;

    REGISTER_ITEM(pl_desc_t, tcp);
    REGISTER_ITEM(pl_desc_t, udp);

    bInitialized = 1;
    return;
}


//=============================================================================
//                Public Function Definition
//=============================================================================
int
pl_create_handle(
	pl_handle_t	    **ppHPhy_linker)
{
	int		result = ERRCODE_OK;

	do{
        pl_dev_t    *pDev = 0;

		if( *ppHPhy_linker != 0 )
		{
			err_msg("err, Exist a handle !!");
            result = ERRCODE_INVALID_PARAM;
			break;
		}

        _register_all();

        if( !(pDev = malloc(sizeof(pl_dev_t))) )
        {
            result = ERRCODE_MALLOC_FAIL;
            break;
        }
        memset(pDev, 0x0, sizeof(pl_dev_t));

        _mutex_init(pDev->pl_mutex);

		//----------------------
		(*ppHPhy_linker) = &pDev->pl_handle;

	}while(0);

	if( result )
	{
		err_msg("err 0x%x !\n", result);
	}

	return result;
}

int
pl_destroy_handle(
	pl_handle_t	    **ppHPhy_linker)
{
	int		result = ERRCODE_OK;

    if( ppHPhy_linker && *ppHPhy_linker )
    {
        pl_dev_t            *pDev = STRUCTURE_POINTER(pl_dev_t, (*ppHPhy_linker), pl_handle);
        pthread_mutex_t     mutex;

        mutex = pDev->pl_mutex;

        free(pDev);

        _mutex_deinit(mutex);

        *ppHPhy_linker = 0;
    }

	return result;
}

int
pl_init(
	pl_handle_t	    *pHPhy_linker,
    pl_init_info_t  *pInit_info)
{
    int		    result = ERRCODE_OK;
    pl_dev_t    *pDev = 0;

    _verify_handle(pHPhy_linker, ERRCODE_NULL_POINTER);
    _verify_handle(pInit_info, ERRCODE_NULL_POINTER);

    pDev = STRUCTURE_POINTER(pl_dev_t, pHPhy_linker, pl_handle);

    _mutex_lock(pDev->pl_mutex);

    do{
        pDev->pCur_desc = FIND_DESC_ITEM(pl_desc_t, pInit_info->pl_type);
        if( !pDev->pCur_desc )
        {
            err_msg("Can't find target linker descriptor !\n");
        }

        pDev->pl_type = pInit_info->pl_type;
        if( pInit_info->reset_descriptor )
        {
            pl_desc_t   *pTmp_desc = 0;

            pInit_info->reset_descriptor(&pTmp_desc, pInit_info);
            pDev->pCur_desc = pTmp_desc;
        }

        if( pDev->pCur_desc &&
            pDev->pCur_desc->init )
        {
            pl_args_t       pl_args = {0};

            result = pDev->pCur_desc->init(&pDev->pl_mgr, &pl_args);
            if( result )    break;
        }

    }while(0);

    _mutex_unlock(pDev->pl_mutex);

	if( result )
	{
		err_msg("err 0x%x !\n", result);
	}
    return result;
}

int
pl_deinit(
	pl_handle_t	    *pHPhy_linker)
{
    int		    result = ERRCODE_OK;
    pl_dev_t    *pDev = 0;

    _verify_handle(pHPhy_linker, ERRCODE_NULL_POINTER);

    pDev = STRUCTURE_POINTER(pl_dev_t, pHPhy_linker, pl_handle);

    _mutex_lock(pDev->pl_mutex);

    do{
        if( pDev->pCur_desc &&
            pDev->pCur_desc->deinit )
        {
            result = pDev->pCur_desc->deinit(&pDev->pl_mgr);
            if( result )    break;
        }

    }while(0);

    _mutex_unlock(pDev->pl_mutex);

	if( result )
	{
		err_msg("err 0x%x !\n", result);
	}
    return result;
}

int
pl_trans(
	pl_handle_t	    *pHPhy_linker,
    pl_trans_info_t *pTrans_info)
{
    int		    result = ERRCODE_OK;
    pl_dev_t    *pDev = 0;

    _verify_handle(pHPhy_linker, ERRCODE_NULL_POINTER);
    _verify_handle(pTrans_info, ERRCODE_NULL_POINTER);

    pDev = STRUCTURE_POINTER(pl_dev_t, pHPhy_linker, pl_handle);

    _mutex_lock(pDev->pl_mutex);

    do{
        if( pDev->pCur_desc &&
            pDev->pCur_desc->trans )
        {
            pl_args_t       pl_args = {0};

            result = pDev->pCur_desc->trans(&pDev->pl_mgr, &pl_args);
            if( result )    break;
        }

    }while(0);

    _mutex_unlock(pDev->pl_mutex);

	if( result )
	{
		err_msg("err 0x%x !\n", result);
	}
    return result;
}

int
pl_recv(
	pl_handle_t	    *pHPhy_linker,
    pl_recv_info_t  *pRecv_info)
{
    int		    result = ERRCODE_OK;
    pl_dev_t    *pDev = 0;

    _verify_handle(pHPhy_linker, ERRCODE_NULL_POINTER);
    _verify_handle(pRecv_info, ERRCODE_NULL_POINTER);

    pDev = STRUCTURE_POINTER(pl_dev_t, pHPhy_linker, pl_handle);

    _mutex_lock(pDev->pl_mutex);

    do{
        if( pDev->pCur_desc &&
            pDev->pCur_desc->recv )
        {
            pl_args_t       pl_args = {0};
            result = pDev->pCur_desc->recv(&pDev->pl_mgr, &pl_args);
            if( result )    break;

            // ToDo: assign received data to user
            // pRecv_info->pData = ;

        }

    }while(0);

    _mutex_unlock(pDev->pl_mutex);

	if( result )
	{
		err_msg("err 0x%x !\n", result);
	}
    return result;
}

/*
int
pl_xxx(
	pl_handle_t	    *pHPhy_linker,
    xxx_t  *pInit_info)
{
    int		result = ERRCODE_OK;

    _verify_handle(pHPhy_linker, ERRCODE_NULL_POINTER);
    _verify_handle(pInit_info, ERRCODE_NULL_POINTER);

    do{
        pl_dev_t    *pDev = STRUCTURE_POINTER(pl_dev_t, pHPhy_linker, pl_handle);

        _mutex_lock(pDev->pl_mutex);

        _mutex_unlock(pDev->pl_mutex);
    }while(0);


	if( result )
	{
		err_msg("err 0x%x !\n", result);
	}
    return result;
}

*/
