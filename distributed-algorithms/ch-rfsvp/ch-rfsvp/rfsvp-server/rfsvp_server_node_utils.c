/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   rfsvp_server_node_utils.c
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
#define RFSVP_SERVER_NODE_READ_MSG_TIMEOUT_MS                 (5000)

/******************************** ENUMERATIONS ********************************/

/************************* STRUCTURE/UNION DATA TYPES *************************/

/************************ STATIC FUNCTION PROTOTYPES **************************/
static RFSVP_RET_E rfsvp_server_node_connect_to_mgr (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt);

/****************************** LOCAL FUNCTIONS *******************************/
RFSVP_RET_E rfsvp_server_node_establish_conn_to_mgr (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   e_error = rfsvp_server_node_connect_to_mgr (px_rfsvp_server_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW("rfsvp_server_node_connect_to_mgr failed: %d", e_error);
   }
   else
   {
      e_error = rfsvp_server_node_send_join_to_mgr (px_rfsvp_server_ctxt);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_S_LOG_LOW("rfsvp_server_node_send_join_to_mgr failed: %d",
            e_error);
      }
   }
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_server_node_connect_to_mgr (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_SOCK_HDL *phl_mgr_sock_hdl = NULL;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   PAL_SOCK_ADDR_IN_X x_local_sock_addr = {0};
   PAL_SOCK_IN_ADDR_X x_in_addr = {0};
   RFSVP_MGR_NODE_CTXT_X *px_mgr = NULL;
   uint32_t ui_dns_name_len = 0;
   uint32_t ui_ip_addr_no = 0;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_mgr = &(px_rfsvp_server_ctxt->x_mgr_node);
   phl_mgr_sock_hdl = &(px_rfsvp_server_ctxt->hl_mgr_sock_hdl);

   e_pal_ret = pal_sock_create(phl_mgr_sock_hdl, ePAL_SOCK_DOMAIN_AF_INET,
      ePAL_SOCK_TYPE_SOCK_STREAM, ePAL_SOCK_PROTOCOL_DEFAULT);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (NULL == *phl_mgr_sock_hdl))
   {
      RFSVP_S_LOG_LOW("pal_sock_create failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   if (0 != px_mgr->ui_ip_addr_ho)
   {
      ui_ip_addr_no = pal_htonl (px_mgr->ui_ip_addr_ho);
      RFSVP_S_LOG_LOW("Trying connection to host with IP: 0x%x:%d.",
         ui_ip_addr_no, px_mgr->us_port_ho);
   }
   else
   {
      RFSVP_S_LOG_LOW("IP not set: %d. Trying with DNS name address if set.",
         px_mgr->ui_ip_addr_ho);
      ui_dns_name_len = pal_strnlen(px_mgr->uca_dns_name_str,
         sizeof(px_mgr->uca_dns_name_str));
      if (ui_dns_name_len > 0)
      {
         e_pal_ret = pal_gethostbyname (px_mgr->uca_dns_name_str,
            &x_in_addr);
         if ((ePAL_RET_SUCCESS != e_pal_ret) || (0 == x_in_addr.ui_ip_addr_no))
         {
            RFSVP_S_LOG_LOW("pal_gethostbyname failed: %d", e_pal_ret);
            RFSVP_S_LOG_LOW("Invalid Args. Node Ip: %d not set.",
               px_mgr->ui_ip_addr_ho);
            e_error = eRFSVP_RET_INVALID_ARGS;
            goto CLEAN_RETURN;
         }
         else
         {
            ui_ip_addr_no = x_in_addr.ui_ip_addr_no;
            RFSVP_S_LOG_LOW("Trying connection to host with IP: 0x%x:%d after name "
               "(%s:%d) resolution.", ui_ip_addr_no, px_mgr->us_port_ho,
               px_mgr->uca_dns_name_str, px_mgr->us_port_ho);
         }
      }
      else
      {
         RFSVP_S_LOG_LOW("Invalid Args. Manager Ip: %d not set.",
            px_mgr->ui_ip_addr_ho);
         e_error = eRFSVP_RET_INVALID_ARGS;
         goto CLEAN_RETURN;
      }
   }

   x_local_sock_addr.us_sin_port_no = pal_htons (px_mgr->us_port_ho);
   x_local_sock_addr.x_sin_addr.ui_ip_addr_no = ui_ip_addr_no;
   e_pal_ret = pal_sock_connect(*phl_mgr_sock_hdl, &x_local_sock_addr,
      ePAL_SOCK_CONN_MODE_BLOCKING, 0);
   if (ePAL_RET_SUCCESS != e_pal_ret)
   {
      RFSVP_S_LOG_LOW("pal_sock_connect to node mgr failed: %d", e_pal_ret);
      pal_sock_close(*phl_mgr_sock_hdl);
      *phl_mgr_sock_hdl = NULL;
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }
   else
   {
      RFSVP_S_LOG_LOW("Connect to node mgr success. Registering the socket.");

      e_error = rfsvp_server_node_register_sock (px_rfsvp_server_ctxt,
         *phl_mgr_sock_hdl);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_S_LOG_LOW("rfsvp_server_node_register_sock failed: %d", e_error);
         e_pal_ret = pal_sock_close(*phl_mgr_sock_hdl);
         if (ePAL_RET_SUCCESS != e_pal_ret)
         {
            RFSVP_S_LOG_LOW("pal_sock_close failed: %d", e_pal_ret);
         }
         *phl_mgr_sock_hdl = NULL;
      }
      else
      {
         e_error = eRFSVP_RET_SUCCESS;
      }
   }

CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_register_sock (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   PAL_SOCK_HDL    hl_sock_hdl)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   SOCKMON_RET_E e_sockmon_ret = eSOCKMON_RET_FAILURE;
   SOCKMON_REGISTER_DATA_X x_register_data = {NULL};

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == hl_sock_hdl))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_register_data.hl_sock_hdl = hl_sock_hdl;
   x_register_data.fn_active_sock_cbk =
         rfsvp_server_node_sockmon_active_sock_cbk;
   x_register_data.p_app_data = px_rfsvp_server_ctxt;
   e_sockmon_ret = sockmon_register_sock (
      px_rfsvp_server_ctxt->x_init_params.hl_sockmon_hdl,
      &x_register_data);
   if (eSOCKMON_RET_SUCCESS != e_sockmon_ret)
   {
      RFSVP_S_LOG_LOW("sockmon_register_sock failed: %d", e_sockmon_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      RFSVP_S_LOG_MED("sockmon_register_sock success");
      e_error = eRFSVP_RET_SUCCESS;
   }

CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_deregister_sock (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   PAL_SOCK_HDL    hl_sock_hdl)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   SOCKMON_RET_E e_sockmon_ret = eSOCKMON_RET_FAILURE;
   SOCKMON_REGISTER_DATA_X x_register_data = {NULL};

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == hl_sock_hdl))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_register_data.hl_sock_hdl = hl_sock_hdl;
   e_sockmon_ret = sockmon_deregister_sock (
      px_rfsvp_server_ctxt->x_init_params.hl_sockmon_hdl, &x_register_data);
   if (eSOCKMON_RET_SUCCESS != e_sockmon_ret)
   {
      RFSVP_S_LOG_LOW("sockmon_deregister_sock failed: %d", e_sockmon_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      RFSVP_S_LOG_MED("sockmon_register_sock success");
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_cleanup_socks (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   uint32_t ui_index = 0;
   PAL_RET_E e_pal_ret = ePAL_RET_SUCCESS;

   if (NULL == px_rfsvp_server_ctxt)
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

      RFSVP_S_LOG_LOW("Closing socket of node %d", ui_index);
      e_pal_ret = udp_sock_delete (
         px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl);
      if (ePAL_RET_SUCCESS != e_pal_ret)
      {
         RFSVP_S_LOG_LOW("pal_sock_close failed: %d", e_pal_ret);
      }
      px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl = NULL;
   }

   for (ui_index = 0; ui_index < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes;
         ui_index++)
   {
      if (NULL != px_rfsvp_server_ctxt->x_server_nodes.hla_tcp_sock_hdl [ui_index])
      {
         RFSVP_S_LOG_LOW("Deregistering socket of node %d", ui_index);

         e_error = rfsvp_server_node_deregister_sock (px_rfsvp_server_ctxt,
            px_rfsvp_server_ctxt->x_server_nodes.hla_tcp_sock_hdl [ui_index]);
         if (eRFSVP_RET_SUCCESS != e_error)
         {
            RFSVP_S_LOG_LOW( "rfsvp_server_node_deregister_sock failed: %d",
               e_error);
         }

         RFSVP_S_LOG_LOW("Closing socket of node %d", ui_index);
         e_pal_ret = pal_sock_close (
            px_rfsvp_server_ctxt->x_server_nodes.hla_tcp_sock_hdl [ui_index]);
         if (ePAL_RET_SUCCESS != e_pal_ret)
         {
            RFSVP_S_LOG_LOW("pal_sock_close failed: %d", e_pal_ret);
         }
         px_rfsvp_server_ctxt->x_server_nodes.hla_tcp_sock_hdl [ui_index] = NULL;
      }
   }

   if (NULL != px_rfsvp_server_ctxt->hl_mgr_sock_hdl)
   {
      RFSVP_S_LOG_LOW("Deregistering TCP socket of node");
      e_error = rfsvp_server_node_deregister_sock (px_rfsvp_server_ctxt,
         px_rfsvp_server_ctxt->hl_mgr_sock_hdl);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_S_LOG_LOW("rfsvp_server_node_deregister_sock failed: %d",
            e_error);
      }

      e_pal_ret = pal_sock_close (px_rfsvp_server_ctxt->hl_mgr_sock_hdl);
      if (ePAL_RET_SUCCESS != e_pal_ret)
      {
         RFSVP_S_LOG_LOW("pal_sock_close failed: %d", e_pal_ret);
      }
      px_rfsvp_server_ctxt->hl_mgr_sock_hdl = NULL;
   }

   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_read_msg (
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
      RFSVP_S_LOG_LOW("Invalid Args");
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
      0, RFSVP_SERVER_NODE_READ_MSG_TIMEOUT_MS);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (ui_recv_len != ui_hdr_len))
   {
      if (ePAL_RET_SOCK_CLOSED == e_pal_ret)
      {
         RFSVP_S_LOG_LOW("pal_sock_recv_fixed failed. Connection closed by peer.",
            RFSVP_SERVER_NODE_READ_MSG_TIMEOUT_MS);
      }
      else if (ePAL_RET_OPERATION_TIMEDOUT == e_pal_ret)
      {
         RFSVP_S_LOG_LOW("pal_sock_recv_fixed timedout after %d ms: %d",
            RFSVP_SERVER_NODE_READ_MSG_TIMEOUT_MS);
      }
      else
      {
         RFSVP_S_LOG_LOW("pal_sock_recv_fixed for header failed: 0x%x, "
            "Header len: %d, Received: %d", e_pal_ret, ui_hdr_len, ui_recv_len);
      }
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   RFSVP_S_LOG_HIGH("Read header of %d bytes.", ui_recv_len);

   px_msg_hdr = (NODE_MSG_HDR_X *) puc_msg_buf;

   ui_recv_len = px_msg_hdr->ui_msg_pay_len;
   e_pal_ret = pal_sock_recv_fixed (hl_sock_hdl, (puc_msg_buf + ui_hdr_len),
      &ui_recv_len, 0, RFSVP_SERVER_NODE_READ_MSG_TIMEOUT_MS);
   if ((ePAL_RET_SUCCESS != e_pal_ret)
      || (ui_recv_len != px_msg_hdr->ui_msg_pay_len))
   {
      if (ePAL_RET_SOCK_CLOSED == e_pal_ret)
      {
         RFSVP_S_LOG_LOW("pal_sock_recv_fixed failed. Connection closed by peer.",
            RFSVP_SERVER_NODE_READ_MSG_TIMEOUT_MS);
      }
      else if (ePAL_RET_OPERATION_TIMEDOUT == e_pal_ret)
      {
         RFSVP_S_LOG_LOW("pal_sock_recv_fixed timedout after %d ms: %d",
            RFSVP_SERVER_NODE_READ_MSG_TIMEOUT_MS);
      }
      else
      {
         RFSVP_S_LOG_LOW("pal_sock_recv_fixed for payload failed: %d, "
         "Header len: %d, Received: %d",
            e_pal_ret, px_msg_hdr->ui_msg_pay_len, ui_recv_len);
      }
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      RFSVP_S_LOG_HIGH("Read payload of %d bytes.", ui_recv_len);
      *pui_msg_size = ui_hdr_len + px_msg_hdr->ui_msg_pay_len;
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_send_msg (
   PAL_SOCK_HDL hl_sock_hdl,
   NODE_MSG_HDR_X *px_msg_hdr)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_send_len = 0;

   if ((NULL == hl_sock_hdl) || (NULL == px_msg_hdr))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   ui_send_len = sizeof(*px_msg_hdr) + px_msg_hdr->ui_msg_pay_len;
   e_pal_ret = pal_sock_send_fixed (hl_sock_hdl, (uint8_t *) px_msg_hdr,
      &ui_send_len, 0, RFSVP_SERVER_SEND_MSG_TIMEOUT_MS);
   if (ePAL_RET_SUCCESS != e_pal_ret)
   {
      RFSVP_S_LOG_LOW("pal_sock_send_fixed failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      RFSVP_S_LOG_LOW("pal_sock_send_fixed success: %d bytes", ui_send_len);
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

void rfsvp_server_node_log_status(
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
#if 0
   uint32_t ui_index = 0;
   RFSVP_SERVER_NODE_CTXT_X *px_node_ctxt = NULL;
   PAL_SOCK_HDL *phl_sock_hdl = NULL;

   if (NULL != px_rfsvp_server_ctxt)
   {
      printf ("\n\n**********************************************************"
         "**********************************************************");
      printf ("\n**********************************************************"
         "**********************************************************");
      printf ("\n%6s | %32s | %10s | %5s | %10s\n",
         "ID", "DNS Name", "IP Address", "Port", "Connection");
      printf ("__________________________________________________________"
         "__________________________________________________________\n");
      for (ui_index = 0; ui_index < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes;
            ui_index++)
      {
         px_node_ctxt = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes[ui_index]);
         phl_sock_hdl = &(px_rfsvp_server_ctxt->x_server_nodes.hla_tcp_sock_hdl[ui_index]);
         if (0 == ui_index)
         {
            printf ("%6s", "Leader");
         }
         else
         {
            printf ("%6s", "Other");
         }
         printf (" | %32s | 0x%8x | %5d | %10s\n",
            px_node_ctxt->uca_dns_name_str,
            px_node_ctxt->ui_ip_addr_ho,
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

bool rfsvp_server_check_all_nodes_have_joined (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   bool b_joined = false;
   uint32_t ui_index = 0;
   RFSVP_SERVER_NODE_CTXT_X *px_node_ctxt = NULL;

   if (NULL != px_rfsvp_server_ctxt)
   {
      for (ui_index = 0; ui_index < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes;
            ui_index++)
      {
         px_node_ctxt = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes [ui_index]);
         if (eRFSVP_SERVER_NODE_STATE_JOINED != px_node_ctxt->e_state)
         {
            break;
         }
      }

      if (ui_index == px_rfsvp_server_ctxt->x_init_params.ui_no_nodes)
      {
         b_joined = true;
      }
   }
   else
   {
      RFSVP_S_LOG_LOW("Invalid Args");
   }
   return b_joined;
}

RFSVP_RET_E rfsvp_server_node_get_active_tcp_sock_index (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   PAL_SOCK_HDL *phl_act_sock_hdl,
   uint32_t *pui_index)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   uint32_t ui_index = 0;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == phl_act_sock_hdl)
      || (NULL == pui_index) || (NULL == *phl_act_sock_hdl))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   for (ui_index = 0; ui_index < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes;
         ui_index++)
   {
      if (px_rfsvp_server_ctxt->x_server_nodes.hla_tcp_sock_hdl [ui_index]
         == *phl_act_sock_hdl)
      {
         break;
      }
   }

   if (ui_index < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes)
   {
      *pui_index = ui_index;
      e_error = eRFSVP_RET_SUCCESS;
   }
   else
   {
      RFSVP_S_LOG_LOW("*********************************************************");
      RFSVP_S_LOG_LOW("*************Fatal Error: Socket IDs Mismatch************");
      RFSVP_S_LOG_LOW("*********************************************************");
      e_error = eRFSVP_RET_INVALID_ARGS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_node_get_active_udp_sock_index (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   NODE_MSG_HDR_X *px_msg_header,
   PAL_SOCK_ADDR_IN_X *px_src_sock_addr_in,
   uint32_t *pui_index)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_PING_X *px_ping = NULL;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_src_sock_addr_in)
      || (NULL == pui_index))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_ping = (NODE_MSG_PING_X *) px_msg_header;

#if 0
   ui_src_ip_ho = pal_ntohl (px_src_sock_addr_in->x_sin_addr.ui_ip_addr_no);

   for (ui_index = 0; ui_index < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes;
         ui_index++)
   {
      if (px_rfsvp_server_ctxt->x_server_nodes.xa_nodes [ui_index].ui_ip_addr_ho
         == ui_src_ip_ho)
      {
         break;
      }
   }

   if (ui_index < px_rfsvp_server_ctxt->x_init_params.ui_no_nodes)
   {
      *pui_index = ui_index;
      e_error = eRFSVP_RET_SUCCESS;
   }
   else
   {
      RFSVP_S_LOG_LOW("*********************************************************");
      RFSVP_S_LOG_LOW("*************Fatal Error: Socket IDs Mismatch************");
      RFSVP_S_LOG_LOW("*********************************************************");
      e_error = eRFSVP_RET_INVALID_ARGS;
   }
#else
   *pui_index = px_ping->ui_node_idx;
   e_error = eRFSVP_RET_SUCCESS;
#endif
CLEAN_RETURN:
   return e_error;
}
