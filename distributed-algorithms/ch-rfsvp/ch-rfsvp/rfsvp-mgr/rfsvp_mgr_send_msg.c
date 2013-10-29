/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   rfsvp_mgr_send_msg.c
 * \author sandeepprakash
 *
 * \date   07-Oct-2012
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

/****************************** LOCAL FUNCTIONS *******************************/
RFSVP_RET_E rfsvp_mgr_node_send_join_done_to_node (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   uint32_t ui_index)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   PAL_SOCK_HDL *phl_node_sock_hdl = NULL;
   NODE_MSG_JOIN_DONE_X x_join_done = {{0}};
   uint32_t ui_send_size = 0;

   if (NULL == px_rfsvp_mgr_ctxt)
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   if (ui_index > px_rfsvp_mgr_ctxt->x_init_params.ui_no_nodes)
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   phl_node_sock_hdl = &(px_rfsvp_mgr_ctxt->x_server_nodes.hla_tcp_sock_hdl[ui_index]);

   RFSVP_M_LOG_LOW("Sending eNODE_MSG_ID_SETUP_JOIN_DONE to node %d",
      ui_index);

   x_join_done.x_hdr.ui_msg_id = eNODE_MSG_ID_SETUP_JOIN_DONE;
   x_join_done.x_hdr.ui_msg_pay_len = sizeof(x_join_done)
      - sizeof(x_join_done.x_hdr);

   ui_send_size = sizeof(x_join_done);
   e_pal_ret = pal_sock_send_fixed (*phl_node_sock_hdl,
      (uint8_t *) &x_join_done, &ui_send_size, 0, RFSVP_MGR_SEND_MSG_TIMEOUT_MS);
   if (ePAL_RET_SUCCESS != e_pal_ret)
   {
      RFSVP_M_LOG_LOW("pal_sock_send_fixed to node %d failed: %d",
         ui_index, e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      RFSVP_M_LOG_LOW("pal_sock_send_fixed to node %d success: %d bytes", ui_index,
         ui_send_size);
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_mgr_node_send_establish_peers_to_node (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   uint32_t ui_index)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   PAL_SOCK_HDL *phl_node_sock_hdl = NULL;
   NODE_MSG_ESTABLISH_PEERS_X *px_est_peers = NULL;
   uint32_t ui_alloc_size = 0;
   NODE_MSG_PEERS_X *px_msg_peers = NULL;
   NODE_MSG_PEER_DATA_X *px_peer_data = NULL;
   uint8_t *puc_peer_offset = NULL;
   RFSVP_SERVER_NODE_CTXT_X *px_node_ctxt = NULL;
   uint32_t ui_send_size = 0;
   uint32_t ui_count = 0;

   if (NULL == px_rfsvp_mgr_ctxt)
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   if (ui_index > px_rfsvp_mgr_ctxt->x_init_params.ui_no_nodes)
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   phl_node_sock_hdl = &(px_rfsvp_mgr_ctxt->x_server_nodes.hla_tcp_sock_hdl[ui_index]);

   RFSVP_M_LOG_LOW("Sending eNODE_MSG_ID_SETUP_ESTABLISH_PEERS to node %d",
      ui_index);
   ui_alloc_size += sizeof(NODE_MSG_ESTABLISH_PEERS_X);
   ui_alloc_size += (px_rfsvp_mgr_ctxt->x_server_nodes.ui_no_nodes
      * sizeof(NODE_MSG_PEER_DATA_X));
   px_est_peers = pal_malloc (ui_alloc_size, NULL);
   if (NULL == px_est_peers)
   {
      RFSVP_M_LOG_LOW("pal_malloc failed");
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   px_est_peers->x_hdr.ui_msg_id = eNODE_MSG_ID_SETUP_ESTABLISH_PEERS;
   px_est_peers->x_hdr.ui_msg_pay_len = ui_alloc_size
      - sizeof(px_est_peers->x_hdr);

   px_msg_peers = &(px_est_peers->x_peers);

   puc_peer_offset = (uint8_t *) &(px_msg_peers->ui_count);
   puc_peer_offset += sizeof(px_msg_peers->ui_count);
   for (ui_count = 0; ui_count < px_rfsvp_mgr_ctxt->x_server_nodes.ui_no_nodes;
         ui_count++)
   {
      px_node_ctxt = &(px_rfsvp_mgr_ctxt->x_server_nodes.xa_nodes [ui_count]);
      px_peer_data = (NODE_MSG_PEER_DATA_X *) puc_peer_offset;

      px_peer_data->ui_ip_addr_ho = px_node_ctxt->ui_ip_addr_ho;
      pal_strncpy (px_peer_data->uca_host_name_str,
         px_node_ctxt->uca_dns_name_str,
         sizeof(px_peer_data->uca_host_name_str));
      px_peer_data->us_udp_port_no_ho = px_node_ctxt->us_udp_port_ho;
      puc_peer_offset += sizeof(NODE_MSG_PEER_DATA_X);
      px_msg_peers->ui_count++;
      RFSVP_M_LOG_FULL("Peer count: %d", px_msg_peers->ui_count);
   }

   ui_send_size = ui_alloc_size;
   e_pal_ret = pal_sock_send_fixed (*phl_node_sock_hdl,
      (uint8_t *) px_est_peers, &ui_send_size, 0, RFSVP_MGR_SEND_MSG_TIMEOUT_MS);
   if (ePAL_RET_SUCCESS != e_pal_ret)
   {
      RFSVP_M_LOG_LOW("pal_sock_send_fixed to node %d failed: %d",
         ui_index, e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      RFSVP_M_LOG_LOW("pal_sock_send_fixed to node %d success: %d bytes",
         ui_index, ui_send_size);
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   if (NULL != px_est_peers)
   {
      pal_free (px_est_peers);
      px_est_peers = NULL;
   }
   return e_error;
}

RFSVP_RET_E rfsvp_mgr_node_send_app_msg_cmd_to_node (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   uint32_t ui_index,
   NODE_MSG_APP_CMD_X *px_cmd)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_SOCK_HDL *phl_node_sock_hdl = NULL;

#if 0
   NODE_MSG_APP_CMD_X x_cmd = {{0}};
#endif

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == px_cmd))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   if (ui_index > px_rfsvp_mgr_ctxt->x_init_params.ui_no_nodes)
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   phl_node_sock_hdl =
      &(px_rfsvp_mgr_ctxt->x_server_nodes.hla_tcp_sock_hdl [ui_index]);

   RFSVP_M_LOG_LOW("Sending eNODE_MSG_ID_APP_MSG_CMD to node %d",
      ui_index);

#if 0
   x_cmd.x_hdr.ui_msg_id = eNODE_MSG_ID_APP_MSG_CMD;
   x_cmd.x_hdr.ui_msg_pay_len = sizeof(x_cmd)
      - sizeof(x_cmd.x_hdr);

   x_cmd.e_cmd = px_cmd->e_cmd;
   x_cmd.ui_server_idx = px_cmd->ui_server_idx;
#endif

   e_error = rfsvp_mgr_node_send_msg (*phl_node_sock_hdl, &(px_cmd->x_hdr));
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_node_send_msg to node %d failed: %d",
         ui_index, e_error);
   }
CLEAN_RETURN:
   return e_error;
}
