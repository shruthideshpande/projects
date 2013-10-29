/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   rfsvp_server_send_msg.c
 * \author sandeepprakash
 *
 * \date   07-Oct-2012
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
RFSVP_RET_E rfsvp_server_node_send_join_to_mgr (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   PAL_SOCK_HDL *phl_mgr_sock_hdl = NULL;
   NODE_MSG_JOIN_X x_join_cred = { { 0 } };
   uint32_t ui_send_size = 0;
   RFSVP_SERVER_NODE_CTXT_X *px_myself = NULL;
   uint32_t ui_my_node_idx = 0;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   phl_mgr_sock_hdl = &(px_rfsvp_server_ctxt->hl_mgr_sock_hdl);

   ui_my_node_idx = px_rfsvp_server_ctxt->x_init_params.ui_node_index;
   px_myself = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes[ui_my_node_idx]);

   RFSVP_S_LOG_LOW("Sending eNODE_MSG_ID_SETUP_JOIN to node mgr");
   x_join_cred.x_hdr.ui_msg_id = eNODE_MSG_ID_SETUP_JOIN;
   x_join_cred.x_hdr.ui_msg_pay_len = sizeof(x_join_cred)
      - sizeof(x_join_cred.x_hdr);

   x_join_cred.x_node_ctxt.ui_ip_addr_ho = px_myself->ui_ip_addr_ho;
   (void) pal_strncpy (x_join_cred.x_node_ctxt.uca_dns_name_str,
      px_myself->uca_dns_name_str,
      sizeof(x_join_cred.x_node_ctxt.uca_dns_name_str));
   x_join_cred.x_node_ctxt.ui_node_index = px_myself->ui_node_index;
   x_join_cred.x_node_ctxt.us_udp_port_ho = px_myself->us_udp_port_ho;
   ui_send_size = sizeof(x_join_cred);
   e_pal_ret = pal_sock_send_fixed (*phl_mgr_sock_hdl, (uint8_t *) &x_join_cred,
      &ui_send_size, 0, RFSVP_SERVER_SEND_MSG_TIMEOUT_MS);
   if (ePAL_RET_SUCCESS != e_pal_ret)
   {
      RFSVP_S_LOG_LOW("pal_sock_send_fixed to mgr failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      RFSVP_S_LOG_LOW("pal_sock_send_fixed to mgr success: %d bytes",
         ui_send_size);
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_send_establish_done_to_mgr (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   PAL_SOCK_HDL *phl_mgr_sock_hdl = NULL;
   uint32_t ui_send_size = 0;
   NODE_MSG_ESTABLISH_DONE_X x_est_done = {{0}};

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   phl_mgr_sock_hdl = &(px_rfsvp_server_ctxt->hl_mgr_sock_hdl);

   RFSVP_S_LOG_LOW("Sending eNODE_MSG_ID_SETUP_ESTABLISH_DONE to mgr");
   x_est_done.x_hdr.ui_msg_id = eNODE_MSG_ID_SETUP_ESTABLISH_DONE;
   x_est_done.x_hdr.ui_msg_pay_len = sizeof(x_est_done)
      - sizeof(x_est_done.x_hdr);

   ui_send_size = sizeof(x_est_done);
   e_pal_ret = pal_sock_send_fixed (*phl_mgr_sock_hdl, (uint8_t *) &x_est_done,
      &ui_send_size, 0, RFSVP_SERVER_SEND_MSG_TIMEOUT_MS);
   if (ePAL_RET_SUCCESS != e_pal_ret)
   {
      RFSVP_S_LOG_LOW("pal_sock_send_fixed to mgr failed: %d",
         e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      RFSVP_S_LOG_LOW("pal_sock_send_fixed to mgr success: %d bytes",
         ui_send_size);
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_send_app_msg_cmd_rsp_to_mgr (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_APP_CMD_RSP_X *px_cmd_rsp)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   PAL_SOCK_HDL *phl_mgr_sock_hdl = NULL;
   uint32_t ui_send_size = 0;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_cmd_rsp))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   phl_mgr_sock_hdl = &(px_rfsvp_server_ctxt->hl_mgr_sock_hdl);

   RFSVP_S_LOG_LOW("Sending eNODE_MSG_ID_APP_MSG_CMD_RSP to mgr");

   ui_send_size = sizeof(px_cmd_rsp->x_hdr) + px_cmd_rsp->x_hdr.ui_msg_pay_len;
   e_pal_ret = pal_sock_send_fixed (*phl_mgr_sock_hdl, (uint8_t *) px_cmd_rsp,
      &ui_send_size, 0, RFSVP_SERVER_SEND_MSG_TIMEOUT_MS);
   if (ePAL_RET_SUCCESS != e_pal_ret)
   {
      RFSVP_S_LOG_LOW("pal_sock_send_fixed to mgr failed: %d",
         e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      RFSVP_S_LOG_LOW("pal_sock_send_fixed to mgr success: %d bytes",
         ui_send_size);
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_send_udp_ping_to_all (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   uint32_t ui_index = 0;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   for (ui_index = 0;
         ui_index < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes; ui_index++)
   {
      if (ui_index == px_rfsvp_server_ctxt->x_init_params.ui_node_index)
      {
         continue;
      }
      e_error = rfsvp_server_node_send_udp_ping_to_node (px_rfsvp_server_ctxt,
         ui_index);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_S_LOG_HIGH(
            "rfsvp_server_node_send_ping_to_node to node %d failed: %d",
            ui_index, e_error);
         break;
      }
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_send_udp_ping_to_node (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t ui_index)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_send_size = 0;
   PAL_SOCK_ADDR_IN_X x_dest_sock_addr_in = {0};
   NODE_MSG_PING_X x_ping = {{0}};
   RFSVP_SERVER_NODE_CTXT_X *px_node_ctxt = NULL;

   if ((NULL == px_rfsvp_server_ctxt)
      || (NULL == px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_ping.x_hdr.ui_msg_id = eNODE_MSG_ID_SETUP_PING;
   x_ping.x_hdr.ui_msg_pay_len = sizeof(x_ping)
      - sizeof(x_ping.x_hdr);
   x_ping.ui_ping_local_ts_ms = pal_get_system_time ();
   x_ping.ui_node_idx = px_rfsvp_server_ctxt->x_init_params.ui_node_index;
   x_ping.ui_ping_seq_no = px_rfsvp_server_ctxt->ui_ping_seq_no;

   px_node_ctxt = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes[ui_index]);
   x_dest_sock_addr_in.us_sin_port_no = pal_htons (
      px_node_ctxt->us_udp_port_ho);
   x_dest_sock_addr_in.x_sin_addr.ui_ip_addr_no = pal_htonl (
      px_node_ctxt->ui_ip_addr_ho);
   ui_send_size = sizeof(x_ping);
   RFSVP_S_LOG_LOW("Generating UDP Ping to 0x%x:%d",
      px_node_ctxt->ui_ip_addr_ho, px_node_ctxt->us_udp_port_ho);
   e_pal_ret = pal_sendto (px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl,
      (uint8_t *) &x_ping, &ui_send_size, 0, &x_dest_sock_addr_in);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (sizeof(x_ping) != ui_send_size))
   {
      RFSVP_S_LOG_LOW("pal_sendto failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_send_is_reachable_to_all (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   uint32_t ui_index = 0;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   for (ui_index = 0;
         ui_index < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes; ui_index++)
   {
      if (ui_index == px_rfsvp_server_ctxt->x_init_params.ui_node_index)
      {
         e_error = eRFSVP_RET_SUCCESS;
         continue;
      }
      e_error = rfsvp_server_node_send_is_reachable_to_node (
         px_rfsvp_server_ctxt, ui_index);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_S_LOG_HIGH(
            "rfsvp_server_node_send_is_reachable_to_node to node %d failed: %d",
            ui_index, e_error);
         break;
      }
   }
   CLEAN_RETURN: return e_error;
}

RFSVP_RET_E rfsvp_server_node_send_is_reachable_to_node (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t ui_index)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_send_size = 0;
   PAL_SOCK_ADDR_IN_X x_dest_sock_addr_in = {0};
   NODE_MSG_ALGO_IS_REACHABLE_X x_reachable = {{0}};
   RFSVP_SERVER_NODE_CTXT_X *px_node_ctxt = NULL;

   if ((NULL == px_rfsvp_server_ctxt)
      || (NULL == px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_reachable.x_hdr.ui_msg_id = eNODE_MSG_ID_ALGO_MSG_IS_REACHABLE;
   x_reachable.x_hdr.ui_msg_pay_len = sizeof(x_reachable)
      - sizeof(x_reachable.x_hdr);
   x_reachable.x_hdr.ui_node_index =
         px_rfsvp_server_ctxt->x_init_params.ui_node_index;

   x_reachable.ui_server_idx =
      px_rfsvp_server_ctxt->x_init_params.ui_node_index;

   px_node_ctxt = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes[ui_index]);
   x_dest_sock_addr_in.us_sin_port_no = pal_htons (
      px_node_ctxt->us_udp_port_ho);
   x_dest_sock_addr_in.x_sin_addr.ui_ip_addr_no = pal_htonl (
      px_node_ctxt->ui_ip_addr_ho);
   ui_send_size = sizeof(x_reachable);
   RFSVP_S_LOG_LOW("Sending eNODE_MSG_ID_ALGO_MSG_IS_REACHABLE to 0x%x:%d",
      px_node_ctxt->ui_ip_addr_ho, px_node_ctxt->us_udp_port_ho);
   e_pal_ret = pal_sendto (px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl,
      (uint8_t *) &x_reachable, &ui_send_size, 0, &x_dest_sock_addr_in);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (sizeof(x_reachable) != ui_send_size))
   {
      RFSVP_S_LOG_LOW("pal_sendto failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_send_is_reachable_rsp_to_node (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t ui_index)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_send_size = 0;
   PAL_SOCK_ADDR_IN_X x_dest_sock_addr_in = {0};
   NODE_MSG_ALGO_IS_REACHABLE_RSP_X x_reachable_rsp = {{0}};
   RFSVP_SERVER_NODE_CTXT_X *px_node_ctxt = NULL;

   if ((NULL == px_rfsvp_server_ctxt)
      || (NULL == px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_reachable_rsp.x_hdr.ui_msg_id = eNODE_MSG_ID_ALGO_MSG_IS_REACHABLE_RSP;
   x_reachable_rsp.x_hdr.ui_msg_pay_len = sizeof(x_reachable_rsp)
      - sizeof(x_reachable_rsp.x_hdr);
   x_reachable_rsp.x_hdr.ui_node_index =
            px_rfsvp_server_ctxt->x_init_params.ui_node_index;
   x_reachable_rsp.ui_server_idx =
      px_rfsvp_server_ctxt->x_init_params.ui_node_index;

   px_node_ctxt = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes[ui_index]);
   x_dest_sock_addr_in.us_sin_port_no = pal_htons (
      px_node_ctxt->us_udp_port_ho);
   x_dest_sock_addr_in.x_sin_addr.ui_ip_addr_no = pal_htonl (
      px_node_ctxt->ui_ip_addr_ho);
   ui_send_size = sizeof(x_reachable_rsp);
   RFSVP_S_LOG_LOW("Sending eNODE_MSG_ID_ALGO_MSG_IS_REACHABLE_RSP to 0x%x:%d",
      px_node_ctxt->ui_ip_addr_ho, px_node_ctxt->us_udp_port_ho);
   e_pal_ret = pal_sendto (px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl,
      (uint8_t *) &x_reachable_rsp, &ui_send_size, 0, &x_dest_sock_addr_in);
   if ((ePAL_RET_SUCCESS != e_pal_ret)
      || (sizeof(x_reachable_rsp) != ui_send_size))
   {
      RFSVP_S_LOG_LOW("pal_sendto failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_send_vote_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t ui_index,
   NODE_VOTE_REQ_PURPOSE_E e_purpose)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_send_size = 0;
   PAL_SOCK_ADDR_IN_X x_dest_sock_addr_in = {0};
   NODE_MSG_ALGO_VOTE_REQ_X x_vote_req = {{0}};
   RFSVP_SERVER_NODE_CTXT_X *px_node_ctxt = NULL;

   if ((NULL == px_rfsvp_server_ctxt)
      || (NULL == px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_vote_req.x_hdr.ui_msg_id = eNODE_MSG_ID_ALGO_MSG_VOTE_REQ;
   x_vote_req.x_hdr.ui_msg_pay_len = sizeof(x_vote_req)
      - sizeof(x_vote_req.x_hdr);
   x_vote_req.x_hdr.ui_node_index =
               px_rfsvp_server_ctxt->x_init_params.ui_node_index;
   x_vote_req.ui_server_idx =
      px_rfsvp_server_ctxt->x_init_params.ui_node_index;

   x_vote_req.e_purpose = e_purpose;

   x_vote_req.ui_seq_no = px_rfsvp_server_ctxt->x_algo.ui_highest_seq_no + 1;

   px_node_ctxt = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes[ui_index]);
   x_dest_sock_addr_in.us_sin_port_no = pal_htons (
      px_node_ctxt->us_udp_port_ho);
   x_dest_sock_addr_in.x_sin_addr.ui_ip_addr_no = pal_htonl (
      px_node_ctxt->ui_ip_addr_ho);
   ui_send_size = sizeof(x_vote_req);
   RFSVP_S_LOG_LOW("Sending eNODE_MSG_ID_ALGO_MSG_VOTE_REQ to 0x%x:%d",
      px_node_ctxt->ui_ip_addr_ho, px_node_ctxt->us_udp_port_ho);
   e_pal_ret = pal_sendto (px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl,
      (uint8_t *) &x_vote_req, &ui_send_size, 0, &x_dest_sock_addr_in);
   if ((ePAL_RET_SUCCESS != e_pal_ret)
      || (sizeof(x_vote_req) != ui_send_size))
   {
      RFSVP_S_LOG_LOW("pal_sendto failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_send_vote_rsp (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t ui_index)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_send_size = 0;
   PAL_SOCK_ADDR_IN_X x_dest_sock_addr_in = {0};
   NODE_MSG_ALGO_VOTE_RSP_X x_vote_rsp = {{0}};
   RFSVP_SERVER_NODE_CTXT_X *px_node_ctxt = NULL;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   RFSVP_SERVER_ALGO_VP_X *px_vp = NULL;
   uint32_t ui_i = 0;

   if ((NULL == px_rfsvp_server_ctxt)
      || (NULL == px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);
   px_vp = &(px_algo->x_vp);

   x_vote_rsp.x_hdr.ui_msg_id = eNODE_MSG_ID_ALGO_MSG_VOTE_RSP;
   x_vote_rsp.x_hdr.ui_msg_pay_len = sizeof(x_vote_rsp)
      - sizeof(x_vote_rsp.x_hdr);
   x_vote_rsp.x_hdr.ui_node_index =
               px_rfsvp_server_ctxt->x_init_params.ui_node_index;
   x_vote_rsp.ui_server_idx =
      px_rfsvp_server_ctxt->x_init_params.ui_node_index;

   x_vote_rsp.x_vp.ui_vn = px_vp->ui_vn;
   x_vote_rsp.x_vp.ui_ru = px_vp->ui_ru;
   x_vote_rsp.x_vp.ui_ds_count = px_vp->ui_ds_count;
   for (ui_i = 0; ui_i < px_vp->ui_ds_count; ui_i++)
   {
      x_vote_rsp.x_vp.uia_ds[ui_i] = px_vp->uia_ds[ui_i];
   }
   x_vote_rsp.x_vp.ui_cur_file_off = px_algo->ui_cur_file_off;

   px_node_ctxt = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes[ui_index]);
   x_dest_sock_addr_in.us_sin_port_no = pal_htons (
      px_node_ctxt->us_udp_port_ho);
   x_dest_sock_addr_in.x_sin_addr.ui_ip_addr_no = pal_htonl (
      px_node_ctxt->ui_ip_addr_ho);
   ui_send_size = sizeof(x_vote_rsp);
   RFSVP_S_LOG_LOW("Sending eNODE_MSG_ID_ALGO_MSG_VOTE_RSP to 0x%x:%d",
      px_node_ctxt->ui_ip_addr_ho, px_node_ctxt->us_udp_port_ho);
   e_pal_ret = pal_sendto (px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl,
      (uint8_t *) &x_vote_rsp, &ui_send_size, 0, &x_dest_sock_addr_in);
   if ((ePAL_RET_SUCCESS != e_pal_ret)
      || (sizeof(x_vote_rsp) != ui_send_size))
   {
      RFSVP_S_LOG_LOW("pal_sendto failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_send_sync_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t ui_index)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_send_size = 0;
   PAL_SOCK_ADDR_IN_X x_dest_sock_addr_in = {0};
   NODE_MSG_ALGO_SYNC_REQ_X x_sync_req = {{0}};
   RFSVP_SERVER_NODE_CTXT_X *px_node_ctxt = NULL;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;

   if ((NULL == px_rfsvp_server_ctxt)
      || (NULL == px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   x_sync_req.x_hdr.ui_msg_id = eNODE_MSG_ID_ALGO_MSG_SYNC_REQ;
   x_sync_req.x_hdr.ui_msg_pay_len = sizeof(x_sync_req)
      - sizeof(x_sync_req.x_hdr);
   x_sync_req.x_hdr.ui_node_index =
               px_rfsvp_server_ctxt->x_init_params.ui_node_index;
   x_sync_req.ui_server_idx =
      px_rfsvp_server_ctxt->x_init_params.ui_node_index;

   x_sync_req.ui_file_offset = px_algo->ui_cur_file_off;
   RFSVP_S_LOG_MED("Sending sync request with current file offset: %d",
      x_sync_req.ui_file_offset);

   px_node_ctxt = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes[ui_index]);
   x_dest_sock_addr_in.us_sin_port_no = pal_htons (
      px_node_ctxt->us_udp_port_ho);
   x_dest_sock_addr_in.x_sin_addr.ui_ip_addr_no = pal_htonl (
      px_node_ctxt->ui_ip_addr_ho);
   ui_send_size = sizeof(x_sync_req);
   RFSVP_S_LOG_LOW("Sending eNODE_MSG_ID_ALGO_MSG_SYNC_REQ to 0x%x:%d",
      px_node_ctxt->ui_ip_addr_ho, px_node_ctxt->us_udp_port_ho);
   e_pal_ret = pal_sendto (px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl,
      (uint8_t *) &x_sync_req, &ui_send_size, 0, &x_dest_sock_addr_in);
   if ((ePAL_RET_SUCCESS != e_pal_ret)
      || (sizeof(x_sync_req) != ui_send_size))
   {
      RFSVP_S_LOG_LOW("pal_sendto failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_send_sync_rsp (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t ui_index,
   NODE_MSG_ALGO_SYNC_RSP_X *px_sync_rsp)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_send_size = 0;
   uint32_t ui_temp = 0;
   PAL_SOCK_ADDR_IN_X x_dest_sock_addr_in = {0};
   RFSVP_SERVER_NODE_CTXT_X *px_node_ctxt = NULL;

   if ((NULL == px_rfsvp_server_ctxt)
      || (NULL == px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_sync_rsp->x_hdr.ui_node_index =
         px_rfsvp_server_ctxt->x_init_params.ui_node_index;

   px_node_ctxt = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes[ui_index]);
   x_dest_sock_addr_in.us_sin_port_no = pal_htons (
      px_node_ctxt->us_udp_port_ho);
   x_dest_sock_addr_in.x_sin_addr.ui_ip_addr_no = pal_htonl (
      px_node_ctxt->ui_ip_addr_ho);
   ui_send_size = sizeof(px_sync_rsp->x_hdr)
      + px_sync_rsp->x_hdr.ui_msg_pay_len;
   ui_temp = ui_send_size;
   RFSVP_S_LOG_LOW("Sending eNODE_MSG_ID_ALGO_MSG_SYNC_RSP to 0x%x:%d",
      px_node_ctxt->ui_ip_addr_ho, px_node_ctxt->us_udp_port_ho);
   e_pal_ret = pal_sendto (px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl,
      (uint8_t *) px_sync_rsp, &ui_send_size, 0, &x_dest_sock_addr_in);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (ui_temp != ui_send_size))
   {
      RFSVP_S_LOG_LOW("pal_sendto failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_send_sync_update (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t ui_index,
   NODE_MSG_ALGO_SYNC_UPDATE_X *px_sync_update)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_send_size = 0;
   uint32_t ui_temp = 0;
   PAL_SOCK_ADDR_IN_X x_dest_sock_addr_in = {0};
   RFSVP_SERVER_NODE_CTXT_X *px_node_ctxt = NULL;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_sync_update)
      || (NULL == px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_sync_update->x_hdr.ui_node_index =
            px_rfsvp_server_ctxt->x_init_params.ui_node_index;
   px_node_ctxt = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes[ui_index]);
   x_dest_sock_addr_in.us_sin_port_no = pal_htons (
      px_node_ctxt->us_udp_port_ho);
   x_dest_sock_addr_in.x_sin_addr.ui_ip_addr_no = pal_htonl (
      px_node_ctxt->ui_ip_addr_ho);
   ui_send_size = sizeof(px_sync_update->x_hdr)
      + px_sync_update->x_hdr.ui_msg_pay_len;
   ui_temp = ui_send_size;
   RFSVP_S_LOG_LOW("Sending eNODE_MSG_ID_ALGO_MSG_SYNC_UPDATE to 0x%x:%d",
      px_node_ctxt->ui_ip_addr_ho, px_node_ctxt->us_udp_port_ho);
   e_pal_ret = pal_sendto (px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl,
      (uint8_t *) px_sync_update, &ui_send_size, 0, &x_dest_sock_addr_in);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (ui_temp != ui_send_size))
   {
      RFSVP_S_LOG_LOW("pal_sendto failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_send_commit_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t ui_index)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_send_size = 0;
   PAL_SOCK_ADDR_IN_X x_dest_sock_addr_in = {0};
   NODE_MSG_ALGO_COMMIT_REQ_X *px_commit_req = NULL;
   RFSVP_SERVER_NODE_CTXT_X *px_node_ctxt = NULL;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   RFSVP_SERVER_ALGO_VP_X *px_vp = NULL;
   uint8_t *puc_data_off_dst = 0;
   uint8_t *puc_data_off_src = 0;
   uint32_t ui_alloc_size = 0;
   NODE_MSG_APP_CMD_X *px_app_cmd = NULL;

   if ((NULL == px_rfsvp_server_ctxt)
      || (NULL == px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);
   px_vp = &(px_algo->x_vp);

   px_app_cmd = (NODE_MSG_APP_CMD_X *) px_algo->px_cur_req_msg_hdr;

   ui_alloc_size = sizeof(*px_commit_req) + px_app_cmd->ui_data_len;
   px_commit_req = pal_malloc (ui_alloc_size, NULL);
   if (NULL == px_commit_req)
   {
      RFSVP_S_LOG_LOW("pal_malloc failed");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_commit_req->x_hdr.ui_msg_id = eNODE_MSG_ID_ALGO_MSG_COMMIT_REQ;
   px_commit_req->x_hdr.ui_msg_pay_len = ui_alloc_size
      - sizeof(px_commit_req->x_hdr);
   px_commit_req->x_hdr.ui_node_index =
               px_rfsvp_server_ctxt->x_init_params.ui_node_index;
   px_commit_req->ui_server_idx =
      px_rfsvp_server_ctxt->x_init_params.ui_node_index;

   px_commit_req->x_updated_vp.ui_ru = px_vp->ui_ru;
   px_commit_req->x_updated_vp.ui_vn = px_vp->ui_vn;
   px_commit_req->x_updated_vp.ui_ds_count = px_vp->ui_ds_count;

   (void) pal_memmove(px_commit_req->x_updated_vp.uia_ds, px_vp->uia_ds,
      sizeof(px_commit_req->x_updated_vp.uia_ds));

   puc_data_off_src = (uint8_t *) &(px_app_cmd->ui_data_len);
   puc_data_off_src += sizeof(px_app_cmd->ui_data_len);

   puc_data_off_dst = (uint8_t *) &(px_commit_req->ui_data_len);
   puc_data_off_dst += sizeof(px_commit_req->ui_data_len);
   (void) pal_memmove (puc_data_off_dst, puc_data_off_src,
      px_app_cmd->ui_data_len);

   px_commit_req->ui_data_len = px_app_cmd->ui_data_len;

   px_node_ctxt = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes[ui_index]);
   x_dest_sock_addr_in.us_sin_port_no = pal_htons (
      px_node_ctxt->us_udp_port_ho);
   x_dest_sock_addr_in.x_sin_addr.ui_ip_addr_no = pal_htonl (
      px_node_ctxt->ui_ip_addr_ho);
   ui_send_size = ui_alloc_size;
   RFSVP_S_LOG_LOW("Sending eNODE_MSG_ID_ALGO_MSG_COMMIT_REQ to 0x%x:%d",
      px_node_ctxt->ui_ip_addr_ho, px_node_ctxt->us_udp_port_ho);
   e_pal_ret = pal_sendto (px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl,
      (uint8_t *) px_commit_req, &ui_send_size, 0, &x_dest_sock_addr_in);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (ui_alloc_size != ui_send_size))
   {
      RFSVP_S_LOG_LOW("pal_sendto failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   if (NULL != px_commit_req)
   {
      pal_free (px_commit_req);
      px_commit_req = NULL;
   }
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_send_commit_rsp (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t ui_index)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_send_size = 0;
   PAL_SOCK_ADDR_IN_X x_dest_sock_addr_in = {0};
   NODE_MSG_ALGO_COMMIT_RSP_X x_commit_rsp = {{0}};
   RFSVP_SERVER_NODE_CTXT_X *px_node_ctxt = NULL;

   if ((NULL == px_rfsvp_server_ctxt)
      || (NULL == px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_commit_rsp.x_hdr.ui_msg_id = eNODE_MSG_ID_ALGO_MSG_COMMIT_RSP;
   x_commit_rsp.x_hdr.ui_msg_pay_len = sizeof(x_commit_rsp)
      - sizeof(x_commit_rsp.x_hdr);
   x_commit_rsp.x_hdr.ui_node_index =
               px_rfsvp_server_ctxt->x_init_params.ui_node_index;
   x_commit_rsp.ui_server_idx =
      px_rfsvp_server_ctxt->x_init_params.ui_node_index;

   px_node_ctxt = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes[ui_index]);
   x_dest_sock_addr_in.us_sin_port_no = pal_htons (
      px_node_ctxt->us_udp_port_ho);
   x_dest_sock_addr_in.x_sin_addr.ui_ip_addr_no = pal_htonl (
      px_node_ctxt->ui_ip_addr_ho);
   ui_send_size = sizeof(x_commit_rsp);
   RFSVP_S_LOG_LOW("Sending eNODE_MSG_ID_ALGO_MSG_COMMIT_RSP to 0x%x:%d",
      px_node_ctxt->ui_ip_addr_ho, px_node_ctxt->us_udp_port_ho);
   e_pal_ret = pal_sendto (px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl,
      (uint8_t *) &x_commit_rsp, &ui_send_size, 0, &x_dest_sock_addr_in);
   if ((ePAL_RET_SUCCESS != e_pal_ret)
      || (sizeof(x_commit_rsp) != ui_send_size))
   {
      RFSVP_S_LOG_LOW("pal_sendto failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_send_abort_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t ui_index)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_send_size = 0;
   PAL_SOCK_ADDR_IN_X x_dest_sock_addr_in = {0};
   NODE_MSG_ALGO_ABORT_REQ_X x_abort_req = {{0}};
   RFSVP_SERVER_NODE_CTXT_X *px_node_ctxt = NULL;

   if ((NULL == px_rfsvp_server_ctxt)
      || (NULL == px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_abort_req.x_hdr.ui_msg_id = eNODE_MSG_ID_ALGO_MSG_ABORT_REQ;
   x_abort_req.x_hdr.ui_msg_pay_len = sizeof(x_abort_req)
      - sizeof(x_abort_req.x_hdr);
   x_abort_req.x_hdr.ui_node_index =
               px_rfsvp_server_ctxt->x_init_params.ui_node_index;
   x_abort_req.ui_server_idx =
      px_rfsvp_server_ctxt->x_init_params.ui_node_index;

   px_node_ctxt = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes [ui_index]);
   x_dest_sock_addr_in.us_sin_port_no = pal_htons (
      px_node_ctxt->us_udp_port_ho);
   x_dest_sock_addr_in.x_sin_addr.ui_ip_addr_no = pal_htonl (
      px_node_ctxt->ui_ip_addr_ho);
   ui_send_size = sizeof(x_abort_req);
   RFSVP_S_LOG_LOW("Sending eNODE_MSG_ID_ALGO_MSG_ABORT_REQ to 0x%x:%d",
      px_node_ctxt->ui_ip_addr_ho, px_node_ctxt->us_udp_port_ho);
   e_pal_ret = pal_sendto (px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl,
      (uint8_t *) &x_abort_req, &ui_send_size, 0, &x_dest_sock_addr_in);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (sizeof(x_abort_req) != ui_send_size))
   {
      RFSVP_S_LOG_LOW("pal_sendto failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_send_abort_rsp (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t ui_index)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_send_size = 0;
   PAL_SOCK_ADDR_IN_X x_dest_sock_addr_in = {0};
   NODE_MSG_ALGO_ABORT_RSP_X x_abort_rsp = {{0}};
   RFSVP_SERVER_NODE_CTXT_X *px_node_ctxt = NULL;

   if ((NULL == px_rfsvp_server_ctxt)
      || (NULL == px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_abort_rsp.x_hdr.ui_msg_id = eNODE_MSG_ID_ALGO_MSG_ABORT_RSP;
   x_abort_rsp.x_hdr.ui_msg_pay_len = sizeof(x_abort_rsp)
      - sizeof(x_abort_rsp.x_hdr);
   x_abort_rsp.x_hdr.ui_node_index =
               px_rfsvp_server_ctxt->x_init_params.ui_node_index;
   x_abort_rsp.ui_server_idx =
      px_rfsvp_server_ctxt->x_init_params.ui_node_index;

   px_node_ctxt = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes [ui_index]);
   x_dest_sock_addr_in.us_sin_port_no = pal_htons (
      px_node_ctxt->us_udp_port_ho);
   x_dest_sock_addr_in.x_sin_addr.ui_ip_addr_no = pal_htonl (
      px_node_ctxt->ui_ip_addr_ho);
   ui_send_size = sizeof(x_abort_rsp);
   RFSVP_S_LOG_LOW("Sending eNODE_MSG_ID_ALGO_MSG_ABORT_RSP to 0x%x:%d",
      px_node_ctxt->ui_ip_addr_ho, px_node_ctxt->us_udp_port_ho);
   e_pal_ret = pal_sendto (px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl,
      (uint8_t *) &x_abort_rsp, &ui_send_size, 0, &x_dest_sock_addr_in);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (sizeof(x_abort_rsp) != ui_send_size))
   {
      RFSVP_S_LOG_LOW("pal_sendto failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_send_lock_req (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t ui_index)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_send_size = 0;
   PAL_SOCK_ADDR_IN_X x_dest_sock_addr_in = {0};
   NODE_MSG_ALGO_LOCK_REQ_X x_lock_req = {{0}};
   RFSVP_SERVER_NODE_CTXT_X *px_node_ctxt = NULL;

   if ((NULL == px_rfsvp_server_ctxt)
      || (NULL == px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_lock_req.x_hdr.ui_msg_id = eNODE_MSG_ID_ALGO_MSG_LOCK_REQ;
   x_lock_req.x_hdr.ui_msg_pay_len = sizeof(x_lock_req)
      - sizeof(x_lock_req.x_hdr);
   x_lock_req.x_hdr.ui_node_index =
               px_rfsvp_server_ctxt->x_init_params.ui_node_index;
   x_lock_req.ui_server_idx =
      px_rfsvp_server_ctxt->x_init_params.ui_node_index;

   x_lock_req.ui_seq_no = px_rfsvp_server_ctxt->x_algo.ui_highest_seq_no + 1;

   px_node_ctxt = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes[ui_index]);
   x_dest_sock_addr_in.us_sin_port_no = pal_htons (
      px_node_ctxt->us_udp_port_ho);
   x_dest_sock_addr_in.x_sin_addr.ui_ip_addr_no = pal_htonl (
      px_node_ctxt->ui_ip_addr_ho);
   ui_send_size = sizeof(x_lock_req);
   RFSVP_S_LOG_LOW("Sending eNODE_MSG_ID_ALGO_MSG_LOCK_REQ to 0x%x:%d",
      px_node_ctxt->ui_ip_addr_ho, px_node_ctxt->us_udp_port_ho);
   e_pal_ret = pal_sendto (px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl,
      (uint8_t *) &x_lock_req, &ui_send_size, 0, &x_dest_sock_addr_in);
   if ((ePAL_RET_SUCCESS != e_pal_ret)
      || (sizeof(x_lock_req) != ui_send_size))
   {
      RFSVP_S_LOG_LOW("pal_sendto failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_send_lock_rsp (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint32_t ui_index)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_send_size = 0;
   PAL_SOCK_ADDR_IN_X x_dest_sock_addr_in = {0};
   NODE_MSG_ALGO_LOCK_RSP_X x_lock_rsp = {{0}};
   RFSVP_SERVER_NODE_CTXT_X *px_node_ctxt = NULL;

   if ((NULL == px_rfsvp_server_ctxt)
      || (NULL == px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_lock_rsp.x_hdr.ui_msg_id = eNODE_MSG_ID_ALGO_MSG_LOCK_RSP;
   x_lock_rsp.x_hdr.ui_msg_pay_len = sizeof(x_lock_rsp)
      - sizeof(x_lock_rsp.x_hdr);
   x_lock_rsp.x_hdr.ui_node_index =
               px_rfsvp_server_ctxt->x_init_params.ui_node_index;
   x_lock_rsp.ui_server_idx =
      px_rfsvp_server_ctxt->x_init_params.ui_node_index;

   px_node_ctxt = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes[ui_index]);
   x_dest_sock_addr_in.us_sin_port_no = pal_htons (
      px_node_ctxt->us_udp_port_ho);
   x_dest_sock_addr_in.x_sin_addr.ui_ip_addr_no = pal_htonl (
      px_node_ctxt->ui_ip_addr_ho);
   ui_send_size = sizeof(x_lock_rsp);
   RFSVP_S_LOG_LOW("Sending eNODE_MSG_ID_ALGO_MSG_LOCK_RSP to 0x%x:%d",
      px_node_ctxt->ui_ip_addr_ho, px_node_ctxt->us_udp_port_ho);
   e_pal_ret = pal_sendto (px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl,
      (uint8_t *) &x_lock_rsp, &ui_send_size, 0, &x_dest_sock_addr_in);
   if ((ePAL_RET_SUCCESS != e_pal_ret)
      || (sizeof(x_lock_rsp) != ui_send_size))
   {
      RFSVP_S_LOG_LOW("pal_sendto failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}
