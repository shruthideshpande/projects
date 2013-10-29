/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   rfsvp_server_node_msgs.c
 * \author sandeepprakash
 *
 * \date   24-Nov-2012
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
static RFSVP_RET_E rfsvp_server_node_handle_udp_ping (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   PAL_SOCK_ADDR_IN_X *px_src_sock_addr_in,
   PAL_SOCK_HDL *phl_act_sock_hdl,
   NODE_MSG_HDR_X *px_msg_header);

/****************************** LOCAL FUNCTIONS *******************************/
RFSVP_RET_E rfsvp_server_node_handle_tcp_msgs_from_nodes (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   PAL_SOCK_HDL *phl_act_sock_hdl,
   uint8_t *puc_msg_data,
   uint32_t ui_msg_size)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_HDR_X *px_msg_header = NULL;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == phl_act_sock_hdl)
      || (NULL == puc_msg_data) || (0 == ui_msg_size))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_msg_header = (NODE_MSG_HDR_X *) px_rfsvp_server_ctxt->uca_temp_sock_buf;
   switch (px_msg_header->ui_msg_id)
   {
      /*
       * Node Setup Messages.
       */
      case eNODE_MSG_ID_SETUP_JOIN_DONE:
      {
         RFSVP_S_LOG_MED("Got eNODE_MSG_ID_SETUP_JOIN_DONE");
         e_error = rfsvp_server_node_handle_setup_join_done (
            px_rfsvp_server_ctxt, phl_act_sock_hdl, px_msg_header);
         break;
      }
      case eNODE_MSG_ID_SETUP_ESTABLISH_PEERS:
      {
         RFSVP_S_LOG_MED("Got eNODE_MSG_ID_SETUP_ESTABLISH_PEERS");
         e_error = rfsvp_server_node_handle_setup_establish_peers (
            px_rfsvp_server_ctxt, phl_act_sock_hdl, px_msg_header);
         break;
      }
      case eNODE_MSG_ID_APP_MSG_CMD:
      {
         e_error = rfsvp_server_node_forward_msg (px_rfsvp_server_ctxt,
            phl_act_sock_hdl, px_msg_header);
         break;
      }
      default:
      {
         RFSVP_S_LOG_LOW("Invalid message: %d received.",
            px_msg_header->ui_msg_id);
         e_error = eRFSVP_RET_INVALID_ARGS;
         break;
      }
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_tcp_data_from_nodes (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   PAL_SOCK_HDL *phl_act_sock_hdl,
   uint8_t *puc_msg_data,
   uint32_t ui_msg_size)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == phl_act_sock_hdl)
      || (NULL == puc_msg_data) || (0 == ui_msg_size))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   e_error = rfsvp_server_node_register_sock (px_rfsvp_server_ctxt,
      *phl_act_sock_hdl);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW("rfsvp_server_node_register_sock failed: %d", e_error);

   }
   else
   {
      e_error = rfsvp_server_node_handle_tcp_msgs_from_nodes (px_rfsvp_server_ctxt,
         phl_act_sock_hdl, puc_msg_data, ui_msg_size);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_S_LOG_LOW("rfsvp_server_node_handle_msgs_from_clients failed: %d",
            e_error);
      }
   }
CLEAN_RETURN:
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      if ((NULL != phl_act_sock_hdl) && (NULL != *phl_act_sock_hdl))
      {
         e_pal_ret = pal_sock_close (*phl_act_sock_hdl);
         if (ePAL_RET_SUCCESS != e_pal_ret)
         {
            RFSVP_S_LOG_MED("pal_sock_close failed: %d", e_pal_ret);
         }
         *phl_act_sock_hdl = NULL;
      }
   }
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_tcp_conn_sock_act (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   PAL_SOCK_HDL *phl_act_sock_hdl)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_recv_size = 0;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == phl_act_sock_hdl))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   ui_recv_size = sizeof(px_rfsvp_server_ctxt->uca_temp_sock_buf);

   e_error = rfsvp_server_node_read_msg (*phl_act_sock_hdl,
      px_rfsvp_server_ctxt->uca_temp_sock_buf,
      sizeof(px_rfsvp_server_ctxt->uca_temp_sock_buf), &ui_recv_size);
   if ((eRFSVP_RET_SUCCESS == e_error)
      && (ui_recv_size >= sizeof(NODE_MSG_HDR_X)))
   {
      RFSVP_S_LOG_LOW("Received %d bytes.", ui_recv_size);
      e_error = rfsvp_server_node_handle_tcp_data_from_nodes (px_rfsvp_server_ctxt,
         phl_act_sock_hdl, px_rfsvp_server_ctxt->uca_temp_sock_buf, ui_recv_size);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_S_LOG_LOW("rfsvp_server_node_handle_data_from_clients failed: %d",
            e_error);
      }
   }
   else
   {
      RFSVP_S_LOG_LOW("rfsvp_server_node_read_msg failed: %d. Hence closing all sockets.",
         e_pal_ret);
      e_pal_ret = pal_sock_close(*phl_act_sock_hdl);
      if (ePAL_RET_SUCCESS != e_pal_ret)
      {
         RFSVP_S_LOG_LOW("pal_sock_close failed: %d", e_pal_ret);
      }
      *phl_act_sock_hdl = NULL;

      (void) rfsvp_server_node_cleanup_socks (px_rfsvp_server_ctxt);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_udp_data_from_nodes (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   PAL_SOCK_ADDR_IN_X *px_src_sock_addr_in,
   PAL_SOCK_HDL *phl_act_sock_hdl,
   uint8_t *puc_msg_data,
   uint32_t ui_msg_size)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_RET_E e_error_pvt = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == phl_act_sock_hdl)
      || (NULL == puc_msg_data) || (0 == ui_msg_size))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   e_error = rfsvp_server_node_register_sock (px_rfsvp_server_ctxt,
      *phl_act_sock_hdl);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW("rfsvp_server_node_register_sock failed: %d", e_error);
   }
   else
   {
      e_error = rfsvp_server_node_handle_udp_msgs_from_nodes (
         px_rfsvp_server_ctxt, px_src_sock_addr_in, phl_act_sock_hdl,
         puc_msg_data, ui_msg_size);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_S_LOG_LOW(
            "rfsvp_server_node_handle_msgs_from_clients failed: %d", e_error);
      }
   }
CLEAN_RETURN:
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      if ((NULL != phl_act_sock_hdl) && (NULL != *phl_act_sock_hdl))
      {
         e_error_pvt = rfsvp_server_node_deregister_sock (px_rfsvp_server_ctxt,
            *phl_act_sock_hdl);
         if (eRFSVP_RET_SUCCESS != e_error_pvt)
         {
            RFSVP_S_LOG_LOW( "rfsvp_server_node_deregister_sock failed: %d",
               e_error_pvt);
         }
         e_pal_ret = pal_sock_close (*phl_act_sock_hdl);
         if (ePAL_RET_SUCCESS != e_pal_ret)
         {
            RFSVP_S_LOG_MED("pal_sock_close failed: %d", e_pal_ret);
         }
         *phl_act_sock_hdl = NULL;
      }
   }
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_udp_conn_sock_act (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   PAL_SOCK_HDL *phl_act_sock_hdl)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_recv_size = 0;
   PAL_SOCK_ADDR_IN_X x_src_sock_addr_in = {0};


   if ((NULL == px_rfsvp_server_ctxt) || (NULL == phl_act_sock_hdl))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   ui_recv_size = sizeof(px_rfsvp_server_ctxt->uca_temp_sock_buf);
   e_pal_ret = pal_recvfrom (*phl_act_sock_hdl,
      px_rfsvp_server_ctxt->uca_temp_sock_buf, &ui_recv_size, 0,
      &x_src_sock_addr_in);
   if ((ePAL_RET_SUCCESS == e_pal_ret)
      && (ui_recv_size >= sizeof(NODE_MSG_HDR_X)))
   {
      RFSVP_S_LOG_LOW("Received %d bytes.", ui_recv_size);
      e_error = rfsvp_server_node_handle_udp_data_from_nodes (
         px_rfsvp_server_ctxt, &x_src_sock_addr_in, phl_act_sock_hdl,
         px_rfsvp_server_ctxt->uca_temp_sock_buf, ui_recv_size);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_S_LOG_LOW( "rfsvp_server_node_handle_data_from_nodes failed: %d",
            e_error);
      }
   }
   else
   {
      RFSVP_S_LOG_LOW("pal_recvfrom failed: %d. Hence closing all sockets.",
         e_pal_ret);
      (void) rfsvp_server_node_cleanup_socks (px_rfsvp_server_ctxt);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_udp_msgs_from_nodes (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   PAL_SOCK_ADDR_IN_X *px_src_sock_addr_in,
   PAL_SOCK_HDL *phl_act_sock_hdl,
   uint8_t *puc_msg_data,
   uint32_t ui_msg_size)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_HDR_X *px_msg_header = NULL;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_src_sock_addr_in)
      || (NULL == phl_act_sock_hdl) || (NULL == puc_msg_data)
      || (0 == ui_msg_size))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_msg_header = (NODE_MSG_HDR_X *) px_rfsvp_server_ctxt->uca_temp_sock_buf;

   switch (px_msg_header->ui_msg_id)
   {
      case eNODE_MSG_ID_SETUP_PING:
      {
         e_error = rfsvp_server_node_handle_udp_ping (px_rfsvp_server_ctxt,
            px_src_sock_addr_in, phl_act_sock_hdl, px_msg_header);
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
         e_error = rfsvp_server_node_forward_msg (px_rfsvp_server_ctxt,
            phl_act_sock_hdl, px_msg_header);
         break;
      }
      default:
      {
         RFSVP_S_LOG_LOW("Invalid message: %d received.",
            px_msg_header->ui_msg_id);
         e_error = eRFSVP_RET_INVALID_ARGS;
         break;
      }
   }
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_server_node_handle_udp_ping (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   PAL_SOCK_ADDR_IN_X *px_src_sock_addr_in,
   PAL_SOCK_HDL *phl_act_sock_hdl,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_PING_X *px_ping = NULL;
   uint8_t uca_ip_addr_str[16] = {0};
   uint32_t ui_index = 0;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_src_sock_addr_in)
      || (NULL == phl_act_sock_hdl) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_ping = (NODE_MSG_PING_X *) px_msg_header;

   e_error = rfsvp_server_node_get_active_udp_sock_index (px_rfsvp_server_ctxt,
      px_msg_header, px_src_sock_addr_in, &ui_index);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW( "rfsvp_server_node_get_active_udp_sock_index failed: %d",
         e_error);
      goto CLEAN_RETURN;
   }

   (void) pal_get_ip_addr_str (&(px_src_sock_addr_in->x_sin_addr),
      uca_ip_addr_str, sizeof(uca_ip_addr_str));

   RFSVP_S_LOG_MED("Got eNODE_MSG_ID_SETUP_PING from %d:%s:%d:%d sent @ %d ms",
      ui_index, uca_ip_addr_str,
      px_rfsvp_server_ctxt->x_server_nodes.xa_nodes[ui_index].us_udp_port_ho,
      px_ping->ui_ping_seq_no, px_ping->ui_ping_local_ts_ms);
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}
