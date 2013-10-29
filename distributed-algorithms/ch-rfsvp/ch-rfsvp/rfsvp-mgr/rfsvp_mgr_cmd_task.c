/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   rfsvp_mgr_cmd_task.c
 * \author sandeepprakash
 *
 * \date   08-Dec-2012
 *
 * \brief  
 *
 ******************************************************************************/

/********************************** INCLUDES **********************************/
#include "rfsvp_mgr_env.h"

/********************************* CONSTANTS **********************************/

/*********************************** MACROS ***********************************/

/******************************** ENUMERATIONS ********************************/

/************************* STRUCTURE/UNION DATA TYPES *************************/

/************************ STATIC FUNCTION PROTOTYPES **************************/
static RFSVP_RET_E rfsvp_mgr_cmd_task_init (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt);

static RFSVP_RET_E rfsvp_mgr_cmd_task_deinit (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt);

/****************************** LOCAL FUNCTIONS *******************************/
void *rfsvp_mgr_cmd_task(
   void *p_thread_args)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   TASK_RET_E e_task_ret = eTASK_RET_FAILURE;
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt = NULL;
   uint8_t uca_cmd_buf[RFSVP_MGR_CMD_LINE_BUF_LEN] = {0};
   uint32_t ui_cmd_buf_len = 0;

   if (NULL == p_thread_args)
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      goto CLEAN_RETURN;
   }

   px_rfsvp_mgr_ctxt = (RFSVP_MGR_CTXT_X *) p_thread_args;

   e_error = rfsvp_mgr_cmd_task_init (px_rfsvp_mgr_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_cmd_task_init failed: %d", e_error);
      goto CLEAN_RETURN;
   }
   RFSVP_M_LOG_MED("rfsvp_mgr_cmd_task_init success");

   while (task_is_in_loop (px_rfsvp_mgr_ctxt->hl_listner_task_hdl))
   {
      ui_cmd_buf_len = sizeof(uca_cmd_buf);
      e_error = rfsvp_mgr_cmd_parser_get_next_command (px_rfsvp_mgr_ctxt,
         uca_cmd_buf, &ui_cmd_buf_len);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_M_LOG_LOW(
            "rfsvp_mgr_cmd_parser_get_next_command failed: %d", e_error);
         break;
      }

      e_error = rfsvp_mgr_cmd_parser_interpret_cmd_line (px_rfsvp_mgr_ctxt,
         uca_cmd_buf, ui_cmd_buf_len);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_M_LOG_LOW(
            "rfsvp_mgr_cmd_parser_interpret_command failed: %d", e_error);
         break;
      }
      RFSVP_M_LOG_FULL("RFSVP Manager Command Task");
   }

   RFSVP_M_LOG_MED("Out of task loop");
   e_error = rfsvp_mgr_cmd_task_deinit (px_rfsvp_mgr_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_cmd_task_deinit failed: %d", e_error);
   }
   RFSVP_M_LOG_MED("rfsvp_mgr_cmd_task_deinit success");
CLEAN_RETURN:
   RFSVP_M_LOG_MED("Notifying task exit");
   e_task_ret = task_notify_exit (px_rfsvp_mgr_ctxt->hl_listner_task_hdl);
   if (eTASK_RET_SUCCESS != e_task_ret)
   {
      RFSVP_M_LOG_LOW("task_notify_exit failed: %d", e_task_ret);
   }
   else
   {
      RFSVP_M_LOG_MED("task_notify_exit success");
   }
   return p_thread_args;
}

static RFSVP_RET_E rfsvp_mgr_cmd_task_init (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;

   if (NULL == px_rfsvp_mgr_ctxt)
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   e_error = rfsvp_mgr_cmd_parser_init (px_rfsvp_mgr_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW ("rfsvp_mgr_cmd_parser_init failed: %d", e_error);
      goto CLEAN_RETURN;
   }
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_mgr_cmd_task_deinit (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;

   if (NULL == px_rfsvp_mgr_ctxt)
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      goto CLEAN_RETURN;
   }

   e_error = rfsvp_mgr_cmd_parser_deinit (px_rfsvp_mgr_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW ("rfsvp_mgr_cmd_parser_deinit failed: %d", e_error);
   }
CLEAN_RETURN:
   return e_error;
}
