/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   rfsvp_server.c
 * \author sandeepprakash
 *
 * \date   24-Sep-2012
 *
 * \brief  
 *
 ******************************************************************************/

/********************************** INCLUDES **********************************/
#include "rfsvp_server_env.h"

/********************************* CONSTANTS **********************************/

/*********************************** MACROS ***********************************/

/******************************** ENUMERATIONS ********************************/

/************************* STRUCTURE/UNION DATA TYPES *************************/

/************************ STATIC FUNCTION PROTOTYPES **************************/

/****************************** LOCAL FUNCTIONS *******************************/
RFSVP_RET_E rfsvp_server_init (
   RFSVP_SERVER_HDL *phl_rfsvp_server_hdl,
   RFSVP_SERVER_INIT_PARAMS_X *px_init_params)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt = NULL;
   TASK_RET_E e_task_ret = eTASK_RET_FAILURE;
   TASK_CREATE_PARAM_X x_task_param = {{0}};

   if ((NULL == phl_rfsvp_server_hdl) || (NULL == px_init_params))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   if ((NULL == px_init_params->hl_sockmon_hdl)
      || (0 == px_init_params->us_listen_port_start_ho)
      || (0 == px_init_params->ui_no_nodes)
      || (px_init_params->ui_no_nodes > RFSVP_MAX_SERVER_NODES)
      || (px_init_params->ui_node_index > (RFSVP_MAX_SERVER_NODES - 1)))
   {
      RFSVP_S_LOG_LOW("Invalid Args. Sockmon Handle: %p, Listen Port: %d, "
         "No of Nodes: %d, Index: %d", px_init_params->hl_sockmon_hdl,
         px_init_params->us_listen_port_start_ho, px_init_params->ui_no_nodes,
         px_init_params->ui_node_index);
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   *phl_rfsvp_server_hdl = NULL;

   px_rfsvp_server_ctxt = pal_malloc(sizeof(RFSVP_SERVER_CTXT_X), NULL);
   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("pal_malloc failed: %p", px_rfsvp_server_ctxt);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   (void) pal_memmove(&(px_rfsvp_server_ctxt->x_init_params), px_init_params,
      sizeof(px_rfsvp_server_ctxt->x_init_params));

   x_task_param.b_msgq_needed = true;
   x_task_param.ui_msgq_size = RFSVP_SERVER_TASK_MSGQ_SIZE;
   x_task_param.fn_task_routine = rfsvp_server_node_main_task;
   x_task_param.p_app_data = px_rfsvp_server_ctxt;
   e_task_ret = task_create(&(px_rfsvp_server_ctxt->hl_main_task_hdl),
      &x_task_param);
   if ((eTASK_RET_SUCCESS != e_task_ret)
      || (NULL == px_rfsvp_server_ctxt->hl_main_task_hdl))
   {
      RFSVP_S_LOG_LOW("task_create failed: %d, %p", e_task_ret,
         px_rfsvp_server_ctxt, px_rfsvp_server_ctxt->hl_main_task_hdl);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_task_ret = task_start (px_rfsvp_server_ctxt->hl_main_task_hdl);
      if (eTASK_RET_SUCCESS != e_task_ret)
      {
         RFSVP_S_LOG_LOW("task_start failed: %d", e_task_ret);
         e_task_ret = task_delete (px_rfsvp_server_ctxt->hl_main_task_hdl);
         if (eTASK_RET_SUCCESS != e_task_ret)
         {
            RFSVP_S_LOG_LOW("task_delete failed: %d", e_task_ret);
         }
         px_rfsvp_server_ctxt->hl_main_task_hdl = NULL;
         e_error = eRFSVP_RET_RESOURCE_FAILURE;
      }
      else
      {
         RFSVP_S_LOG_MED("task_create success for hl_listner_task_hdl");
         RFSVP_S_LOG_MED("RFSVP Server Init Success");
         *phl_rfsvp_server_hdl = (RFSVP_SERVER_HDL) px_rfsvp_server_ctxt;
         e_error = eRFSVP_RET_SUCCESS;
      }
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_deinit (
   RFSVP_SERVER_HDL hl_rfsvp_server_hdl)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt = NULL;
   TASK_RET_E e_task_ret = eTASK_RET_FAILURE;

   if (NULL == hl_rfsvp_server_hdl)
   {
      RFSVP_S_LOG_LOW("Invalid Handle");
      e_error = eRFSVP_RET_INVALID_HANDLE;
      goto CLEAN_RETURN;
   }

   px_rfsvp_server_ctxt = (RFSVP_SERVER_CTXT_X *) hl_rfsvp_server_hdl;

   if (NULL != px_rfsvp_server_ctxt->hl_main_task_hdl)
   {
      e_task_ret = task_delete (px_rfsvp_server_ctxt->hl_main_task_hdl);
      if (eTASK_RET_SUCCESS != e_task_ret)
      {
         RFSVP_S_LOG_LOW("task_delete failed: %d", e_task_ret);
      }
      px_rfsvp_server_ctxt->hl_main_task_hdl = NULL;
   }

   pal_free(px_rfsvp_server_ctxt);
   px_rfsvp_server_ctxt = NULL;
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}
