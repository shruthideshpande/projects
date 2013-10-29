/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   rfsvp_mgr_node_utils.c
 * \author sandeepprakash
 *
 * \date   29-Sep-2012
 *
 * \brief  
 *
 ******************************************************************************/

/********************************** INCLUDES **********************************/
#include "rfsvp_mgr_env.h"

/********************************* CONSTANTS **********************************/

/*********************************** MACROS ***********************************/
#define RFSVP_MGR_NODE_READ_MSG_TIMEOUT_MS                 (5000)

/******************************** ENUMERATIONS ********************************/

/************************* STRUCTURE/UNION DATA TYPES *************************/

/************************ STATIC FUNCTION PROTOTYPES **************************/

/****************************** LOCAL FUNCTIONS *******************************/
RFSVP_RET_E rfsvp_mgr_node_register_sock (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   PAL_SOCK_HDL    hl_sock_hdl)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   SOCKMON_RET_E e_sockmon_ret = eSOCKMON_RET_FAILURE;
   SOCKMON_REGISTER_DATA_X x_register_data = {NULL};

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == hl_sock_hdl))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_register_data.hl_sock_hdl = hl_sock_hdl;
   x_register_data.fn_active_sock_cbk =
         rfsvp_mgr_node_sockmon_active_sock_cbk;
   x_register_data.p_app_data = px_rfsvp_mgr_ctxt;
   e_sockmon_ret = sockmon_register_sock (
      px_rfsvp_mgr_ctxt->x_init_params.hl_sockmon_hdl,
      &x_register_data);
   if (eSOCKMON_RET_SUCCESS != e_sockmon_ret)
   {
      RFSVP_M_LOG_LOW("sockmon_register_sock failed: %d", e_sockmon_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      RFSVP_M_LOG_MED("sockmon_register_sock success");
      e_error = eRFSVP_RET_SUCCESS;
   }

CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_mgr_node_deregister_sock (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   PAL_SOCK_HDL    hl_sock_hdl)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   SOCKMON_RET_E e_sockmon_ret = eSOCKMON_RET_FAILURE;
   SOCKMON_REGISTER_DATA_X x_register_data = {NULL};

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == hl_sock_hdl))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_register_data.hl_sock_hdl = hl_sock_hdl;
   e_sockmon_ret = sockmon_deregister_sock (
      px_rfsvp_mgr_ctxt->x_init_params.hl_sockmon_hdl, &x_register_data);
   if (eSOCKMON_RET_SUCCESS != e_sockmon_ret)
   {
      RFSVP_M_LOG_LOW("sockmon_deregister_sock failed: %d", e_sockmon_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      RFSVP_M_LOG_MED("sockmon_register_sock success");
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_mgr_node_cleanup_socks (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   uint32_t ui_index = 0;
   PAL_RET_E e_pal_ret = ePAL_RET_SUCCESS;

   if (NULL == px_rfsvp_mgr_ctxt)
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   for (ui_index = 0; ui_index < px_rfsvp_mgr_ctxt->x_init_params.ui_no_nodes;
         ui_index++)
   {
      if (NULL != px_rfsvp_mgr_ctxt->x_server_nodes.hla_tcp_sock_hdl [ui_index])
      {
         RFSVP_M_LOG_LOW("Deregistering socket of node %d", ui_index);
         e_error = rfsvp_mgr_node_deregister_sock (px_rfsvp_mgr_ctxt,
            px_rfsvp_mgr_ctxt->x_server_nodes.hla_tcp_sock_hdl [ui_index]);
         if (eRFSVP_RET_SUCCESS != e_error)
         {
            RFSVP_M_LOG_LOW ("rfsvp_mgr_node_deregister_sock failed: %d",
               e_error);
         }

         RFSVP_M_LOG_LOW("Closing socket of node %d", ui_index);
         e_pal_ret = pal_sock_close (
            px_rfsvp_mgr_ctxt->x_server_nodes.hla_tcp_sock_hdl [ui_index]);
         if (ePAL_RET_SUCCESS != e_pal_ret)
         {
            RFSVP_M_LOG_LOW("pal_sock_close failed: %d", e_pal_ret);
         }
         px_rfsvp_mgr_ctxt->x_server_nodes.hla_tcp_sock_hdl [ui_index] = NULL;
      }
   }
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_mgr_node_read_msg (
   PAL_SOCK_HDL hl_sock_hdl,
   uint8_t *puc_msg_buf,
   uint32_t ui_msg_buf_len,
   uint32_t *pui_msg_size)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_recv_len = 0;
   uint32_t ui_hdr_len = 0;
   NODE_MSG_HDR_X *px_msg_hdr = NULL;

   if ((NULL == hl_sock_hdl) || (NULL == puc_msg_buf) || (0 == ui_msg_buf_len)
      || (NULL == pui_msg_size) || (ui_msg_buf_len < sizeof(NODE_MSG_HDR_X)))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   /*
    * 1. Read the message header first.
    * 2. Get the payload size from the header.
    * 3. Read the payload size number of bytes from the socket.
    * 4. That will give the complete message.
    */
   ui_hdr_len = sizeof(NODE_MSG_HDR_X);
   ui_recv_len = ui_hdr_len;
   e_pal_ret = pal_sock_recv_fixed (hl_sock_hdl, puc_msg_buf, &ui_recv_len,
      0, RFSVP_MGR_NODE_READ_MSG_TIMEOUT_MS);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (ui_recv_len != ui_hdr_len))
   {
      if (ePAL_RET_SOCK_CLOSED == e_pal_ret)
      {
         RFSVP_M_LOG_LOW("pal_sock_recv_fixed failed. Connection closed by peer.",
            RFSVP_MGR_NODE_READ_MSG_TIMEOUT_MS);
      }
      else if (ePAL_RET_OPERATION_TIMEDOUT == e_pal_ret)
      {
         RFSVP_M_LOG_LOW("pal_sock_recv_fixed timedout after %d ms: %d",
            RFSVP_MGR_NODE_READ_MSG_TIMEOUT_MS);
      }
      else
      {
         RFSVP_M_LOG_LOW("pal_sock_recv_fixed for header failed: 0x%x, "
            "Header len: %d, Received: %d", e_pal_ret, ui_hdr_len, ui_recv_len);
      }
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   RFSVP_M_LOG_HIGH("Read header of %d bytes.", ui_recv_len);

   px_msg_hdr = (NODE_MSG_HDR_X *) puc_msg_buf;

   ui_recv_len = px_msg_hdr->ui_msg_pay_len;
   e_pal_ret = pal_sock_recv_fixed (hl_sock_hdl, (puc_msg_buf + ui_hdr_len),
      &ui_recv_len, 0, RFSVP_MGR_NODE_READ_MSG_TIMEOUT_MS);
   if ((ePAL_RET_SUCCESS != e_pal_ret)
      || (ui_recv_len != px_msg_hdr->ui_msg_pay_len))
   {
      if (ePAL_RET_SOCK_CLOSED == e_pal_ret)
      {
         RFSVP_M_LOG_LOW("pal_sock_recv_fixed failed. Connection closed by peer.",
            RFSVP_MGR_NODE_READ_MSG_TIMEOUT_MS);
      }
      else if (ePAL_RET_OPERATION_TIMEDOUT == e_pal_ret)
      {
         RFSVP_M_LOG_LOW("pal_sock_recv_fixed timedout after %d ms: %d",
            RFSVP_MGR_NODE_READ_MSG_TIMEOUT_MS);
      }
      else
      {
         RFSVP_M_LOG_LOW("pal_sock_recv_fixed for payload failed: %d, "
         "Header len: %d, Received: %d",
            e_pal_ret, px_msg_hdr->ui_msg_pay_len, ui_recv_len);
      }
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      RFSVP_M_LOG_HIGH("Read payload of %d bytes.", ui_recv_len);
      *pui_msg_size = ui_hdr_len + px_msg_hdr->ui_msg_pay_len;
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_mgr_node_send_msg (
   PAL_SOCK_HDL hl_sock_hdl,
   NODE_MSG_HDR_X *px_msg_hdr)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_send_len = 0;

   if ((NULL == hl_sock_hdl) || (NULL == px_msg_hdr))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   ui_send_len = sizeof(*px_msg_hdr) + px_msg_hdr->ui_msg_pay_len;
   e_pal_ret = pal_sock_send_fixed (hl_sock_hdl, (uint8_t *) px_msg_hdr,
      &ui_send_len, 0, RFSVP_MGR_SEND_MSG_TIMEOUT_MS);
   if (ePAL_RET_SUCCESS != e_pal_ret)
   {
      RFSVP_M_LOG_LOW("pal_sock_send_fixed failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      RFSVP_M_LOG_LOW("pal_sock_send_fixed success: %d bytes", ui_send_len);
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

void rfsvp_mgr_node_log_status(
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt)
{
#if 0
   uint32_t ui_index = 0;
   RFSVP_SERVER_NODE_CTXT_X *px_node_ctxt = NULL;
   PAL_SOCK_HDL *phl_sock_hdl = NULL;

   if (NULL != px_rfsvp_mgr_ctxt)
   {
      printf ("\n\n**********************************************************"
         "**********************************************************");
      printf ("\n**********************************************************"
         "**********************************************************");
      printf ("\n%6s | %32s | %10s | %5s | %10s\n",
         "ID", "DNS Name", "IP Address", "Port", "Connection");
      printf ("__________________________________________________________"
         "__________________________________________________________\n");
      for (ui_index = 0; ui_index < px_rfsvp_mgr_ctxt->x_init_params.ui_no_nodes;
            ui_index++)
      {
         px_node_ctxt = &(px_rfsvp_mgr_ctxt->x_server_nodes.xa_nodes[ui_index]);
         phl_sock_hdl = &(px_rfsvp_mgr_ctxt->x_server_nodes.hla_tcp_sock_hdl[ui_index]);
         if (0 == ui_index)
         {
            printf ("%6s", "Leader");
         }
         else if (px_rfsvp_mgr_ctxt->x_server_nodes.ui_this_node_idx == ui_index)
         {
            printf ("%6s", "Myself");
         }
         else
         {
            printf ("%6s", "Other");
         }
         printf (" | %32s | 0x%8x | %5d | %10s\n",
            px_node_ctxt->uca_node_dns_name_str,
            px_node_ctxt->ui_node_ip_addr_ho,
            px_node_ctxt->us_node_listen_port_ho,
            ((NULL != *phl_sock_hdl) ? ("Connected") : ("No")));
      }
      printf ("\n**********************************************************"
         "**********************************************************");
      printf ("\n**********************************************************"
         "**********************************************************");
      printf ("\n\n");
   }
#endif
}

bool rfsvp_mgr_check_all_nodes_have_joined (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt)
{
   bool b_joined = false;
   uint32_t ui_index = 0;
   RFSVP_SERVER_NODE_CTXT_X *px_node_ctxt = NULL;

   if (NULL != px_rfsvp_mgr_ctxt)
   {
      for (ui_index = 0; ui_index < px_rfsvp_mgr_ctxt->x_init_params.ui_no_nodes;
            ui_index++)
      {
         px_node_ctxt = &(px_rfsvp_mgr_ctxt->x_server_nodes.xa_nodes [ui_index]);
         if (eRFSVP_SERVER_NODE_STATE_JOINED != px_node_ctxt->e_state)
         {
            break;
         }
      }

      if (ui_index == px_rfsvp_mgr_ctxt->x_init_params.ui_no_nodes)
      {
         b_joined = true;
      }
   }
   else
   {
      RFSVP_M_LOG_LOW("Invalid Args");
   }
   return b_joined;
}

RFSVP_RET_E rfsvp_mgr_node_get_active_sock_index (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   PAL_SOCK_HDL *phl_act_sock_hdl,
   uint32_t *pui_index)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   uint32_t ui_index = 0;

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == phl_act_sock_hdl)
      || (NULL == pui_index) || (NULL == *phl_act_sock_hdl))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   for (ui_index = 0; ui_index < px_rfsvp_mgr_ctxt->x_init_params.ui_no_nodes;
         ui_index++)
   {
      if (px_rfsvp_mgr_ctxt->x_server_nodes.hla_tcp_sock_hdl [ui_index]
         == *phl_act_sock_hdl)
      {
         break;
      }
   }

   if (ui_index < px_rfsvp_mgr_ctxt->x_init_params.ui_no_nodes)
   {
      *pui_index = ui_index;
      e_error = eRFSVP_RET_SUCCESS;
   }
   else
   {
      RFSVP_M_LOG_LOW("*********************************************************");
      RFSVP_M_LOG_LOW("*************Fatal Error: Socket IDs Mismatch************");
      RFSVP_M_LOG_LOW("*********************************************************");
      e_error = eRFSVP_RET_INVALID_ARGS;
   }
CLEAN_RETURN:
   return e_error;
}
