/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   rfsvp_server_algo_sub_state.c
 * \author sandeepprakash
 *
 * \date   03-Dec-2012
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
static RFSVP_RET_E rfsvp_server_algo_honor_cur_read_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_req_msg_hdr);

static RFSVP_RET_E rfsvp_server_algo_honor_cur_write_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_req_msg_hdr);

RFSVP_RET_E rfsvp_server_algo_prepare_sync_msg (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t ui_index,
   NODE_MSG_ALGO_SYNC_UPDATE_X **ppx_sync_update);

RFSVP_RET_E rfsvp_server_algo_sync_reachable_node (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t ui_index);

static RFSVP_RET_E rfsvp_server_algo_sync_other_reachable_nodes (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt);

static RFSVP_RET_E rfsvp_server_algo_honor_cur_node_up_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_req_msg_hdr);

static RFSVP_RET_E rfsvp_server_algo_honor_cur_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt);

/****************************** LOCAL FUNCTIONS *******************************/
RFSVP_RET_E rfsvp_server_algo_sub_state_reach_req (
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

   (void) pal_memset (px_algo->ba_reachable, 0x00,
      sizeof(px_algo->ba_reachable));

   e_error = rfsvp_server_node_send_is_reachable_to_all (px_rfsvp_server_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW( "rfsvp_server_node_send_is_reachable_to_all failed: %d",
         e_error);
   }
   else
   {
      px_algo->ui_reachability_start_ms = pal_get_system_time ();
      RFSVP_S_LOG_MED("Reachability check started @ %d ms",
         px_algo->ui_reachability_start_ms);

      px_algo->ba_reachable [px_rfsvp_server_ctxt->x_init_params.ui_node_index] =
         true;

      px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_REACH_REQ;
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_REACH_WAITING");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_REACH_WAITING;
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

#define RFSVP_SERVER_REACHABILITY_CHECK_TIMEOUT_MS       (2000)

RFSVP_RET_E rfsvp_server_algo_sub_state_reach_waiting (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   uint32_t ui_cur_time_ms = 0;
   uint32_t ui_elapsed_time_ms = 0;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   ui_cur_time_ms = pal_get_system_time();
   ui_elapsed_time_ms = ui_cur_time_ms - px_algo->ui_reachability_start_ms;
   /*
    * Run the timer for the response here. If the timer expires then set the
    * sub state to eRFSVP_SERVER_ALGO_SUB_STATE_REACH_REQ_DONE.
    */
   if (ui_elapsed_time_ms >= RFSVP_SERVER_REACHABILITY_CHECK_TIMEOUT_MS)
   {
      RFSVP_S_LOG_MED("Reachability check ended @ %d ms. Timer expired: %d",
         ui_cur_time_ms, ui_elapsed_time_ms);
      RFSVP_S_LOG_MED("Reachability only partial. Those that have responded.");
      px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_REACH_WAITING;
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_REACH_REQ_DONE");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_REACH_REQ_DONE;
   }
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_algo_sub_state_reach_req_done (
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


   px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_REACH_REQ_DONE;
   RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_LOCK_REQ");
   px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_LOCK_REQ;
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_algo_sub_state_lock_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   uint32_t ui_i = 0;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   uint32_t ui_count = 0;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   if (NULL == px_algo->px_cur_req_msg_hdr)
   {
      RFSVP_S_LOG_LOW("Invalid px_algo->px_cur_req_msg_hdr");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   (void) pal_memset (px_algo->ba_lock_rsp, 0x00, sizeof(px_algo->ba_lock_rsp));

   for (ui_i = 0; ui_i < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes;
         ui_i++)
   {
      if (true == px_rfsvp_server_ctxt->x_algo.ba_reachable [ui_i])
      {
         if (ui_i == px_rfsvp_server_ctxt->x_init_params.ui_node_index)
         {
            RFSVP_S_LOG_LOW( "Not sending to my node: %d", ui_i);
            continue;
         }
         e_error = rfsvp_server_node_send_lock_req (px_rfsvp_server_ctxt, ui_i);
         if (eRFSVP_RET_SUCCESS != e_error)
         {
            RFSVP_S_LOG_LOW( "rfsvp_server_node_send_lock_req failed: %d",
               e_error);
            break;
         }
         ui_count++;
      }
      else
      {
         RFSVP_S_LOG_LOW( "Not sending to unreachable node: %d", ui_i);
         e_error = eRFSVP_RET_SUCCESS;
      }
   }
   if (ui_i == px_rfsvp_server_ctxt->x_init_params.ui_no_nodes)
   {
      if (true
         == px_algo->ba_reachable [px_rfsvp_server_ctxt->x_init_params.ui_node_index])
      {
         px_algo->ba_lock_rsp [px_rfsvp_server_ctxt->x_init_params.ui_node_index] =
            true;
      }

      if (0 == ui_count)
      {
         RFSVP_S_LOG_MED("All nodes have responded to LOCK REQUEST");
         px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_LOCK_REQ;
         RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_VOTE_REQ");
         px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_VOTE_REQ;
      }
      else
      {
         px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_LOCK_REQ;
         RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_LOCK_REQ_DONE");
         px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_LOCK_REQ_DONE;
      }
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_algo_sub_state_lock_req_done (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   RFSVP_S_LOG_MED("Waiting for eNODE_MSG_ID_ALGO_MSG_LOCK_RSP");
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_algo_sub_state_vote_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   uint32_t ui_i = 0;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   NODE_MSG_APP_CMD_X *px_app_cmd = NULL;
   NODE_VOTE_REQ_PURPOSE_E e_purpose = eNODE_VOTE_REQ_PURPOSE_INVALID;
   uint32_t ui_count = 0;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   if (NULL == px_algo->px_cur_req_msg_hdr)
   {
      RFSVP_S_LOG_LOW("Invalid px_algo->px_cur_req_msg_hdr");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_app_cmd = (NODE_MSG_APP_CMD_X *) px_algo->px_cur_req_msg_hdr;

   switch (px_app_cmd->e_cmd)
   {
      case eRFSVP_SERVER_CMD_NODE_UP:
      {
         e_purpose = eNODE_VOTE_REQ_PURPOSE_QUERY;
         break;
      }
      case eRFSVP_SERVER_CMD_READ:
      case eRFSVP_SERVER_CMD_WRITE:
      {
         e_purpose = eNODE_VOTE_REQ_PURPOSE_VOTING;
         break;
      }
      default:
      {
         break;
      }
   }

   (void) pal_memset (px_algo->ba_vote_rsp, 0x00, sizeof(px_algo->ba_vote_rsp));

   for (ui_i = 0; ui_i < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes; ui_i++)
   {
      if (true == px_rfsvp_server_ctxt->x_algo.ba_reachable[ui_i])
      {
         if (ui_i == px_rfsvp_server_ctxt->x_init_params.ui_node_index)
         {
            RFSVP_S_LOG_LOW( "Not sending to my node: %d", ui_i);
            continue;
         }
         e_error = rfsvp_server_node_send_vote_req (px_rfsvp_server_ctxt, ui_i,
            e_purpose);
         if (eRFSVP_RET_SUCCESS != e_error)
         {
            RFSVP_S_LOG_LOW( "rfsvp_server_node_send_vote_req failed: %d",
               e_error);
            break;
         }
         ui_count++;
      }
      else
      {
         RFSVP_S_LOG_LOW( "Not sending to unreachable node: %d", ui_i);
         e_error = eRFSVP_RET_SUCCESS;
      }
   }
   if (ui_i == px_rfsvp_server_ctxt->x_init_params.ui_no_nodes)
   {
      if (true
         == px_algo->ba_reachable [px_rfsvp_server_ctxt->x_init_params.ui_node_index])
      {
         px_algo->ba_vote_rsp [px_rfsvp_server_ctxt->x_init_params.ui_node_index] =
            true;

         (void) pal_memmove (
            &(px_algo->xa_nodex_vp [px_rfsvp_server_ctxt->x_init_params.ui_node_index]),
            &(px_algo->x_vp),
            sizeof(px_algo->xa_nodex_vp [px_rfsvp_server_ctxt->x_init_params.ui_node_index]));
      }

      if (0 == ui_count)
      {
         RFSVP_S_LOG_MED("All nodes have responded to VOTE REQUEST");

         rfsvp_server_node_print_voting_table (px_rfsvp_server_ctxt);
         px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_VOTE_REQ;
         RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ");
         px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ;
      }
      else
      {
         px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_VOTE_REQ;
         RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_VOTE_REQ_DONE");
         px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_VOTE_REQ_DONE;
      }
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_algo_sub_state_vote_req_done (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   RFSVP_S_LOG_MED("Waiting for eNODE_MSG_ID_ALGO_MSG_VOTE_RSP");
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_server_algo_honor_cur_read_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_req_msg_hdr)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   uint32_t ui_line_len = 0;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   /*
    * 3. If the request is READ, sends the reply with last line of the file.
    */
   e_pal_ret = pal_fseek(px_algo->hl_file_hdl, px_algo->ui_last_line_off,
      ePAL_FILE_WHENCE_SEEK_SET);
   if (ePAL_RET_SUCCESS != e_pal_ret)
   {
      RFSVP_S_LOG_LOW("pal_fseek failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   (void) pal_memset (px_algo->uca_read_data, 0x00,
      sizeof(px_algo->uca_read_data));
   e_pal_ret = pal_freadline (px_algo->hl_file_hdl, px_algo->uca_read_data,
      sizeof(px_algo->uca_read_data), &ui_line_len);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (0 == ui_line_len))
   {
      RFSVP_S_LOG_LOW( "pal_freadline failed: %d, %d", e_pal_ret, ui_line_len);
      px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ;
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ;

      px_algo->e_cur_req_rsp_code = eNODE_MSG_CMD_RSP_CODE_FILE_EMPTY;

      e_error = eRFSVP_RET_SUCCESS;
      goto CLEAN_RETURN;
   }

   e_pal_ret = pal_fseek(px_algo->hl_file_hdl, px_algo->ui_cur_file_off,
      ePAL_FILE_WHENCE_SEEK_SET);
   if (ePAL_RET_SUCCESS != e_pal_ret)
   {
      RFSVP_S_LOG_LOW("pal_fseek failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ;
   RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ");
   px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ;

   px_algo->e_cur_req_rsp_code = eNODE_MSG_CMD_RSP_CODE_SUCCESS;

   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_algo_prepare_sync_msg (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t ui_index,
   NODE_MSG_ALGO_SYNC_UPDATE_X **ppx_sync_update)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   RFSVP_SERVER_ALGO_VP_X *px_their_vp = NULL;
   uint32_t ui_total_data_len = 0;
   uint32_t ui_alloc_len = 0;
   NODE_MSG_ALGO_SYNC_UPDATE_X *px_sync_update = NULL;
   uint8_t *puc_data_off = NULL;
   uint32_t ui_read_len = 0;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == ppx_sync_update))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);
   px_their_vp = &(px_algo->xa_nodex_vp [ui_index]);

   ui_total_data_len = px_algo->ui_cur_file_off - px_their_vp->ui_cur_file_off;
   ui_alloc_len = sizeof(*px_sync_update) + ui_total_data_len;

   px_sync_update = pal_malloc (ui_alloc_len, NULL);
   if (NULL == px_sync_update)
   {
      RFSVP_S_LOG_LOW("pal_malloc failed");
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   px_sync_update->x_hdr.ui_msg_id = eNODE_MSG_ID_ALGO_MSG_SYNC_UPDATE;
   px_sync_update->x_hdr.ui_msg_pay_len = ui_alloc_len
      - sizeof(px_sync_update->x_hdr);

   px_sync_update->ui_data_len = ui_total_data_len;
   px_sync_update->ui_final_offset = px_algo->ui_cur_file_off;
   px_sync_update->ui_sync_vn = px_algo->ui_cur_max_vn;

   RFSVP_S_LOG_LOW("Need to fetch: %d, Seeking to %d from current %d",
      ui_total_data_len, px_their_vp->ui_cur_file_off,
      px_algo->ui_cur_file_off);
   e_pal_ret = pal_fseek(px_algo->hl_file_hdl, px_their_vp->ui_cur_file_off,
      ePAL_FILE_WHENCE_SEEK_SET);
   if (ePAL_RET_SUCCESS != e_pal_ret)
   {
      RFSVP_S_LOG_LOW("pal_fseek failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   puc_data_off = (uint8_t *) &(px_sync_update->ui_data_len);
   puc_data_off += sizeof(px_sync_update->ui_data_len);

   ui_read_len = ui_total_data_len;
   e_pal_ret = pal_fread (px_algo->hl_file_hdl, puc_data_off, &ui_read_len);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (ui_read_len != ui_total_data_len))
   {
      RFSVP_S_LOG_LOW("pal_fread failed: %d, %d", e_pal_ret, ui_read_len);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      RFSVP_S_LOG_LOW("Fetched: %d, Seeking back to %d from current %d",
         ui_total_data_len, px_algo->ui_cur_file_off,
         px_their_vp->ui_cur_file_off);
      e_pal_ret = pal_fseek (px_algo->hl_file_hdl, px_algo->ui_cur_file_off,
         ePAL_FILE_WHENCE_SEEK_SET);
      if (ePAL_RET_SUCCESS != e_pal_ret)
      {
         RFSVP_S_LOG_LOW("pal_fseek failed: %d", e_pal_ret);
         e_error = eRFSVP_RET_RESOURCE_FAILURE;
      }
      else
      {
         *ppx_sync_update = px_sync_update;
         e_error = eRFSVP_RET_SUCCESS;
      }
   }
CLEAN_RETURN:
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      if (NULL != px_sync_update)
      {
         pal_free (px_sync_update);
         px_sync_update = NULL;
      }
   }
   return e_error;
}

RFSVP_RET_E rfsvp_server_algo_sync_reachable_node (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t ui_index)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_ALGO_SYNC_UPDATE_X *px_sync_update = NULL;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   e_error = rfsvp_server_algo_prepare_sync_msg (px_rfsvp_server_ctxt, ui_index,
      &px_sync_update);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW( "rfsvp_server_algo_sync_reachable_node failed: %d",
         e_error);
      goto CLEAN_RETURN;
   }

   e_error = rfsvp_server_node_send_sync_update (px_rfsvp_server_ctxt, ui_index,
      px_sync_update);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW( "rfsvp_server_algo_sync_reachable_node failed: %d",
         e_error);
   }
CLEAN_RETURN:
   if (NULL != px_sync_update)
   {
      pal_free (px_sync_update);
      px_sync_update = NULL;
   }
   return e_error;
}

static RFSVP_RET_E rfsvp_server_algo_sync_other_reachable_nodes (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   RFSVP_SERVER_ALGO_VP_X *px_vp = NULL;
   uint32_t ui_i = 0;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   for (ui_i = 0; ui_i < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes;
         ui_i++)
   {
      if (true == px_algo->ba_reachable [ui_i])
      {
         if (ui_i == px_rfsvp_server_ctxt->x_init_params.ui_node_index)
         {
            continue;
         }
         px_vp = &(px_algo->xa_nodex_vp [ui_i]);

         if (px_vp->ui_vn < px_algo->ui_cur_max_vn)
         {
            RFSVP_S_LOG_LOW("Node %d has an old version: %d (%d), Need to be "
            "@ %d (%d), Need to send sync data to it!",
               ui_i, px_vp->ui_vn, px_vp->ui_cur_file_off,
               px_algo->ui_cur_max_vn, px_algo->ui_cur_file_off);

            e_error = rfsvp_server_algo_sync_reachable_node (
               px_rfsvp_server_ctxt, ui_i);
            if (eRFSVP_RET_SUCCESS != e_error)
            {
               RFSVP_S_LOG_LOW(
                  "rfsvp_server_algo_sync_reachable_node failed: %d", e_error);
               break;
            }
         }
         else
         {
            e_error = eRFSVP_RET_SUCCESS;
         }
      }
      else
      {
         e_error = eRFSVP_RET_SUCCESS;
      }
   }
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_server_algo_honor_cur_write_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_req_msg_hdr)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   RFSVP_SERVER_ALGO_VP_X *px_vp = NULL;
   uint32_t ui_i = 0;
   NODE_MSG_APP_CMD_X *px_app_cmd = NULL;
   uint8_t *puc_data_off = 0;
   uint32_t ui_data_len = 0;
   uint32_t ui_temp = 0;
   uint8_t uca_write_buf[RFSVP_SERVER_MAX_FILE_WRITE_BUF] = {0};
   uint8_t uca_ds_buf[RFSVP_SERVER_MAX_FILE_WRITE_BUF] = {0};
   uint32_t ui_cur_ds_buf_len = 0;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_req_msg_hdr))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);
   px_vp = &(px_algo->x_vp);

   e_error = rfsvp_server_algo_sync_other_reachable_nodes (
      px_rfsvp_server_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW(
         "rfsvp_server_algo_sync_other_reachable_nodes failed: %d", e_error);
      px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ;
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ;
      px_algo->e_cur_req_rsp_code = eNODE_MSG_CMD_RSP_CODE_ABORTED;
      goto CLEAN_RETURN;
   }


   /*
    * 4. If the request is WRITE:
    *    a. Update <VN[i], RU[i], DS[i]> by calling Do-Update procedure.
    *    b. Append the following to the local copy of file:
    *       <VN[i], RU[i], DS[i], text>.
    *    c. Send COMMIT request with <VN[i], RU[i], DS[i], text> to all nodes
    *       in P[i] besides itself.
    */
   e_error = rfsvp_server_node_do_update (px_rfsvp_server_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW( "rfsvp_server_node_do_update failed: %d", e_error);
      px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ;
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ;
      px_algo->e_cur_req_rsp_code = eNODE_MSG_CMD_RSP_CODE_ABORTED;
      goto CLEAN_RETURN;
   }

   px_app_cmd = (NODE_MSG_APP_CMD_X *) px_req_msg_hdr;

   for (ui_i = 0; ui_i < px_vp->ui_ds_count; ui_i++)
   {
      ui_cur_ds_buf_len = pal_strnlen (uca_ds_buf, sizeof(uca_ds_buf));
      snprintf (((char *) uca_ds_buf) + ui_cur_ds_buf_len,
         (sizeof(uca_ds_buf) - ui_cur_ds_buf_len), "%d, ", px_vp->uia_ds[ui_i]);
   }

   puc_data_off = (uint8_t *) &(px_app_cmd->ui_data_len);
   puc_data_off += sizeof(px_app_cmd->ui_data_len);

   snprintf ((char *) uca_write_buf, sizeof(uca_write_buf),
      "%4d, %2d, {%24s}, %s\n",
      px_vp->ui_vn, px_vp->ui_ru, uca_ds_buf, puc_data_off);

   ui_data_len = pal_strnlen (uca_write_buf, sizeof(uca_write_buf)); // px_app_cmd->ui_data_len;
   ui_temp = ui_data_len;
   e_pal_ret = pal_fwrite(px_algo->hl_file_hdl, uca_write_buf, &ui_data_len);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (ui_data_len != ui_temp))
   {
      RFSVP_S_LOG_LOW( "pal_fwrite failed: %d", e_pal_ret);
      px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ;
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ;
      px_algo->e_cur_req_rsp_code = eNODE_MSG_CMD_RSP_CODE_ABORTED;
      e_error = eRFSVP_RET_SUCCESS;
      goto CLEAN_RETURN;
   }

   px_algo->ui_last_line_off = px_algo->ui_cur_file_off;

   e_pal_ret = pal_ftell (px_algo->hl_file_hdl, &(px_algo->ui_cur_file_off));
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (0 == px_algo->ui_cur_file_off))
   {
      RFSVP_S_LOG_LOW( "pal_ftell failed: %d, %d", e_pal_ret,
         px_algo->ui_cur_file_off);
      px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ;
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ;
      px_algo->e_cur_req_rsp_code = eNODE_MSG_CMD_RSP_CODE_ABORTED;
      e_error = eRFSVP_RET_SUCCESS;
      goto CLEAN_RETURN;
   }

   (void) pal_memset (px_algo->ba_commit_rsp, 0x00,
      sizeof(px_algo->ba_commit_rsp));
   for (ui_i = 0; ui_i < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes; ui_i++)
   {
      if (true == px_rfsvp_server_ctxt->x_algo.ba_reachable[ui_i])
      {
         if (ui_i == px_rfsvp_server_ctxt->x_init_params.ui_node_index)
         {
            continue;
         }
         e_error = rfsvp_server_node_send_commit_req (px_rfsvp_server_ctxt,
            ui_i);
         if (eRFSVP_RET_SUCCESS != e_error)
         {
            RFSVP_S_LOG_LOW( "rfsvp_server_node_send_commit_req failed: %d",
               e_error);
            break;
         }
      }
      else
      {
         RFSVP_S_LOG_LOW( "Not sending to unreachable node: %d", ui_i);
         e_error = eRFSVP_RET_SUCCESS;
      }
   }
   if (ui_i == px_rfsvp_server_ctxt->x_init_params.ui_no_nodes)
   {
      px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ;
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ_DONE");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ_DONE;
      px_algo->e_cur_req_rsp_code = eNODE_MSG_CMD_RSP_CODE_SUCCESS;
      e_error = eRFSVP_RET_SUCCESS;
   }
   else
   {
      px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ;
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ;
      px_algo->e_cur_req_rsp_code = eNODE_MSG_CMD_RSP_CODE_ABORTED;
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_server_algo_honor_cur_node_up_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_req_msg_hdr)
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

   if (NULL == px_algo->px_cur_req_msg_hdr)
   {
      RFSVP_S_LOG_LOW("Invalid px_algo->px_cur_req_msg_hdr");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }
   px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ;
   RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ");
   px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ;
   px_algo->e_cur_req_rsp_code = eNODE_MSG_CMD_RSP_CODE_SUCCESS;
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_server_algo_honor_cur_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   NODE_MSG_APP_CMD_X *px_app_cmd = NULL;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   if (NULL == px_algo->px_cur_req_msg_hdr)
   {
      RFSVP_S_LOG_LOW("Invalid px_algo->px_cur_req_msg_hdr");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_app_cmd = (NODE_MSG_APP_CMD_X *) px_algo->px_cur_req_msg_hdr;

   /*
    * 3. If the request is READ, sends the reply with last line of the file.
    *
    * 4. If the request is WRITE:
    *    a. Update <VN[i], RU[i], DS[i]> by calling Do-Update procedure.
    *    b. Append the following to the local copy of file:
    *       <VN[i], RU[i], DS[i], text>.
    *    c. Send COMMIT request with <VN[i], RU[i], DS[i], text> to all nodes
    *       in P[i] besides itself.
    */

   switch (px_app_cmd->e_cmd)
   {
      case eRFSVP_SERVER_CMD_READ:
      {
         RFSVP_S_LOG_MED("Honoring eRFSVP_SERVER_CMD_READ");
         e_error = rfsvp_server_algo_honor_cur_read_req (px_rfsvp_server_ctxt,
            px_algo->px_cur_req_msg_hdr);
         break;
      }
      case eRFSVP_SERVER_CMD_WRITE:
      {
         RFSVP_S_LOG_MED("Honoring eRFSVP_SERVER_CMD_WRITE");
         e_error = rfsvp_server_algo_honor_cur_write_req (px_rfsvp_server_ctxt,
            px_algo->px_cur_req_msg_hdr);
         break;
      }
      case eRFSVP_SERVER_CMD_NODE_UP:
      {
         RFSVP_S_LOG_MED("Honoring eRFSVP_SERVER_CMD_NODE_UP");
         e_error = rfsvp_server_algo_honor_cur_node_up_req (px_rfsvp_server_ctxt,
            px_algo->px_cur_req_msg_hdr);
         break;
      }
      default:
      {
         RFSVP_S_LOG_MED("Fatal Error!!! Invalid Request");
         px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ;
         RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ");
         px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ;
         e_error = eRFSVP_RET_INVALID_ARGS;
         break;
      }
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_algo_sub_state_commit_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   bool b_is_distinguished = false;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   /*
    * When one of the seven nodes, S[i] , receives a READ/WRITE request:
    * 1. S[i] sends a VOTE-REQUEST message to all reachable nodes (P[i]).
    *    a. Upon receiving a VOTE-REQUEST from node S[i], S[j] sends
    *       <VN[j], RU[j], DS[j]> to S.
    *
    * 2. Upon receiving all responses, S[i] checks if it belongs to the
    *    distinguished partition by calling Is-Distinguished procedure.
    *    a. If YES: If S[i] does not have the latest copy (based on VNs),
    *       then obtain the latest copy from a node.
    *    b. If NO: ABORT the request.
    *
    * 3. If the request is READ, sends the reply with last line of the file.
    *
    * 4. If the request is WRITE:
    *    a. Update <VN[i], RU[i], DS[i]> by calling Do-Update procedure.
    *    b. Append the following to the local copy of file:
    *       <VN[i], RU[i], DS[i], text>.
    *    c. Send COMMIT request with <VN[i], RU[i], DS[i], text> to all nodes
    *       in P[i] besides itself.
    *
    * 5. Upon receiving a COMMIT request from S[j], append the following to
    *    the local copy of file: <VN[j], RU[j], DS[j], text> and perform the
    *    corresponding updates to the local values of VN, RU and DS.
    */

   b_is_distinguished = rfsvp_server_node_is_distinguished (
      px_rfsvp_server_ctxt);
   if (true == b_is_distinguished)
   {
      RFSVP_S_LOG_MED("The current partition is a distinguished one!");
      if (px_algo->x_vp.ui_vn >= px_algo->ui_cur_max_vn)
      {
         /*
          * My version number is greater than or equal to the current max
          * version number. This means my copy of the file is an up-to-date
          * one.
          */
         RFSVP_S_LOG_MED(
            "My version number %d == the latest version number: %d",
            px_algo->x_vp.ui_vn, px_algo->ui_cur_max_vn);

         /*
          * Process the client/manager request now.
          */
         e_error = rfsvp_server_algo_honor_cur_req (px_rfsvp_server_ctxt);
         if (eRFSVP_RET_SUCCESS != e_error)
         {
            RFSVP_S_LOG_LOW( "rfsvp_server_algo_honor_cur_req failed: %d",
               e_error);
         }
      }
      else
      {
         /*
          * Synchronize with one of the servers with the latest VN.
          */
         RFSVP_S_LOG_MED("My version number %d < the latest version number: %d",
            px_algo->x_vp.ui_vn, px_algo->ui_cur_max_vn);
         RFSVP_S_LOG_MED("Need to synchronize with the latest");
         px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ;
         RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_SYNC_REQ");
         px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_SYNC_REQ;
         e_error = eRFSVP_RET_SUCCESS;
      }
   }
   else
   {
      RFSVP_S_LOG_MED("The current partition is not a distinguished one!");
      px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ;
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ;
      px_algo->e_cur_req_rsp_code = eNODE_MSG_CMD_RSP_CODE_ABORTED;
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_algo_sub_state_sync_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   uint32_t ui_i = 0;
   RFSVP_SERVER_ALGO_VP_X *px_vp = NULL;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);
   for (ui_i = 0; ui_i < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes; ui_i++)
   {
      if (true == px_algo->ba_reachable[ui_i])
      {
         px_vp = &(px_algo->xa_nodex_vp[ui_i]);

         if (px_algo->ui_cur_max_vn == px_vp->ui_vn)
         {
            break;
         }
      }
   }

   if (ui_i == px_rfsvp_server_ctxt->x_init_params.ui_no_nodes)
   {
      RFSVP_S_LOG_MED("Fatal Error! Node with Max VN Not found!");
      px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_SYNC_REQ;
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ;
      e_error = eRFSVP_RET_SUCCESS;
      goto CLEAN_RETURN;
   }

   RFSVP_S_LOG_MED("Trying to synchronize with Node %d", ui_i);
   e_error = rfsvp_server_node_send_sync_req (px_rfsvp_server_ctxt, ui_i);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW( "rfsvp_server_node_send_sync_req failed: %d", e_error);
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_algo_sub_state_sync_progress (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   uint32_t ui_i = 0;
   RFSVP_SERVER_ALGO_VP_X *px_vp = NULL;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);
   for (ui_i = 0; ui_i < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes; ui_i++)
   {
      if (true == px_algo->ba_reachable[ui_i])
      {
         px_vp = &(px_algo->xa_nodex_vp[ui_i]);

         if (px_algo->ui_cur_max_vn == px_vp->ui_vn)
         {
            break;
         }
      }
   }

   if (ui_i == px_rfsvp_server_ctxt->x_init_params.ui_no_nodes)
   {
      RFSVP_S_LOG_MED("Fatal Error! Node with Max VN Not found!");
      px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_SYNC_REQ;
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ;
      e_error = eRFSVP_RET_SUCCESS;
      goto CLEAN_RETURN;
   }

   RFSVP_S_LOG_MED("Trying to synchronize with Node %d", ui_i);
   e_error = rfsvp_server_node_send_sync_req (px_rfsvp_server_ctxt, ui_i);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW( "rfsvp_server_node_send_sync_req failed: %d", e_error);
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_algo_sub_state_commit_req_done (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_algo_sub_state_abort_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   uint32_t ui_i = 0;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   (void) pal_memset (px_algo->ba_abort_rsp, 0x00,
      sizeof(px_algo->ba_abort_rsp));

   for (ui_i = 0; ui_i < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes; ui_i++)
   {
      if (true == px_rfsvp_server_ctxt->x_algo.ba_reachable[ui_i])
      {
         e_error = rfsvp_server_node_send_abort_req (px_rfsvp_server_ctxt,
            ui_i);
         if (eRFSVP_RET_SUCCESS != e_error)
         {
            RFSVP_S_LOG_LOW( "rfsvp_server_node_send_abort_req failed: %d",
               e_error);
            break;
         }
      }
      else
      {
         RFSVP_S_LOG_LOW( "Not sending to unreachable node: %d", ui_i);
         e_error = eRFSVP_RET_SUCCESS;
      }
   }
   if (ui_i == px_rfsvp_server_ctxt->x_init_params.ui_no_nodes)
   {
      px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ;
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ_DONE");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ_DONE;
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_algo_sub_state_abort_req_done (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }
   RFSVP_S_LOG_MED("Waiting for eNODE_MSG_ID_ALGO_MSG_ABORT_RSP");
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_algo_sub_state_complete (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   NODE_MSG_APP_CMD_X *px_app_cmd = NULL;
   NODE_MSG_APP_CMD_RSP_X *px_rsp = NULL;
   uint32_t ui_alloc_size = 0;
   uint8_t *puc_data_off = 0;
   uint32_t ui_str_len = 0;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   px_app_cmd = (NODE_MSG_APP_CMD_X *) px_algo->px_cur_req_msg_hdr;
   switch (px_app_cmd->e_cmd)
   {
      case eRFSVP_SERVER_CMD_READ:
      {
         RFSVP_S_LOG_MED("Responding to eRFSVP_SERVER_CMD_READ");
         ui_str_len = pal_strnlen (px_algo->uca_read_data,
            sizeof(px_algo->uca_read_data)) + 1;
         ui_alloc_size = sizeof(*px_rsp) + ui_str_len;
         break;
      }
      case eRFSVP_SERVER_CMD_WRITE:
      {
         RFSVP_S_LOG_MED("Responding to eRFSVP_SERVER_CMD_WRITE");
         ui_alloc_size = sizeof(*px_rsp);
         break;
      }
      case eRFSVP_SERVER_CMD_NODE_UP:
      {
         RFSVP_S_LOG_MED("Responding to eRFSVP_SERVER_CMD_NODE_UP");
         ui_alloc_size = sizeof(*px_rsp);
         break;
      }
      default:
      {
         RFSVP_S_LOG_MED("Fatal Error!!! Invalid Request");
         e_error = eRFSVP_RET_INVALID_ARGS;
         ui_alloc_size = sizeof(*px_rsp);
         break;
      }
   }

   px_rsp = pal_malloc (ui_alloc_size, NULL);
   if (NULL == px_rsp)
   {
      px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_COMPLETE;
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_STATE_IDLE");
      px_algo->e_state = eRFSVP_SERVER_ALGO_STATE_IDLE;
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }

   if ((eRFSVP_SERVER_CMD_READ == px_app_cmd->e_cmd)
      && (eNODE_MSG_CMD_RSP_CODE_SUCCESS == px_algo->e_cur_req_rsp_code))
   {
      puc_data_off = (uint8_t *) &(px_rsp->ui_data_len);
      puc_data_off += sizeof(px_rsp->ui_data_len);

      (void) pal_memmove (puc_data_off, px_algo->uca_read_data, ui_str_len);

      RFSVP_S_LOG_LOW("Sending Read Response: %s", puc_data_off);

      (void) pal_memset (px_algo->uca_read_data, 0x00,
         sizeof(px_algo->uca_read_data));
   }

   px_rsp->e_cmd = px_app_cmd->e_cmd;
   px_rsp->x_hdr.ui_msg_id = eNODE_MSG_ID_APP_MSG_CMD_RSP;
   px_rsp->x_hdr.ui_msg_pay_len = ui_alloc_size - sizeof(px_rsp->x_hdr);
   px_rsp->ui_server_idx = px_rfsvp_server_ctxt->x_init_params.ui_node_index;
   px_rsp->e_rsp_code = px_algo->e_cur_req_rsp_code;
   e_error = rfsvp_server_node_send_app_msg_cmd_rsp_to_mgr (
      px_rfsvp_server_ctxt, px_rsp);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW(
         "rfsvp_server_node_send_app_msg_cmd_rsp_to_mgr failed: %d", e_error);
   }

   if (NULL != px_algo->px_cur_req_msg_hdr)
   {
      pal_free (px_algo->px_cur_req_msg_hdr);
      px_algo->px_cur_req_msg_hdr = NULL;
   }

   px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_COMPLETE;
   RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_STATE_IDLE");
   px_algo->e_state = eRFSVP_SERVER_ALGO_STATE_IDLE;
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   if (NULL != px_rsp)
   {
      pal_free (px_rsp);
      px_rsp = NULL;
   }
   return e_error;
}
