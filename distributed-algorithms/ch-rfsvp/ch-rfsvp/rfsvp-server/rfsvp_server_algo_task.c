/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   rfsvp_server_algo_task.c
 * \author sandeepprakash
 *
 * \date   02-Dec-2012
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
static RFSVP_RET_E rfsvp_server_node_algo_task_init (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt);

static RFSVP_RET_E rfsvp_server_node_algo_task_deinit (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt);

static RFSVP_RET_E rfsvp_server_node_algo_task_proc (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt);

static RFSVP_RET_E rfsvp_server_algo_task_handle_algo_msg (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_hdr);

static RFSVP_RET_E rfsvp_server_node_handle_app_msg_cmd (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header);

static RFSVP_RET_E rfsvp_server_node_handle_app_cmd (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header);

static RFSVP_RET_E rfsvp_server_algo_task_handle_msg (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   RFSVP_SERVER_ALGO_MSG_DATA_X *px_msg_data);

/****************************** LOCAL FUNCTIONS *******************************/
void *rfsvp_server_node_algo_task (
   void *p_thread_args)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   TASK_RET_E e_task_ret = eTASK_RET_FAILURE;
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt = NULL;
   MSGQ_DATA_X x_data = {NULL};
   RFSVP_SERVER_ALGO_MSG_DATA_X *px_msg_data = NULL;

   if (NULL == p_thread_args)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      goto CLEAN_RETURN;
   }

   px_rfsvp_server_ctxt = (RFSVP_SERVER_CTXT_X *) p_thread_args;

   e_error = rfsvp_server_node_algo_task_init (px_rfsvp_server_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW("rfsvp_server_node_algo_task_init failed: %d", e_error);
      goto CLEAN_RETURN;
   }
   RFSVP_S_LOG_MED("rfsvp_server_node_algo_task_init success");

   while (task_is_in_loop (px_rfsvp_server_ctxt->x_algo.hl_algo_task_hdl))
   {
      e_task_ret = task_get_msg_from_q (
         px_rfsvp_server_ctxt->x_algo.hl_algo_task_hdl, &x_data,
         RFSVP_SERVER_ALGO_TASK_Q_WAIT_TIMEOUT);
      if ((eTASK_RET_SUCCESS == e_task_ret) && (NULL != x_data.p_data)
         && (0 != x_data.ui_data_size))
      {
         px_msg_data = (RFSVP_SERVER_ALGO_MSG_DATA_X *) x_data.p_data;
         e_error = rfsvp_server_algo_task_handle_msg (px_rfsvp_server_ctxt,
            px_msg_data);
         if (eRFSVP_RET_SUCCESS != e_error)
         {
            RFSVP_S_LOG_LOW("dimutex_algo_messaging_task_handle_msg failed: %d",
               e_error);
         }
      }

      e_error = rfsvp_server_node_algo_task_proc (px_rfsvp_server_ctxt);
      RFSVP_S_LOG_FULL("RFSVP Server Algo Task");
   }

   RFSVP_S_LOG_MED("Out of task loop");
   e_error = rfsvp_server_node_algo_task_deinit (px_rfsvp_server_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW("rfsvp_server_node_algo_task_deinit failed: %d", e_error);
   }
   RFSVP_S_LOG_MED("rfsvp_server_node_algo_task_deinit success");
CLEAN_RETURN:
   RFSVP_S_LOG_MED("Notifying task exit");
   e_task_ret = task_notify_exit (px_rfsvp_server_ctxt->x_algo.hl_algo_task_hdl);
   if (eTASK_RET_SUCCESS != e_task_ret)
   {
      RFSVP_S_LOG_LOW("task_notify_exit failed: %d", e_task_ret);
   }
   else
   {
      RFSVP_S_LOG_MED("task_notify_exit success");
   }
   return p_thread_args;
}

static RFSVP_RET_E rfsvp_server_node_algo_task_init (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   e_error = rfsvp_server_node_algo_init (px_rfsvp_server_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW("rfsvp_server_node_algo_init failed: %d", e_error);
   }
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_server_node_algo_task_deinit (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   e_error = rfsvp_server_node_algo_deinit (px_rfsvp_server_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW("rfsvp_server_node_algo_deinit failed: %d", e_error);
   }
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_server_node_algo_task_proc (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   switch (px_algo->ui_sub_state)
   {
      case eRFSVP_SERVER_ALGO_SUB_STATE_REACH_REQ:
      {
         RFSVP_S_LOG_MED("Processing eRFSVP_SERVER_ALGO_SUB_STATE_REACH_REQ");
         e_error = rfsvp_server_algo_sub_state_reach_req (px_rfsvp_server_ctxt);
         break;
      }
      case eRFSVP_SERVER_ALGO_SUB_STATE_REACH_WAITING:
      {
         RFSVP_S_LOG_MED(
            "Processing eRFSVP_SERVER_ALGO_SUB_STATE_REACH_WAITING");
         e_error = rfsvp_server_algo_sub_state_reach_waiting (
            px_rfsvp_server_ctxt);
         break;
      }
      case eRFSVP_SERVER_ALGO_SUB_STATE_REACH_REQ_DONE:
      {
         RFSVP_S_LOG_MED(
            "Processing eRFSVP_SERVER_ALGO_SUB_STATE_REACH_REQ_DONE");
         e_error = rfsvp_server_algo_sub_state_reach_req_done (
            px_rfsvp_server_ctxt);
         break;
      }
      case eRFSVP_SERVER_ALGO_SUB_STATE_LOCK_REQ:
      {
         RFSVP_S_LOG_MED("Processing eRFSVP_SERVER_ALGO_SUB_STATE_LOCK_REQ");
         e_error = rfsvp_server_algo_sub_state_lock_req (px_rfsvp_server_ctxt);
         break;
      }
      case eRFSVP_SERVER_ALGO_SUB_STATE_LOCK_REQ_DONE:
      {
         RFSVP_S_LOG_MED(
            "Processing eRFSVP_SERVER_ALGO_SUB_STATE_LOCK_REQ_DONE");
         e_error = rfsvp_server_algo_sub_state_lock_req_done (
            px_rfsvp_server_ctxt);
         break;
      }
      case eRFSVP_SERVER_ALGO_SUB_STATE_VOTE_REQ:
      {
         RFSVP_S_LOG_MED("Processing eRFSVP_SERVER_ALGO_SUB_STATE_VOTE_REQ");
         e_error = rfsvp_server_algo_sub_state_vote_req (px_rfsvp_server_ctxt);
         break;
      }
      case eRFSVP_SERVER_ALGO_SUB_STATE_VOTE_REQ_DONE:
      {
         RFSVP_S_LOG_MED(
            "Processing eRFSVP_SERVER_ALGO_SUB_STATE_VOTE_REQ_DONE");
         e_error = rfsvp_server_algo_sub_state_vote_req_done (
            px_rfsvp_server_ctxt);
         break;
      }
      case eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ:
      {
         RFSVP_S_LOG_MED("Processing eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ");
         e_error = rfsvp_server_algo_sub_state_commit_req (
            px_rfsvp_server_ctxt);
         break;
      }
      case eRFSVP_SERVER_ALGO_SUB_STATE_SYNC_REQ:
      {
         RFSVP_S_LOG_MED("Processing eRFSVP_SERVER_ALGO_SUB_STATE_SYNC_REQ");
         e_error = rfsvp_server_algo_sub_state_sync_req (
            px_rfsvp_server_ctxt);
         break;
      }
      case eRFSVP_SERVER_ALGO_SUB_STATE_SYNC_PROGRESS:
      {
         RFSVP_S_LOG_MED(
            "Processing eRFSVP_SERVER_ALGO_SUB_STATE_SYNC_PROGRESS");
         e_error = rfsvp_server_algo_sub_state_sync_progress (
            px_rfsvp_server_ctxt);
         break;
      }
      case eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ_DONE:
      {
         RFSVP_S_LOG_MED(
            "Processing eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ_DONE");
         e_error = rfsvp_server_algo_sub_state_commit_req_done (
            px_rfsvp_server_ctxt);
         break;
      }
      case eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ:
      {
         RFSVP_S_LOG_MED(
            "Processing eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ");
         e_error = rfsvp_server_algo_sub_state_abort_req (
            px_rfsvp_server_ctxt);
         break;
      }
      case eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ_DONE:
      {
         RFSVP_S_LOG_MED(
            "Processing eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ_DONE");
         e_error = rfsvp_server_algo_sub_state_abort_req_done (
            px_rfsvp_server_ctxt);
         break;
      }
      case eRFSVP_SERVER_ALGO_SUB_STATE_COMPLETE:
      {
         RFSVP_S_LOG_MED( "Processing eRFSVP_SERVER_ALGO_SUB_STATE_COMPLETE");
         e_error = rfsvp_server_algo_sub_state_complete (
            px_rfsvp_server_ctxt);
         break;
      }
      default:
      {
         /* Initial State. */
         e_error = eRFSVP_RET_SUCCESS;
         break;
      }
   }

   if (eRFSVP_SERVER_ALGO_STATE_IN_PROGRESS == px_algo->e_state)
   {
      goto CLEAN_RETURN;
   }


   switch (px_algo->ui_wait_state)
   {
      case eRFSVP_SERVER_ALGO_WAIT_STATE_NEW_REQ_WAITING:
      {
         MSGQ_RET_E e_msgq_ret = eMSGQ_RET_FAILURE;
         MSGQ_DATA_X x_data = {NULL};
         NODE_MSG_HDR_X *px_msg_hdr = NULL;
         uint32_t ui_q_size = 0;

         RFSVP_S_LOG_MED(
               "Processing eRFSVP_SERVER_ALGO_WAIT_STATE_NEW_REQ_WAITING");

         e_msgq_ret = msgq_get_msg (
            px_rfsvp_server_ctxt->x_algo.hl_algo_req_msgq_hdl, &x_data,
            RFSVP_SERVER_ALGO_TASK_Q_WAIT_TIMEOUT);
         if ((eMSGQ_RET_SUCCESS == e_msgq_ret) && (NULL != x_data.p_data)
            && (0 != x_data.ui_data_size))
         {
            RFSVP_S_LOG_MED( "Request was waiting...processing it!");

            px_msg_hdr = (NODE_MSG_HDR_X *) x_data.p_data;
            e_error = rfsvp_server_algo_task_handle_algo_msg (
               px_rfsvp_server_ctxt, px_msg_hdr);
            if (eRFSVP_RET_SUCCESS != e_error)
            {
               RFSVP_S_LOG_LOW(
                  "rfsvp_server_algo_task_handle_algo_msg failed: %d", e_error);
            }

            e_msgq_ret = msgq_get_filled_q_size (
               px_rfsvp_server_ctxt->x_algo.hl_algo_req_msgq_hdl, &ui_q_size);
            if ((eMSGQ_RET_SUCCESS != e_msgq_ret) && (ui_q_size > 0))
            {
               if (eRFSVP_SERVER_ALGO_STATE_IN_PROGRESS == px_algo->e_state)
               {
                  px_algo->ui_wait_state |=
                     eRFSVP_SERVER_ALGO_WAIT_STATE_NEW_REQ_WAITING;
               }
            }
         }
         else
         {
            RFSVP_S_LOG_MED( "No request was waiting");
            px_algo->ui_wait_state &=
               ~eRFSVP_SERVER_ALGO_WAIT_STATE_NEW_REQ_WAITING;
         }
         break;
      }
      default:
      {
         /* Initial State. */
         e_error = eRFSVP_RET_SUCCESS;
         break;
      }
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_post_msg_to_algo_task_q (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint8_t *puc_data,
   uint32_t ui_data_len)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   TASK_RET_E e_task_ret = eTASK_RET_FAILURE;
   MSGQ_DATA_X x_data = {NULL};

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == puc_data) || (0 == ui_data_len))
   {
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_data.p_data = puc_data;
   x_data.ui_data_size = ui_data_len;
   e_task_ret = task_add_msg_to_q(px_rfsvp_server_ctxt->x_algo.hl_algo_task_hdl,
      &x_data, RFSVP_SERVER_ALGO_TASK_Q_WAIT_TIMEOUT);
   if (eTASK_RET_SUCCESS != e_task_ret)
   {
      RFSVP_S_LOG_LOW("task_add_msg_to_q failed: %d", e_error);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }

CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_server_algo_task_handle_msg (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   RFSVP_SERVER_ALGO_MSG_DATA_X *px_msg_data)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_HDR_X *px_msg_hdr = NULL;
   bool b_free = false;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_data))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   if (px_msg_data->ui_data_len < sizeof(NODE_MSG_HDR_X))
   {
      RFSVP_S_LOG_LOW("Invalid Args. Invalid px_msg_data->ui_data_len: %d",
         px_msg_data->ui_data_len);
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   if (NULL == px_msg_data->puc_data)
   {
      RFSVP_S_LOG_LOW("Invalid Args. Invalid px_msg_data->puc_data: %p",
         px_msg_data->puc_data);
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_msg_hdr = (NODE_MSG_HDR_X *) px_msg_data->puc_data;

   if (eNODE_MSG_ID_APP_MSG_CMD == px_msg_hdr->ui_msg_id)
   {
      b_free = false;
   }
   else
   {
      b_free = true;
   }

   switch (px_msg_hdr->ui_msg_id)
   {
      case eNODE_MSG_ID_APP_MSG_CMD:
      {
         RFSVP_S_LOG_MED("Got eNODE_MSG_ID_APP_MSG_CMD");
         e_error = rfsvp_server_node_handle_app_msg_cmd (px_rfsvp_server_ctxt,
            px_msg_hdr);
         break;
      }
      case eNODE_MSG_ID_ALGO_MSG_IS_REACHABLE:
      case eNODE_MSG_ID_ALGO_MSG_IS_REACHABLE_RSP:
      case eNODE_MSG_ID_ALGO_MSG_VOTE_REQ:
      case eNODE_MSG_ID_ALGO_MSG_VOTE_RSP:
      case eNODE_MSG_ID_ALGO_MSG_COMMIT_REQ:
      case eNODE_MSG_ID_ALGO_MSG_COMMIT_RSP:
      case eNODE_MSG_ID_ALGO_MSG_SYNC_REQ:
      case eNODE_MSG_ID_ALGO_MSG_SYNC_RSP:
      case eNODE_MSG_ID_ALGO_MSG_SYNC_UPDATE:
      case eNODE_MSG_ID_ALGO_MSG_ABORT_REQ:
      case eNODE_MSG_ID_ALGO_MSG_ABORT_RSP:
      case eNODE_MSG_ID_ALGO_MSG_LOCK_REQ:
      case eNODE_MSG_ID_ALGO_MSG_LOCK_RSP:
      {
         e_error = rfsvp_server_algo_task_handle_algo_msg (
            px_rfsvp_server_ctxt, px_msg_hdr);
         break;
      }
      default:
      {
         RFSVP_S_LOG_LOW("Invalid message: %d received.",
            px_msg_hdr->ui_msg_id);
         e_error = eRFSVP_RET_INVALID_ARGS;
         break;
      }
   }

CLEAN_RETURN:
   if (NULL != px_msg_data)
   {
      if (NULL != px_msg_data->puc_data)
      {
         px_msg_hdr = (NODE_MSG_HDR_X *) px_msg_data->puc_data;
         if (true == b_free)
         {
            pal_free(px_msg_data->puc_data);
            px_msg_data->puc_data = NULL;
         }
      }
      pal_free(px_msg_data);
      px_msg_data = NULL;
   }
   return e_error;
}

static RFSVP_RET_E rfsvp_server_algo_task_handle_algo_msg (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_hdr)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_hdr))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   switch (px_msg_hdr->ui_msg_id)
   {
      case eNODE_MSG_ID_ALGO_MSG_IS_REACHABLE:
      {
         RFSVP_S_LOG_MED("Got eNODE_MSG_ID_ALGO_MSG_IS_REACHABLE: %d",
            px_msg_hdr->ui_node_index);
         e_error = rfsvp_server_node_handle_algo_msg_is_reachable (
            px_rfsvp_server_ctxt, px_msg_hdr);
         break;
      }
      case eNODE_MSG_ID_ALGO_MSG_IS_REACHABLE_RSP:
      {
         RFSVP_S_LOG_MED("Got eNODE_MSG_ID_ALGO_MSG_IS_REACHABLE_RSP: %d",
            px_msg_hdr->ui_node_index);
         e_error = rfsvp_server_node_handle_algo_msg_is_reachable_rsp (
            px_rfsvp_server_ctxt, px_msg_hdr);
         break;
      }
      case eNODE_MSG_ID_ALGO_MSG_VOTE_REQ:
      {
         RFSVP_S_LOG_MED("Got eNODE_MSG_ID_ALGO_MSG_VOTE_REQ: %d",
            px_msg_hdr->ui_node_index);
         e_error = rfsvp_server_node_handle_algo_msg_vote_req (
            px_rfsvp_server_ctxt, px_msg_hdr);
         break;
      }
      case eNODE_MSG_ID_ALGO_MSG_VOTE_RSP:
      {
         RFSVP_S_LOG_MED("Got eNODE_MSG_ID_ALGO_MSG_VOTE_RSP: %d",
            px_msg_hdr->ui_node_index);
         e_error = rfsvp_server_node_handle_algo_msg_vote_rsp (
            px_rfsvp_server_ctxt, px_msg_hdr);
         break;
      }
      case eNODE_MSG_ID_ALGO_MSG_COMMIT_REQ:
      {
         RFSVP_S_LOG_MED("Got eNODE_MSG_ID_ALGO_MSG_COMMIT_REQ: %d",
            px_msg_hdr->ui_node_index);
         e_error = rfsvp_server_node_handle_algo_msg_commit_req (
            px_rfsvp_server_ctxt, px_msg_hdr);
         break;
      }
      case eNODE_MSG_ID_ALGO_MSG_COMMIT_RSP:
      {
         RFSVP_S_LOG_MED("Got eNODE_MSG_ID_ALGO_MSG_COMMIT_RSP: %d",
            px_msg_hdr->ui_node_index);
         e_error = rfsvp_server_node_handle_algo_msg_commit_rsp (
            px_rfsvp_server_ctxt, px_msg_hdr);
         break;
      }
      case eNODE_MSG_ID_ALGO_MSG_SYNC_REQ:
      {
         RFSVP_S_LOG_MED("Got eNODE_MSG_ID_ALGO_MSG_SYNC_REQ: %d",
            px_msg_hdr->ui_node_index);
         e_error = rfsvp_server_node_handle_algo_msg_sync_req (
            px_rfsvp_server_ctxt, px_msg_hdr);
         break;
      }
      case eNODE_MSG_ID_ALGO_MSG_SYNC_RSP:
      {
         RFSVP_S_LOG_MED("Got eNODE_MSG_ID_ALGO_MSG_SYNC_RSP: %d",
            px_msg_hdr->ui_node_index);
         e_error = rfsvp_server_node_handle_algo_msg_sync_rsp (
            px_rfsvp_server_ctxt, px_msg_hdr);
         break;
      }
      case eNODE_MSG_ID_ALGO_MSG_SYNC_UPDATE:
      {
         RFSVP_S_LOG_MED("Got eNODE_MSG_ID_ALGO_MSG_SYNC_UPDATE: %d",
            px_msg_hdr->ui_node_index);
         e_error = rfsvp_server_node_handle_algo_msg_sync_update (
            px_rfsvp_server_ctxt, px_msg_hdr);
         break;
      }
      case eNODE_MSG_ID_ALGO_MSG_ABORT_REQ:
      {
         RFSVP_S_LOG_MED("Got eNODE_MSG_ID_ALGO_MSG_ABORT_REQ: %d",
            px_msg_hdr->ui_node_index);
         e_error = rfsvp_server_node_handle_algo_msg_abort_req (
            px_rfsvp_server_ctxt, px_msg_hdr);
         break;
      }
      case eNODE_MSG_ID_ALGO_MSG_ABORT_RSP:
      {
         RFSVP_S_LOG_MED("Got eNODE_MSG_ID_ALGO_MSG_ABORT_RSP: %d",
            px_msg_hdr->ui_node_index);
         e_error = rfsvp_server_node_handle_algo_msg_abort_rsp (
            px_rfsvp_server_ctxt, px_msg_hdr);
         break;
      }
      case eNODE_MSG_ID_ALGO_MSG_LOCK_REQ:
      {
         RFSVP_S_LOG_MED("Got eNODE_MSG_ID_ALGO_MSG_LOCK_REQ: %d",
            px_msg_hdr->ui_node_index);
         e_error = rfsvp_server_node_handle_algo_msg_lock_req (
            px_rfsvp_server_ctxt, px_msg_hdr);
         break;
      }
      case eNODE_MSG_ID_ALGO_MSG_LOCK_RSP:
      {
         RFSVP_S_LOG_MED("Got eNODE_MSG_ID_ALGO_MSG_LOCK_RSP: %d",
            px_msg_hdr->ui_node_index);
         e_error = rfsvp_server_node_handle_algo_msg_lock_rsp (
            px_rfsvp_server_ctxt, px_msg_hdr);
         break;
      }
      default:
      {
         RFSVP_S_LOG_LOW("Invalid message: %d received.",
            px_msg_hdr->ui_msg_id);
         e_error = eRFSVP_RET_INVALID_ARGS;
         break;
      }
   }

CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_server_node_handle_app_msg_cmd (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;

   RFSVP_SERVER_ALGO_X *px_algo = NULL;
#if 1
   MSGQ_DATA_X x_data = {NULL};
   MSGQ_RET_E e_msgq_ret = eMSGQ_RET_FAILURE;
#else
   NODE_MSG_APP_CMD_X *px_cmd = NULL;
   NODE_MSG_APP_CMD_RSP_X x_rsp = {{0}};
#endif
   bool b_free = false;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

#if 0
   px_cmd = (NODE_MSG_APP_CMD_X *) px_msg_header;
#endif

   if (eRFSVP_SERVER_ALGO_STATE_IN_PROGRESS == px_algo->e_state)
   {
      RFSVP_S_LOG_LOW ("An algo request is being processed already. "
         "ABORTING the request");

#if 1
      x_data.p_data = px_msg_header;
      x_data.ui_data_size = sizeof(px_msg_header)
         + px_msg_header->ui_msg_pay_len;
      e_msgq_ret = msgq_add_msg (px_algo->hl_app_req_msgq_hdl, &x_data, 0);
      if (eMSGQ_RET_SUCCESS != e_msgq_ret)
      {
         if (eMSGQ_RET_OP_TIMEDOUT == e_msgq_ret)
         {
            RFSVP_S_LOG_LOW("msgq_add_msg timedout: %d", e_msgq_ret);
            e_error = eRFSVP_RET_RESOURCE_FAILURE;
         }
         else
         {
            RFSVP_S_LOG_LOW("msgq_add_msg failed: %d", e_msgq_ret);
            e_error = eRFSVP_RET_RESOURCE_FAILURE;
         }
         b_free = true;
      }
      else
      {
         e_error = eRFSVP_RET_SUCCESS;
         b_free = false;
      }
#else
      x_rsp.x_hdr.ui_msg_id = eNODE_MSG_ID_APP_MSG_CMD_RSP;
      x_rsp.x_hdr.ui_msg_pay_len = sizeof(x_rsp) - sizeof(x_rsp.x_hdr);

      x_rsp.e_cmd = px_cmd->e_cmd;
      x_rsp.ui_server_idx = px_rfsvp_server_ctxt->x_init_params.ui_node_index;
      x_rsp.e_rsp_code = eNODE_MSG_CMD_RSP_CODE_ABORTED;
      e_error = rfsvp_server_node_send_app_msg_cmd_rsp_to_mgr (
         px_rfsvp_server_ctxt, &x_rsp);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_S_LOG_LOW(
            "rfsvp_server_node_send_app_msg_cmd_rsp_to_mgr failed: %d", e_error);
      }
      b_free = true;
#endif
      goto CLEAN_RETURN;
   }
   else
   {
      b_free = true;
   }

   e_error = rfsvp_server_node_handle_app_cmd (px_rfsvp_server_ctxt,
      px_msg_header);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW( "rfsvp_server_node_handle_app_cmd failed: %d", e_error);
   }
CLEAN_RETURN:
   if (true == b_free)
   {
      if (NULL != px_msg_header)
      {
         pal_free (px_msg_header);
         px_msg_header = NULL;
      }
   }
   return e_error;
}

static RFSVP_RET_E rfsvp_server_node_handle_app_cmd (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_APP_CMD_X *px_cmd = NULL;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_cmd = (NODE_MSG_APP_CMD_X *) px_msg_header;

   switch (px_cmd->e_cmd)
   {
      case eRFSVP_SERVER_CMD_HALT:
      {
         RFSVP_S_LOG_MED("Got eRFSVP_SERVER_CMD_HALT");
         e_error = rfsvp_server_node_handle_app_msg_halt (px_rfsvp_server_ctxt,
            px_msg_header);
         break;
      }
      case eRFSVP_SERVER_CMD_NODE_DOWN:
      {
         RFSVP_S_LOG_MED("Got eRFSVP_SERVER_CMD_NODE_DOWN");
         e_error = rfsvp_server_node_handle_app_msg_node_down (
            px_rfsvp_server_ctxt, px_msg_header);
         break;
      }
      case eRFSVP_SERVER_CMD_NODE_UP:
      {
         RFSVP_S_LOG_MED("Got eRFSVP_SERVER_CMD_NODE_UP");
         e_error = rfsvp_server_node_handle_app_msg_node_up (
            px_rfsvp_server_ctxt, px_msg_header);
         break;
      }
      case eRFSVP_SERVER_CMD_READ:
      {
         RFSVP_S_LOG_MED("Got eRFSVP_SERVER_CMD_READ");
         e_error = rfsvp_server_node_handle_app_msg_read (px_rfsvp_server_ctxt,
            px_msg_header);
         break;
      }
      case eRFSVP_SERVER_CMD_WRITE:
      {
         RFSVP_S_LOG_MED("Got eRFSVP_SERVER_CMD_WRITE");
         e_error = rfsvp_server_node_handle_app_msg_write (px_rfsvp_server_ctxt,
            px_msg_header);
         break;
      }
      default:
      {
         e_error = eRFSVP_RET_INVALID_ARGS;
         break;
      }
   }

   if (eRFSVP_RET_INVALID_ARGS == e_error)
   {
      goto CLEAN_RETURN;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}
