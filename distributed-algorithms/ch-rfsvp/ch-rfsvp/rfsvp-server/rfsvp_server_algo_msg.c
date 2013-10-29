/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   rfsvp_server_algo_msg.c
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

/****************************** LOCAL FUNCTIONS *******************************/
RFSVP_RET_E rfsvp_server_node_handle_algo_msg_is_reachable (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_ALGO_IS_REACHABLE_X *px_reachable = NULL;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW ("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_reachable = (NODE_MSG_ALGO_IS_REACHABLE_X *) px_msg_header;

   e_error = rfsvp_server_node_send_is_reachable_rsp_to_node (
      px_rfsvp_server_ctxt, px_reachable->ui_server_idx);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW(
         "rfsvp_server_node_send_is_reachable_rsp_to_node failed: %d", e_error);
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_algo_msg_is_reachable_rsp (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   NODE_MSG_ALGO_IS_REACHABLE_RSP_X *px_reachable_rsp = NULL;
   uint32_t ui_i = 0;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW ("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_reachable_rsp = (NODE_MSG_ALGO_IS_REACHABLE_RSP_X *) px_msg_header;
   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   if (!(eRFSVP_SERVER_ALGO_SUB_STATE_REACH_WAITING & px_algo->ui_sub_state))
   {
      RFSVP_S_LOG_MED("Reachability check is already complete. Discarding this "
         "response");
      e_error = eRFSVP_RET_SUCCESS;
      goto CLEAN_RETURN;
   }

   px_algo->ba_reachable[px_reachable_rsp->ui_server_idx] = true;
   RFSVP_S_LOG_MED("Node %d is reachable", px_reachable_rsp->ui_server_idx);

   for (ui_i = 0; ui_i < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes;
         ui_i++)
   {
      if (false == px_algo->ba_reachable[ui_i])
      {
         break;
      }
   }
   if (ui_i == px_rfsvp_server_ctxt->x_init_params.ui_no_nodes)
   {
      RFSVP_S_LOG_MED("All nodes are reachable");
      px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_REACH_WAITING;
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_REACH_REQ_DONE");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_REACH_REQ_DONE;
   }
   else
   {
      RFSVP_S_LOG_MED("%d nodes are reachable", ui_i);
   }
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_algo_msg_vote_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_ALGO_VOTE_REQ_X *px_vote_req = NULL;
#if 0
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
#endif
   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW ("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }
#if 0
   px_algo = &(px_rfsvp_server_ctxt->x_algo);
#endif
   px_vote_req = (NODE_MSG_ALGO_VOTE_REQ_X *) px_msg_header;

   switch (px_vote_req->e_purpose)
   {
      case eNODE_VOTE_REQ_PURPOSE_VOTING:
      {
         RFSVP_S_LOG_MED("Vote Req Purpose eNODE_VOTE_REQ_PURPOSE_VOTING");
#if 0
         RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_STATE_IN_PROGRESS");
         px_algo->e_state = eRFSVP_SERVER_ALGO_STATE_IN_PROGRESS;
#endif
         break;
      }
      case eNODE_VOTE_REQ_PURPOSE_QUERY:
      {
         RFSVP_S_LOG_MED("Vote Req Purpose eNODE_VOTE_REQ_PURPOSE_QUERY");
         break;
      }
      default:
      {
         break;
      }
   }

   e_error = rfsvp_server_node_send_vote_rsp (px_rfsvp_server_ctxt,
      px_vote_req->ui_server_idx);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW( "rfsvp_server_node_send_vote_rsp failed: %d", e_error);
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_algo_msg_vote_rsp (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_ALGO_VOTE_RSP_X *px_vote_rsp = NULL;
   uint32_t ui_i = 0;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW ("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_vote_rsp = (NODE_MSG_ALGO_VOTE_RSP_X *) px_msg_header;
   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   RFSVP_S_LOG_MED("Vote response from node %d: %d, %d, %d, %d",
      px_vote_rsp->ui_server_idx, px_vote_rsp->x_vp.ui_vn,
      px_vote_rsp->x_vp.ui_ru, px_vote_rsp->x_vp.ui_ds_count,
      px_vote_rsp->x_vp.ui_cur_file_off);

   (void) pal_memmove(
      &(px_algo->xa_nodex_vp[px_vote_rsp->ui_server_idx]),
      &(px_vote_rsp->x_vp),
      sizeof(px_algo->xa_nodex_vp[px_vote_rsp->ui_server_idx]));

   px_algo->ba_vote_rsp[px_vote_rsp->ui_server_idx] = true;

   for (ui_i = 0; ui_i < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes; ui_i++)
   {
      if (true == px_algo->ba_reachable[ui_i])
      {
         if (false == px_algo->ba_vote_rsp[ui_i])
         {
            break;
         }
      }
   }

   if (ui_i == px_rfsvp_server_ctxt->x_init_params.ui_no_nodes)
   {
      RFSVP_S_LOG_MED("All nodes have responded to VOTE REQUEST");

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

      rfsvp_server_node_print_voting_table (px_rfsvp_server_ctxt);
      px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_VOTE_REQ_DONE;
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ;
   }
   else
   {
      RFSVP_S_LOG_MED("%d nodes have responded to VOTE REQUEST", ui_i);
   }

   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_algo_msg_commit_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   NODE_MSG_ALGO_COMMIT_REQ_X *px_commit_req = NULL;
   uint8_t *puc_data_off = 0;
   uint32_t ui_data_len = 0;
   uint32_t ui_temp = 0;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   RFSVP_SERVER_ALGO_VP_X *px_vp = NULL;
   uint32_t ui_i = 0;
   uint8_t uca_write_buf[RFSVP_SERVER_MAX_FILE_WRITE_BUF] = {0};
   uint8_t uca_ds_buf[RFSVP_SERVER_MAX_FILE_WRITE_BUF] = {0};
   uint32_t ui_cur_ds_buf_len = 0;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW ("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);
   px_commit_req = (NODE_MSG_ALGO_COMMIT_REQ_X *) px_msg_header;
   px_vp = &(px_algo->x_vp);

   px_algo->x_vp.ui_vn = px_commit_req->x_updated_vp.ui_vn;
   px_algo->x_vp.ui_ru = px_commit_req->x_updated_vp.ui_ru;
   px_algo->x_vp.ui_ds_count = px_commit_req->x_updated_vp.ui_ds_count;

   RFSVP_S_LOG_MED("Commit Request: %d, %d, %d",
      px_algo->x_vp.ui_vn, px_algo->x_vp.ui_ru, px_algo->x_vp.ui_ds_count);
   (void) pal_memset (px_algo->x_vp.uia_ds, 0x00,
      sizeof(px_algo->x_vp.uia_ds));
   for (ui_i = 0; ui_i < px_algo->x_vp.ui_ds_count; ui_i++)
   {
      px_algo->x_vp.uia_ds[ui_i] = px_commit_req->x_updated_vp.uia_ds[ui_i];
   }

   for (ui_i = 0; ui_i < px_vp->ui_ds_count; ui_i++)
   {
      ui_cur_ds_buf_len = pal_strnlen (uca_ds_buf, sizeof(uca_ds_buf));
      snprintf (((char *) uca_ds_buf) + ui_cur_ds_buf_len,
         (sizeof(uca_ds_buf) - ui_cur_ds_buf_len), "%d, ", px_vp->uia_ds[ui_i]);
   }

   puc_data_off = (uint8_t *) &(px_commit_req->ui_data_len);
   puc_data_off += sizeof(px_commit_req->ui_data_len);

   snprintf ((char *) uca_write_buf, sizeof(uca_write_buf),
      "%4d, %2d, {%24s}, %s\n",
      px_vp->ui_vn, px_vp->ui_ru, uca_ds_buf, puc_data_off);

   ui_data_len = pal_strnlen (uca_write_buf, sizeof(uca_write_buf)); // px_commit_req->ui_data_len;
   ui_temp = ui_data_len;
   e_pal_ret = pal_fwrite (px_algo->hl_file_hdl, uca_write_buf, &ui_data_len);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (ui_data_len != ui_temp))
   {
      RFSVP_S_LOG_LOW( "pal_fwrite failed: %d", e_pal_ret);
      goto CLEAN_RETURN;
   }

   px_algo->ui_last_line_off = px_algo->ui_cur_file_off;

   e_pal_ret = pal_ftell (px_algo->hl_file_hdl, &(px_algo->ui_cur_file_off));
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (0 == px_algo->ui_cur_file_off))
   {
      RFSVP_S_LOG_LOW( "pal_ftell failed: %d, %d", e_pal_ret,
         px_algo->ui_cur_file_off);
      goto CLEAN_RETURN;
   }

   RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_STATE_IDLE");
   px_algo->e_state = eRFSVP_SERVER_ALGO_STATE_IDLE;
   e_error = rfsvp_server_node_send_commit_rsp (px_rfsvp_server_ctxt,
      px_commit_req->ui_server_idx);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW( "rfsvp_server_node_send_commit_rsp failed: %d",
         e_error);
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_algo_msg_commit_rsp (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_ALGO_COMMIT_RSP_X *px_commit_rsp = NULL;
   uint32_t ui_i = 0;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW ("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_commit_rsp = (NODE_MSG_ALGO_COMMIT_RSP_X *) px_msg_header;
   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   RFSVP_S_LOG_MED("Commit response from node %d", px_commit_rsp->ui_server_idx);

   px_algo->ba_commit_rsp [px_commit_rsp->ui_server_idx] = true;
   for (ui_i = 0; ui_i < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes;
         ui_i++)
   {
      if (true == px_algo->ba_reachable [ui_i])
      {
         if (ui_i == px_rfsvp_server_ctxt->x_init_params.ui_node_index)
         {
            continue;
         }
         if (false == px_algo->ba_commit_rsp [ui_i])
         {
            break;
         }
      }
   }

   if (ui_i == px_rfsvp_server_ctxt->x_init_params.ui_no_nodes)
   {
      RFSVP_S_LOG_MED("All nodes have responded to COMMIT REQUEST");
      px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ_DONE;
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_COMPLETE");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_COMPLETE;
   }
   else
   {
      RFSVP_S_LOG_MED("%d nodes have responded to COMMIT REQUEST", ui_i);
   }
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_algo_msg_sync_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   NODE_MSG_ALGO_SYNC_REQ_X *px_sync_req = NULL;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   uint8_t uca_write_buf[RFSVP_SERVER_MAX_FILE_WRITE_BUF] = {0};
   uint32_t ui_line_len = 0;
   NODE_MSG_ALGO_SYNC_RSP_X *px_sync_rsp = NULL;
   uint32_t ui_alloc_size;
   uint8_t *puc_data_off = NULL;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW ("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   px_sync_req = (NODE_MSG_ALGO_SYNC_REQ_X *) px_msg_header;

   RFSVP_S_LOG_MED("Sync request from file offset: %d. My current offset: %d",
      px_sync_req->ui_file_offset, px_algo->ui_cur_file_off);

   e_pal_ret = pal_fseek(px_algo->hl_file_hdl, px_sync_req->ui_file_offset,
      ePAL_FILE_WHENCE_SEEK_SET);
   if (ePAL_RET_SUCCESS != e_pal_ret)
   {
      RFSVP_S_LOG_LOW("pal_fseek failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   e_pal_ret = pal_freadline (px_algo->hl_file_hdl, uca_write_buf,
      sizeof(uca_write_buf), &ui_line_len);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (0 == ui_line_len))
   {
      RFSVP_S_LOG_LOW( "pal_freadline failed: %d, %d", e_pal_ret, ui_line_len);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
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



   ui_alloc_size = sizeof(*px_sync_rsp) + ui_line_len;
   px_sync_rsp = pal_malloc (ui_alloc_size, NULL);
   if (ePAL_RET_SUCCESS != e_pal_ret)
   {
      RFSVP_S_LOG_LOW("pal_malloc failed");
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   px_sync_rsp->x_hdr.ui_msg_id = eNODE_MSG_ID_ALGO_MSG_SYNC_RSP;
   px_sync_rsp->x_hdr.ui_msg_pay_len = ui_alloc_size
      - sizeof(px_sync_rsp->x_hdr);

   RFSVP_S_LOG_LOW("Sending line: \"%s\", Length: %d, Header: %d",
      uca_write_buf, ui_line_len, px_sync_rsp->x_hdr.ui_msg_pay_len);

   px_sync_rsp->ui_server_idx =
      px_rfsvp_server_ctxt->x_init_params.ui_node_index;

   px_sync_rsp->ui_final_offset = px_algo->ui_cur_file_off;
   px_sync_rsp->ui_sync_vn = px_algo->x_vp.ui_vn;
   px_sync_rsp->ui_data_len = ui_line_len;

   puc_data_off = (uint8_t *) &(px_sync_rsp->ui_data_len);
   puc_data_off += sizeof(px_sync_rsp->ui_data_len);

   (void) pal_memmove(puc_data_off, uca_write_buf, ui_line_len);

   e_error = rfsvp_server_node_send_sync_rsp (px_rfsvp_server_ctxt,
      px_sync_req->ui_server_idx, px_sync_rsp);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW( "rfsvp_server_node_send_sync_rsp failed: %d", e_error);
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_algo_msg_sync_rsp (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_ALGO_SYNC_RSP_X *px_sync_rsp = NULL;
   uint8_t *puc_data_off = NULL;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   uint32_t ui_data_len = 0;
   uint32_t ui_temp = 0;
   uint8_t uca_write_buf[RFSVP_SERVER_MAX_FILE_WRITE_BUF] = {0};

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW ("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);
   px_sync_rsp = (NODE_MSG_ALGO_SYNC_RSP_X *) px_msg_header;

   puc_data_off = (uint8_t *) &(px_sync_rsp->ui_data_len);
   puc_data_off += sizeof(px_sync_rsp->ui_data_len);

   RFSVP_S_LOG_LOW("Got line: \"%s\", Length: %d, Header: %d",
      puc_data_off, px_sync_rsp->ui_data_len, px_msg_header->ui_msg_pay_len);

   snprintf ((char *) uca_write_buf, sizeof(uca_write_buf), "%s\n",
      puc_data_off);

   ui_data_len = pal_strnlen (uca_write_buf, sizeof(uca_write_buf));
   ui_temp = ui_data_len;
   e_pal_ret = pal_fwrite (px_algo->hl_file_hdl, uca_write_buf, &ui_data_len);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (ui_data_len != ui_temp))
   {
      RFSVP_S_LOG_LOW( "pal_fwrite failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   px_algo->ui_last_line_off = px_algo->ui_cur_file_off;

   e_pal_ret = pal_ftell (px_algo->hl_file_hdl, &(px_algo->ui_cur_file_off));
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (0 == px_algo->ui_cur_file_off))
   {
      RFSVP_S_LOG_LOW( "pal_ftell failed: %d, %d", e_pal_ret,
         px_algo->ui_cur_file_off);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      if (px_algo->ui_cur_file_off == px_sync_rsp->ui_final_offset)
      {
         RFSVP_S_LOG_LOW("Synchronization complete!");
         px_algo->x_vp.ui_vn = px_sync_rsp->ui_sync_vn;

         px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_SYNC_REQ;
         px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_SYNC_PROGRESS;
         RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ");
         px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_COMMIT_REQ;
      }
      else
      {
         RFSVP_S_LOG_LOW("Synchronization incomplete. Need %d more bytes",
            (px_sync_rsp->ui_final_offset - px_algo->ui_cur_file_off));
         px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_SYNC_REQ;
         RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_SYNC_PROGRESS");
         px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_SYNC_PROGRESS;
      }
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_algo_msg_sync_update (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_ALGO_SYNC_UPDATE_X *px_sync_update = NULL;
   uint8_t *puc_data_off = NULL;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   uint32_t ui_data_len = 0;
   uint32_t ui_temp = 0;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW ("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);
   px_sync_update = (NODE_MSG_ALGO_SYNC_UPDATE_X *) px_msg_header;

   puc_data_off = (uint8_t *) &(px_sync_update->ui_data_len);
   puc_data_off += sizeof(px_sync_update->ui_data_len);

   RFSVP_S_LOG_LOW("Got data: Length: %d, Header: %d",
      px_sync_update->ui_data_len, px_msg_header->ui_msg_pay_len);

   ui_data_len = px_sync_update->ui_data_len;
   ui_temp = ui_data_len;
   e_pal_ret = pal_fwrite (px_algo->hl_file_hdl, puc_data_off, &ui_data_len);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (ui_data_len != ui_temp))
   {
      RFSVP_S_LOG_LOW( "pal_fwrite failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   px_algo->ui_last_line_off = px_algo->ui_cur_file_off;

   e_pal_ret = pal_ftell (px_algo->hl_file_hdl, &(px_algo->ui_cur_file_off));
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (0 == px_algo->ui_cur_file_off))
   {
      RFSVP_S_LOG_LOW( "pal_ftell failed: %d, %d", e_pal_ret,
         px_algo->ui_cur_file_off);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_algo_msg_abort_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_ALGO_ABORT_REQ_X *px_abort_req = NULL;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW ("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);
   px_abort_req = (NODE_MSG_ALGO_ABORT_REQ_X *) px_msg_header;

   RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_STATE_IDLE");
   px_algo->e_state = eRFSVP_SERVER_ALGO_STATE_IDLE;

   e_error = rfsvp_server_node_send_abort_rsp (px_rfsvp_server_ctxt,
      px_abort_req->ui_server_idx);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW( "rfsvp_server_node_send_abort_rsp failed: %d", e_error);
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_algo_msg_abort_rsp (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_ALGO_ABORT_RSP_X *px_abort_rsp = NULL;
   uint32_t ui_i = 0;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW ("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_abort_rsp = (NODE_MSG_ALGO_ABORT_RSP_X *) px_msg_header;
   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   RFSVP_S_LOG_MED("Abort response from node %d", px_abort_rsp->ui_server_idx);

   px_algo->ba_abort_rsp [px_abort_rsp->ui_server_idx] = true;
   for (ui_i = 0; ui_i < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes;
         ui_i++)
   {
      if (true == px_algo->ba_reachable [ui_i])
      {
         if (false == px_algo->ba_abort_rsp [ui_i])
         {
            break;
         }
      }
   }

   if (ui_i == px_rfsvp_server_ctxt->x_init_params.ui_no_nodes)
   {
      RFSVP_S_LOG_MED("All nodes have responded to ABORT REQUEST");
      px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_ABORT_REQ_DONE;
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_COMPLETE");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_COMPLETE;
   }
   else
   {
      RFSVP_S_LOG_MED("%d nodes have responded to ABORT REQUEST", ui_i);
   }
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_algo_msg_lock_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_ALGO_LOCK_REQ_X *px_lock_req = NULL;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   MSGQ_DATA_X x_data = {NULL};
   MSGQ_RET_E e_msgq_ret = eMSGQ_RET_FAILURE;
   NODE_MSG_HDR_X *px_msg_temp = NULL;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW ("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);
   px_lock_req = (NODE_MSG_ALGO_LOCK_REQ_X *) px_msg_header;

   px_algo->ui_highest_seq_no = (
         (px_lock_req->ui_seq_no > px_algo->ui_seq_no) ?
               (px_lock_req->ui_seq_no) : (px_algo->ui_seq_no));
   RFSVP_S_LOG_MED( "Lock Req : %d, seq: %d",
      px_lock_req->ui_server_idx, px_lock_req->ui_seq_no);
   if ((eRFSVP_SERVER_ALGO_STATE_IN_PROGRESS == px_algo->e_state)
      && ((px_lock_req->ui_seq_no > px_algo->ui_seq_no)
         || ((px_lock_req->ui_seq_no > px_algo->ui_seq_no)
            && (px_lock_req->ui_server_idx
               > px_rfsvp_server_ctxt->x_init_params.ui_node_index))))
   {
      RFSVP_S_LOG_MED("Previous request already is getting processed. "
      "Will wait till it finishes.");

      px_msg_temp = pal_malloc (
         sizeof(*px_msg_header) + px_msg_header->ui_msg_pay_len, NULL );

      pal_memmove (px_msg_temp, px_msg_header,
         sizeof(*px_msg_header) + px_msg_header->ui_msg_pay_len);

      x_data.p_data = px_msg_temp;
      x_data.ui_data_size = sizeof(*px_msg_temp) + px_msg_temp->ui_msg_pay_len;
      e_msgq_ret = msgq_add_msg (px_algo->hl_algo_req_msgq_hdl, &x_data, 0);
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
      }
      else
      {
         RFSVP_S_LOG_MED(
            "Setting eRFSVP_SERVER_ALGO_WAIT_STATE_NEW_REQ_WAITING");
         px_algo->ui_wait_state |=
            eRFSVP_SERVER_ALGO_WAIT_STATE_NEW_REQ_WAITING;
         e_error = eRFSVP_RET_SUCCESS;
      }
   }
   else
   {
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_STATE_IN_PROGRESS");
      px_algo->e_state = eRFSVP_SERVER_ALGO_STATE_IN_PROGRESS;

      e_error = rfsvp_server_node_send_lock_rsp (px_rfsvp_server_ctxt,
         px_lock_req->ui_server_idx);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_S_LOG_LOW( "rfsvp_server_node_send_vote_rsp failed: %d",
            e_error);
      }
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_algo_msg_lock_rsp (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_ALGO_LOCK_RSP_X *px_lock_rsp = NULL;
   uint32_t ui_i = 0;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW ("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_lock_rsp = (NODE_MSG_ALGO_LOCK_RSP_X *) px_msg_header;
   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   RFSVP_S_LOG_MED("Lock response from node %d",
      px_lock_rsp->ui_server_idx);

   px_algo->ba_lock_rsp[px_lock_rsp->ui_server_idx] = true;

   for (ui_i = 0; ui_i < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes; ui_i++)
   {
      if (true == px_algo->ba_reachable[ui_i])
      {
         if (false == px_algo->ba_lock_rsp[ui_i])
         {
            break;
         }
      }
   }

   if (ui_i == px_rfsvp_server_ctxt->x_init_params.ui_no_nodes)
   {
      RFSVP_S_LOG_MED("All nodes have responded to LOCK REQUEST");

      if (true
         == px_algo->ba_reachable [px_rfsvp_server_ctxt->x_init_params.ui_node_index])
      {
         px_algo->ba_lock_rsp [px_rfsvp_server_ctxt->x_init_params.ui_node_index] =
            true;
      }

      px_algo->ui_sub_state &= ~eRFSVP_SERVER_ALGO_SUB_STATE_LOCK_REQ_DONE;
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_VOTE_REQ");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_VOTE_REQ;
   }
   else
   {
      RFSVP_S_LOG_MED("%d nodes have responded to LOCK REQUEST", ui_i);
   }
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}
