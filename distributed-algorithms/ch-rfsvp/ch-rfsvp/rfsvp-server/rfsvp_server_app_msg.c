/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   rfsvp_server_app_msg.c
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
RFSVP_RET_E rfsvp_server_node_handle_app_msg_halt (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_APP_CMD_RSP_X x_rsp = {{0}};
   RFSVP_SERVER_NODE_EXIT_TASK_DATA_X *px_exit_data = NULL;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_rsp.x_hdr.ui_msg_id = eNODE_MSG_ID_APP_MSG_CMD_RSP;
   x_rsp.x_hdr.ui_msg_pay_len = sizeof(x_rsp) - sizeof(x_rsp.x_hdr);

   x_rsp.e_cmd = eRFSVP_SERVER_CMD_HALT;
   x_rsp.ui_server_idx = px_rfsvp_server_ctxt->x_init_params.ui_node_index;
   x_rsp.e_rsp_code = eNODE_MSG_CMD_RSP_CODE_SUCCESS;
   e_error = rfsvp_server_node_send_app_msg_cmd_rsp_to_mgr (
      px_rfsvp_server_ctxt, &x_rsp);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW(
         "rfsvp_server_node_send_app_msg_cmd_rsp_to_mgr failed: %d", e_error);
   }


   px_exit_data = pal_malloc (sizeof(RFSVP_SERVER_NODE_EXIT_TASK_DATA_X),
      NULL);

   px_exit_data->x_hdr.ui_msg_id = eRFSVP_SERVER_MSG_ID_EXIT_TASK;
   px_exit_data->x_hdr.ui_msg_pay_len = sizeof(*px_exit_data) -
         sizeof(px_exit_data->x_hdr);
   e_error = rfsvp_server_post_msg_to_main_task_q (
      px_rfsvp_server_ctxt,
      (uint8_t *) px_exit_data,
      sizeof(*px_exit_data));
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW("rfsvp_server_post_msg_to_q failed: %d", e_error);
      pal_free(px_exit_data);
      px_exit_data = NULL;
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_app_msg_node_down (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_SUCCESS;
   NODE_MSG_APP_CMD_RSP_X x_rsp = {{0}};

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   if (NULL != px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl)
   {
      RFSVP_S_LOG_LOW("Deregistering UDP socket of node");

      e_error = rfsvp_server_node_deregister_sock (px_rfsvp_server_ctxt,
         px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_S_LOG_LOW("rfsvp_server_node_deregister_sock failed: %d",
            e_error);
      }

      RFSVP_S_LOG_LOW("Closing socket of node");
      e_pal_ret = udp_sock_delete (
         px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl);
      if (ePAL_RET_SUCCESS != e_pal_ret)
      {
         RFSVP_S_LOG_LOW("pal_sock_close failed: %d", e_pal_ret);
      }
      px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl = NULL;
   }

   x_rsp.x_hdr.ui_msg_id = eNODE_MSG_ID_APP_MSG_CMD_RSP;
   x_rsp.x_hdr.ui_msg_pay_len = sizeof(x_rsp) - sizeof(x_rsp.x_hdr);

   x_rsp.e_cmd = eRFSVP_SERVER_CMD_NODE_DOWN;
   x_rsp.ui_server_idx = px_rfsvp_server_ctxt->x_init_params.ui_node_index;
   x_rsp.e_rsp_code = eNODE_MSG_CMD_RSP_CODE_SUCCESS;
   e_error = rfsvp_server_node_send_app_msg_cmd_rsp_to_mgr (
      px_rfsvp_server_ctxt, &x_rsp);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW(
         "rfsvp_server_node_send_app_msg_cmd_rsp_to_mgr failed: %d", e_error);
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_app_msg_node_up (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_NODE_CTXT_X *px_myself = NULL;
   uint32_t ui_my_node_idx = 0;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   uint32_t ui_alloc_size = 0;
   NODE_MSG_APP_CMD_RSP_X x_rsp = {{0}};

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
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
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   ui_alloc_size = sizeof(*px_msg_header) + px_msg_header->ui_msg_pay_len;
   px_algo->px_cur_req_msg_hdr = pal_malloc (ui_alloc_size, NULL );
   if (NULL == px_algo->px_cur_req_msg_hdr)
   {
      x_rsp.x_hdr.ui_msg_id = eNODE_MSG_ID_APP_MSG_CMD_RSP;
      x_rsp.x_hdr.ui_msg_pay_len = sizeof(x_rsp) - sizeof(x_rsp.x_hdr);

      x_rsp.e_cmd = eRFSVP_SERVER_CMD_NODE_UP;
      x_rsp.ui_server_idx = px_rfsvp_server_ctxt->x_init_params.ui_node_index;
      x_rsp.e_rsp_code = eNODE_MSG_CMD_RSP_CODE_FAILURE;
      e_error = rfsvp_server_node_send_app_msg_cmd_rsp_to_mgr (
         px_rfsvp_server_ctxt, &x_rsp);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_S_LOG_LOW(
            "rfsvp_server_node_send_app_msg_cmd_rsp_to_mgr failed: %d", e_error);
      }
   }
   else
   {
      (void) pal_memmove (px_algo->px_cur_req_msg_hdr, px_msg_header,
         ui_alloc_size);
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_REACH_REQ");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_REACH_REQ;
      e_error = eRFSVP_RET_SUCCESS;
   }

CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_app_msg_read (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   uint32_t ui_alloc_size = 0;
   NODE_MSG_APP_CMD_RSP_X x_rsp = {{0}};

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   ui_alloc_size = sizeof(*px_msg_header) + px_msg_header->ui_msg_pay_len;
   px_algo->px_cur_req_msg_hdr = pal_malloc (ui_alloc_size, NULL );
   if (NULL == px_algo->px_cur_req_msg_hdr)
   {
      x_rsp.x_hdr.ui_msg_id = eNODE_MSG_ID_APP_MSG_CMD_RSP;
      x_rsp.x_hdr.ui_msg_pay_len = sizeof(x_rsp) - sizeof(x_rsp.x_hdr);

      x_rsp.e_cmd = eRFSVP_SERVER_CMD_READ;
      x_rsp.ui_server_idx = px_rfsvp_server_ctxt->x_init_params.ui_node_index;
      x_rsp.e_rsp_code = eNODE_MSG_CMD_RSP_CODE_FAILURE;
      e_error = rfsvp_server_node_send_app_msg_cmd_rsp_to_mgr (
         px_rfsvp_server_ctxt, &x_rsp);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_S_LOG_LOW(
            "rfsvp_server_node_send_app_msg_cmd_rsp_to_mgr failed: %d", e_error);
      }
   }
   else
   {
      (void) pal_memmove (px_algo->px_cur_req_msg_hdr, px_msg_header,
         ui_alloc_size);
      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_REACH_REQ");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_REACH_REQ;
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_handle_app_msg_write (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_ALGO_X *px_algo = NULL;
   uint32_t ui_alloc_size = 0;
   NODE_MSG_APP_CMD_RSP_X x_rsp = {{0}};

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_header))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_algo = &(px_rfsvp_server_ctxt->x_algo);

   ui_alloc_size = sizeof(*px_msg_header) + px_msg_header->ui_msg_pay_len;
   px_algo->px_cur_req_msg_hdr = pal_malloc (ui_alloc_size, NULL );
   if (NULL == px_algo->px_cur_req_msg_hdr)
   {
      x_rsp.x_hdr.ui_msg_id = eNODE_MSG_ID_APP_MSG_CMD_RSP;
      x_rsp.x_hdr.ui_msg_pay_len = sizeof(x_rsp) - sizeof(x_rsp.x_hdr);

      x_rsp.e_cmd = eRFSVP_SERVER_CMD_WRITE;
      x_rsp.ui_server_idx = px_rfsvp_server_ctxt->x_init_params.ui_node_index;
      x_rsp.e_rsp_code = eNODE_MSG_CMD_RSP_CODE_FAILURE;
      e_error = rfsvp_server_node_send_app_msg_cmd_rsp_to_mgr (
         px_rfsvp_server_ctxt, &x_rsp);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_S_LOG_LOW(
            "rfsvp_server_node_send_app_msg_cmd_rsp_to_mgr failed: %d", e_error);
      }
   }
   else
   {
      (void) pal_memmove (px_algo->px_cur_req_msg_hdr, px_msg_header,
         ui_alloc_size);

      RFSVP_S_LOG_MED("Setting eRFSVP_SERVER_ALGO_SUB_STATE_REACH_REQ");
      px_algo->ui_sub_state |= eRFSVP_SERVER_ALGO_SUB_STATE_REACH_REQ;
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}
