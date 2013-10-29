/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   rfsvp_server_node_setup.c
 * \author sandeepprakash
 *
 * \date   29-Sep-2012
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
RFSVP_RET_E rfsvp_server_node_handle_setup_join_done (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   PAL_SOCK_HDL *phl_act_sock_hdl,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == phl_act_sock_hdl)
      || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_setup_establish_peers (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   PAL_SOCK_HDL *phl_act_sock_hdl,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_index = 0;
   NODE_MSG_ESTABLISH_PEERS_X *px_est_peers = NULL;
   uint32_t ui_exp_size = 0;
   NODE_MSG_PEER_DATA_X *px_peer_data = NULL;
   uint8_t *puc_peer_offset = NULL;
   uint8_t uca_ip_addr_str[16] = {0};
   PAL_SOCK_IN_ADDR_X x_in_addr = {0};
   RFSVP_SERVER_NODE_CTXT_X *px_myself = NULL;
   RFSVP_SERVER_NODE_CTXT_X *px_node = NULL;
   uint32_t ui_my_node_idx = 0;
   TASK_RET_E e_task_ret = eTASK_RET_FAILURE;
   TASK_CREATE_PARAM_X x_task_param = {{0}};
   MSGQ_RET_E                  e_msgq_ret       = eMSGQ_RET_FAILURE;
   MSGQ_INIT_PARAMS_X          x_msgq_param     = {0};

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == phl_act_sock_hdl)
      || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_est_peers = (NODE_MSG_ESTABLISH_PEERS_X *) px_msg_header;

   ui_exp_size += sizeof(NODE_MSG_ESTABLISH_PEERS_X);
   ui_exp_size +=
      (px_est_peers->x_peers.ui_count * sizeof(NODE_MSG_PEER_DATA_X));
   ui_exp_size = ui_exp_size - sizeof(px_est_peers->x_hdr);

   if (ui_exp_size != px_est_peers->x_hdr.ui_msg_pay_len)
   {
      RFSVP_S_LOG_LOW("Malformed message. Expected payload size: %d bytes, "
         "Got %d bytes.", ui_exp_size, px_est_peers->x_hdr.ui_msg_pay_len);
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   puc_peer_offset = (uint8_t *) &(px_est_peers->x_peers.ui_count);
   puc_peer_offset += sizeof(px_est_peers->x_peers.ui_count);
   for (ui_index = 0; ui_index < px_est_peers->x_peers.ui_count; ui_index++)
   {
      px_node = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes[ui_index]);
      px_peer_data = (NODE_MSG_PEER_DATA_X *) puc_peer_offset;

      px_node->ui_ip_addr_ho = px_peer_data->ui_ip_addr_ho;
      px_node->us_udp_port_ho = px_peer_data->us_udp_port_no_ho;
      (void) pal_strncpy(px_node->uca_dns_name_str,
         px_peer_data->uca_host_name_str, sizeof(px_node->uca_dns_name_str));

      x_in_addr.ui_ip_addr_no = pal_htonl (px_peer_data->ui_ip_addr_ho);
      (void) pal_get_ip_addr_str (&x_in_addr, uca_ip_addr_str,
            sizeof(uca_ip_addr_str));

      RFSVP_S_LOG_LOW("Peer Info: %d, %s (0x%x):%d", ui_index,
         uca_ip_addr_str, px_peer_data->ui_ip_addr_ho,
         px_peer_data->us_udp_port_no_ho);

      puc_peer_offset += sizeof(NODE_MSG_PEER_DATA_X);
   }

   x_msgq_param.ui_msgq_size = RFSVP_SERVER_TASK_MSGQ_SIZE;
   e_msgq_ret = msgq_init (&(px_rfsvp_server_ctxt->x_algo.hl_algo_req_msgq_hdl),
      &x_msgq_param);
   if ((eMSGQ_RET_SUCCESS != e_msgq_ret)
      || (NULL == px_rfsvp_server_ctxt->x_algo.hl_algo_req_msgq_hdl))
   {
      RFSVP_S_LOG_LOW("msgq_init failed: %d", e_msgq_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }
   else
   {
      RFSVP_S_LOG_LOW("msgq_init success");
   }

   x_msgq_param.ui_msgq_size = RFSVP_SERVER_TASK_MSGQ_SIZE;
   e_msgq_ret = msgq_init (&(px_rfsvp_server_ctxt->x_algo.hl_app_req_msgq_hdl),
      &x_msgq_param);
   if ((eMSGQ_RET_SUCCESS != e_msgq_ret)
      || (NULL == px_rfsvp_server_ctxt->x_algo.hl_app_req_msgq_hdl))
   {
      RFSVP_S_LOG_LOW("msgq_init failed: %d", e_msgq_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }
   else
   {
      RFSVP_S_LOG_LOW("msgq_init success");
   }

   x_task_param.b_msgq_needed = true;
   x_task_param.ui_msgq_size = RFSVP_SERVER_TASK_MSGQ_SIZE;
   x_task_param.fn_task_routine = rfsvp_server_node_algo_task;
   x_task_param.p_app_data = px_rfsvp_server_ctxt;
   e_task_ret = task_create(&(px_rfsvp_server_ctxt->x_algo.hl_algo_task_hdl),
      &x_task_param);
   if ((eTASK_RET_SUCCESS != e_task_ret)
      || (NULL == px_rfsvp_server_ctxt->x_algo.hl_algo_task_hdl))
   {
      RFSVP_S_LOG_LOW("task_create failed: %d, %p", e_task_ret,
         px_rfsvp_server_ctxt, px_rfsvp_server_ctxt->x_algo.hl_algo_task_hdl);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_task_ret = task_start (px_rfsvp_server_ctxt->x_algo.hl_algo_task_hdl);
      if (eTASK_RET_SUCCESS != e_task_ret)
      {
         RFSVP_S_LOG_LOW("task_start failed: %d", e_task_ret);
         e_error = eRFSVP_RET_RESOURCE_FAILURE;
         goto CLEAN_RETURN;
      }
      else
      {
         RFSVP_S_LOG_MED("task_create success for hl_algo_task_hdl");
      }
   }

   ui_my_node_idx = px_rfsvp_server_ctxt->x_init_params.ui_node_index;
   px_myself = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes[ui_my_node_idx]);

   e_pal_ret = udp_sock_create (
      &(px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl),
      px_myself->us_udp_port_ho);
   if ((ePAL_RET_SUCCESS != e_pal_ret)
      || (NULL == px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl))
   {
      RFSVP_S_LOG_LOW("udp_sock_create failed: %d", e_pal_ret);
      goto CLEAN_RETURN;
   }

   RFSVP_S_LOG_LOW("Registering the socket.");

   e_error = rfsvp_server_node_register_sock (px_rfsvp_server_ctxt,
      px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW("rfsvp_server_node_register_sock failed: %d", e_error);
      goto CLEAN_RETURN;
   }

   e_error = rfsvp_server_node_send_establish_done_to_mgr (
      px_rfsvp_server_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW("rfsvp_server_node_send_establish_done_to_mgr failed: %d",
         e_error);
   }
CLEAN_RETURN:
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      if (NULL !=  px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl)
      {
         e_pal_ret = udp_sock_delete (
            px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl);
         if (ePAL_RET_SUCCESS != e_pal_ret)
         {
            RFSVP_S_LOG_LOW("udp_sock_delete failed: %d", e_pal_ret);
         }
         px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl = NULL;
      }
      if (NULL != px_rfsvp_server_ctxt->x_algo.hl_algo_task_hdl)
      {
         e_task_ret = task_delete (px_rfsvp_server_ctxt->x_algo.hl_algo_task_hdl);
         if (eTASK_RET_SUCCESS != e_task_ret)
         {
            RFSVP_S_LOG_LOW("task_delete failed: %d", e_task_ret);
         }
         px_rfsvp_server_ctxt->x_algo.hl_algo_task_hdl = NULL;
      }
      if (NULL != px_rfsvp_server_ctxt->x_algo.hl_algo_req_msgq_hdl)
      {
         e_msgq_ret = msgq_deinit(px_rfsvp_server_ctxt->x_algo.hl_algo_req_msgq_hdl);
         if (eMSGQ_RET_SUCCESS != e_msgq_ret)
         {
            RFSVP_S_LOG_LOW("msgq_deinit failed: %d", e_msgq_ret);
         }
         px_rfsvp_server_ctxt->x_algo.hl_algo_req_msgq_hdl = NULL;
      }
   }
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_forward_msg (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   PAL_SOCK_HDL *phl_act_sock_hdl,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   MSGQ_DATA_X x_data = {NULL};
   TASK_RET_E e_task_ret = eTASK_RET_FAILURE;
   uint32_t ui_index = 0;
   RFSVP_SERVER_ALGO_MSG_DATA_X *px_msg_data = NULL;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == phl_act_sock_hdl)
      || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_msg_data = pal_malloc (sizeof(RFSVP_SERVER_ALGO_MSG_DATA_X), NULL);
   if (NULL == px_msg_data)
   {
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   px_msg_data->ui_src_node_index = ui_index;
   px_msg_data->ui_data_len =
      (sizeof(NODE_MSG_HDR_X) + px_msg_header->ui_msg_pay_len);
   px_msg_data->puc_data = pal_malloc (px_msg_data->ui_data_len, NULL );
   if (NULL == px_msg_data->puc_data)
   {
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   (void) pal_memmove (px_msg_data->puc_data, px_msg_header,
      px_msg_data->ui_data_len);

   x_data.p_data = px_msg_data;
   x_data.ui_data_size = sizeof(RFSVP_SERVER_ALGO_MSG_DATA_X);
   e_task_ret = task_add_msg_to_q (
      px_rfsvp_server_ctxt->x_algo.hl_algo_task_hdl, &x_data,
      RFSVP_SERVER_ALGO_TASK_Q_WAIT_TIMEOUT);
   if (eTASK_RET_SUCCESS != e_task_ret)
   {
      RFSVP_S_LOG_LOW("task_add_msg_to_q failed: %d",
         e_task_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      if (NULL != px_msg_data)
      {
         if (NULL != px_msg_data->puc_data)
         {
            pal_free(px_msg_data->puc_data);
            px_msg_data->puc_data = NULL;
         }
         pal_free (px_msg_data);
         px_msg_data = NULL;
      }
   }
   return e_error;
}
