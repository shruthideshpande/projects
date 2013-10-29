/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   dimutex_node_utils.c
 * \author sandeepprakash
 *
 * \date   29-Sep-2012
 *
 * \brief
 *
 ******************************************************************************/

/********************************** INCLUDES **********************************/
#include "ch-pal/exp_pal.h"
#include "ch-utils/exp_msgq.h"
#include "ch-utils/exp_task.h"
#include "ch-sockmon/exp_sockmon.h"
#include "dimutex.h"
#include "dimutex_node.h"
#include "dimutex_algo_stats.h"
#include "dimutex_private.h"
#include "dimutex_algo.h"
#include "dimutex_node_comm.h"
#include "dimutex_node_task.h"
#include "dimutex_node_utils.h"
#include "dimutex_send_msg.h"

/********************************* CONSTANTS **********************************/

/*********************************** MACROS ***********************************/
#define DIMUTEX_NODE_READ_MSG_TIMEOUT_MS              (5000)

/******************************** ENUMERATIONS ********************************/

/************************* STRUCTURE/UNION DATA TYPES *************************/

/************************ STATIC FUNCTION PROTOTYPES **************************/
static DIMUTEX_RET_E dimutex_node_connect_to_node (
   DIMUTEX_CTXT_X *px_dimutex_ctxt,
   uint32_t ui_index);

static DIMUTEX_RET_E dimutex_node_summarize_stats (
   DIMUTEX_CTXT_X *px_dimutex_ctxt);

/****************************** LOCAL FUNCTIONS *******************************/
DIMUTEX_RET_E dimutex_node_establish_conn_to_node (
   DIMUTEX_CTXT_X *px_dimutex_ctxt,
   uint32_t ui_index)
{
   DIMUTEX_RET_E e_error = eDIMUTEX_RET_FAILURE;

   if (NULL == px_dimutex_ctxt)
   {
      DMUT_LOG_LOW("Invalid Args");
      e_error = eDIMUTEX_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   if (ui_index > px_dimutex_ctxt->x_init_params.ui_no_nodes)
   {
      DMUT_LOG_LOW("Invalid Args");
      e_error = eDIMUTEX_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   e_error = dimutex_node_connect_to_node (px_dimutex_ctxt, ui_index);
   if (eDIMUTEX_RET_SUCCESS != e_error)
   {
      DMUT_LOG_LOW("dimutex_node_connect_to_node failed: %d", e_error);
   }
   else
   {
      e_error = dimutex_node_send_join_to_node (px_dimutex_ctxt, ui_index);
      if (eDIMUTEX_RET_SUCCESS != e_error)
      {
         DMUT_LOG_LOW("dimutex_node_send_credentials_to_node failed: %d",
            e_error);
      }
   }
CLEAN_RETURN:
   return e_error;
}

static DIMUTEX_RET_E dimutex_node_connect_to_node (
   DIMUTEX_CTXT_X *px_dimutex_ctxt,
   uint32_t ui_index)
{
   DIMUTEX_RET_E e_error = eDIMUTEX_RET_FAILURE;
   PAL_SOCK_HDL *phl_node_sock_hdl = NULL;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   PAL_SOCK_ADDR_IN_X x_local_sock_addr = {0};
   PAL_SOCK_IN_ADDR_X x_in_addr = {0};
   DIMUTEX_NODE_CTXT_X *px_node = NULL;
   uint32_t ui_dns_name_len = 0;
   uint32_t ui_ip_addr_no = 0;

   if (NULL == px_dimutex_ctxt)
   {
      DMUT_LOG_LOW("Invalid Args");
      e_error = eDIMUTEX_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   if (ui_index > px_dimutex_ctxt->x_init_params.ui_no_nodes)
   {
      DMUT_LOG_LOW("Invalid Args");
      e_error = eDIMUTEX_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_node = &(px_dimutex_ctxt->x_nodes.xa_nodes[ui_index]);
   phl_node_sock_hdl = &(px_dimutex_ctxt->x_nodes.hla_node_sock[ui_index]);

   if (0 == px_node->us_node_listen_port_ho)
   {
      DMUT_LOG_LOW("Invalid Args. Leader port: %d/Name: %s not set.",
         px_node->us_node_listen_port_ho, px_node->uca_node_dns_name_str);
      e_error = eDIMUTEX_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   e_pal_ret = pal_sock_create(phl_node_sock_hdl, ePAL_SOCK_DOMAIN_AF_INET,
      ePAL_SOCK_TYPE_SOCK_STREAM, ePAL_SOCK_PROTOCOL_DEFAULT);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (NULL == *phl_node_sock_hdl))
   {
      DMUT_LOG_LOW("pal_sock_create failed: %d", e_pal_ret);
      e_error = eDIMUTEX_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   ui_dns_name_len = pal_strnlen(px_node->uca_node_dns_name_str,
      sizeof(px_node->uca_node_dns_name_str));
   if (ui_dns_name_len > 0)
   {
      e_pal_ret = pal_gethostbyname (px_node->uca_node_dns_name_str,
         &x_in_addr);
      if ((ePAL_RET_SUCCESS != e_pal_ret) || (0 == x_in_addr.ui_ip_addr_no))
      {
         DMUT_LOG_LOW("pal_gethostbyname failed: %d", e_pal_ret);
         DMUT_LOG_LOW("Node Hostname : %s malformed/not reachable. "
         "Trying with IP address if set.", px_node->uca_node_dns_name_str);
         if (0 == px_node->ui_node_ip_addr_ho)
         {
            DMUT_LOG_LOW("Invalid Args. Node Ip: %d not set.",
               px_node->ui_node_ip_addr_ho);
            e_error = eDIMUTEX_RET_INVALID_ARGS;
            goto CLEAN_RETURN;
         }
         else
         {
            ui_ip_addr_no = pal_htonl (px_node->ui_node_ip_addr_ho);
            DMUT_LOG_LOW("Trying connection to host with IP: %d.",
               px_node->uca_node_dns_name_str, ui_ip_addr_no);
         }
      }
      else
      {
         ui_ip_addr_no = x_in_addr.ui_ip_addr_no;
         DMUT_LOG_LOW("Trying connection to \"%s\" with IP: %d.",
            px_node->uca_node_dns_name_str, ui_ip_addr_no);
      }
   }
   else
   {
      DMUT_LOG_LOW("Node Hostname : %s not set. Trying with IP address.",
         px_node->uca_node_dns_name_str);
      if (0 == px_node->ui_node_ip_addr_ho)
      {
         DMUT_LOG_LOW("Invalid Args. Node Ip: %d not set.",
            px_node->ui_node_ip_addr_ho);
         e_error = eDIMUTEX_RET_INVALID_ARGS;
         goto CLEAN_RETURN;
      }
      else
      {
         ui_ip_addr_no = pal_htonl (px_node->ui_node_ip_addr_ho);
         DMUT_LOG_LOW("Trying connection to host with IP: %d.",
            px_node->uca_node_dns_name_str, ui_ip_addr_no);
      }
   }
   x_local_sock_addr.us_sin_port_no = pal_htons (
      px_node->us_node_listen_port_ho);
   x_local_sock_addr.x_sin_addr.ui_ip_addr_no = ui_ip_addr_no;
   e_pal_ret = pal_sock_connect(*phl_node_sock_hdl, &x_local_sock_addr,
      ePAL_SOCK_CONN_MODE_BLOCKING, 0);
   if (ePAL_RET_SUCCESS != e_pal_ret)
   {
      DMUT_LOG_LOW("pal_sock_connect to node %d failed: %d", ui_index,
         e_pal_ret);
      pal_sock_close(*phl_node_sock_hdl);
      *phl_node_sock_hdl = NULL;
      e_error = eDIMUTEX_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }
   else
   {
      DMUT_LOG_LOW("Connect to node %d success. Registering the socket.",
         ui_index);

      e_error = dimutex_node_register_sock (px_dimutex_ctxt,
         *phl_node_sock_hdl);
      if (eDIMUTEX_RET_SUCCESS != e_error)
      {
         DMUT_LOG_LOW("dimutex_node_register_sock failed: %d", e_error);
         e_pal_ret = pal_sock_close(*phl_node_sock_hdl);
         if (ePAL_RET_SUCCESS != e_pal_ret)
         {
            DMUT_LOG_LOW("pal_sock_close failed: %d", e_pal_ret);
         }
         *phl_node_sock_hdl = NULL;
      }
      else
      {
         e_error = eDIMUTEX_RET_SUCCESS;
      }
   }

CLEAN_RETURN:
   return e_error;
}

DIMUTEX_RET_E dimutex_node_register_sock (
   DIMUTEX_CTXT_X *px_dimutex_ctxt,
   PAL_SOCK_HDL    hl_sock_hdl)
{
   DIMUTEX_RET_E e_error = eDIMUTEX_RET_FAILURE;
   SOCKMON_RET_E e_sockmon_ret = eSOCKMON_RET_FAILURE;
   SOCKMON_REGISTER_DATA_X x_register_data = {NULL};

   if ((NULL == px_dimutex_ctxt) || (NULL == hl_sock_hdl))
   {
      DMUT_LOG_LOW("Invalid Args");
      e_error = eDIMUTEX_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_register_data.hl_sock_hdl = hl_sock_hdl;
   x_register_data.fn_active_sock_cbk =
         dimutex_node_sockmon_active_sock_cbk;
   x_register_data.p_app_data = px_dimutex_ctxt;
   e_sockmon_ret = sockmon_register_sock (
      px_dimutex_ctxt->x_init_params.hl_sockmon_hdl,
      &x_register_data);
   if (eSOCKMON_RET_SUCCESS != e_sockmon_ret)
   {
      DMUT_LOG_LOW("sockmon_register_sock failed: %d", e_sockmon_ret);
      e_error = eDIMUTEX_RET_RESOURCE_FAILURE;
   }
   else
   {
      DMUT_LOG_MED("sockmon_register_sock success");
      e_error = eDIMUTEX_RET_SUCCESS;
   }

CLEAN_RETURN:
   return e_error;
}

DIMUTEX_RET_E dimutex_node_deregister_sock (
   DIMUTEX_CTXT_X *px_dimutex_ctxt,
   PAL_SOCK_HDL    hl_sock_hdl)
{
   DIMUTEX_RET_E e_error = eDIMUTEX_RET_FAILURE;
   SOCKMON_RET_E e_sockmon_ret = eSOCKMON_RET_FAILURE;
   SOCKMON_REGISTER_DATA_X x_register_data = {NULL};

   if ((NULL == px_dimutex_ctxt) || (NULL == hl_sock_hdl))
   {
      DMUT_LOG_LOW("Invalid Args");
      e_error = eDIMUTEX_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_register_data.hl_sock_hdl = hl_sock_hdl;
   e_sockmon_ret = sockmon_deregister_sock (
      px_dimutex_ctxt->x_init_params.hl_sockmon_hdl, &x_register_data);
   if (eSOCKMON_RET_SUCCESS != e_sockmon_ret)
   {
      DMUT_LOG_LOW("sockmon_deregister_sock failed: %d", e_sockmon_ret);
      e_error = eDIMUTEX_RET_RESOURCE_FAILURE;
   }
   else
   {
      DMUT_LOG_MED("sockmon_register_sock success");
      e_error = eDIMUTEX_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

DIMUTEX_RET_E dimutex_node_cleanup_socks (
   DIMUTEX_CTXT_X *px_dimutex_ctxt)
{
   DIMUTEX_RET_E e_error = eDIMUTEX_RET_FAILURE;
   uint32_t ui_index = 0;
   SOCKMON_RET_E e_sockmon_ret = eSOCKMON_RET_FAILURE;
   SOCKMON_REGISTER_DATA_X x_register_data = {NULL};
   PAL_RET_E e_pal_ret = ePAL_RET_SUCCESS;

   if (NULL == px_dimutex_ctxt)
   {
      DMUT_LOG_LOW("Invalid Args");
      e_error = eDIMUTEX_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   for (ui_index = 0; ui_index < px_dimutex_ctxt->x_init_params.ui_no_nodes;
         ui_index++)
   {
      if (NULL != px_dimutex_ctxt->x_nodes.hla_node_sock [ui_index])
      {
         DMUT_LOG_LOW("Deregistering socket of node %d", ui_index);
         x_register_data.hl_sock_hdl =
            px_dimutex_ctxt->x_nodes.hla_node_sock [ui_index];
         e_sockmon_ret = sockmon_deregister_sock (
            px_dimutex_ctxt->x_init_params.hl_sockmon_hdl, &x_register_data);
         if (eSOCKMON_RET_SUCCESS != e_sockmon_ret)
         {
            DMUT_LOG_LOW("sockmon_deregister_sock failed: %d", e_sockmon_ret);
         }
         DMUT_LOG_LOW("Closing socket of node %d", ui_index);
         e_pal_ret = pal_sock_close (
            px_dimutex_ctxt->x_nodes.hla_node_sock [ui_index]);
         if (ePAL_RET_SUCCESS != e_pal_ret)
         {
            DMUT_LOG_LOW("pal_sock_close failed: %d", e_pal_ret);
         }
         px_dimutex_ctxt->x_nodes.hla_node_sock [ui_index] = NULL;
      }
   }
   e_error = eDIMUTEX_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

DIMUTEX_RET_E dimutex_node_read_msg (
   PAL_SOCK_HDL hl_sock_hdl,
   uint8_t *puc_msg_buf,
   uint32_t ui_msg_buf_len,
   uint32_t *pui_msg_size)
{
   DIMUTEX_RET_E e_error = eDIMUTEX_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_recv_len = 0;
   uint32_t ui_hdr_len = 0;
   NODE_MSG_HDR_X *px_msg_hdr = NULL;

   if ((NULL == hl_sock_hdl) || (NULL == puc_msg_buf) || (0 == ui_msg_buf_len)
      || (NULL == pui_msg_size) || (ui_msg_buf_len < sizeof(NODE_MSG_HDR_X)))
   {
      DMUT_LOG_LOW("Invalid Args");
      e_error = eDIMUTEX_RET_INVALID_ARGS;
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
      0, DIMUTEX_NODE_READ_MSG_TIMEOUT_MS);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (ui_recv_len != ui_hdr_len))
   {
      if (ePAL_RET_SOCK_CLOSED == e_pal_ret)
      {
         DMUT_LOG_LOW("pal_sock_recv_fixed failed. Connection closed by peer.",
            DIMUTEX_NODE_READ_MSG_TIMEOUT_MS);
      }
      else if (ePAL_RET_OPERATION_TIMEDOUT == e_pal_ret)
      {
         DMUT_LOG_LOW("pal_sock_recv_fixed timedout after %d ms: %d",
            DIMUTEX_NODE_READ_MSG_TIMEOUT_MS);
      }
      else
      {
         DMUT_LOG_LOW("pal_sock_recv_fixed for header failed: 0x%x, "
            "Header len: %d, Received: %d", e_pal_ret, ui_hdr_len, ui_recv_len);
      }
      e_error = eDIMUTEX_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   DMUT_LOG_HIGH("Read header of %d bytes.", ui_recv_len);

   px_msg_hdr = (NODE_MSG_HDR_X *) puc_msg_buf;

   ui_recv_len = px_msg_hdr->ui_msg_pay_len;
   e_pal_ret = pal_sock_recv_fixed (hl_sock_hdl, (puc_msg_buf + ui_hdr_len),
      &ui_recv_len, 0, DIMUTEX_NODE_READ_MSG_TIMEOUT_MS);
   if ((ePAL_RET_SUCCESS != e_pal_ret)
      || (ui_recv_len != px_msg_hdr->ui_msg_pay_len))
   {
      if (ePAL_RET_SOCK_CLOSED == e_pal_ret)
      {
         DMUT_LOG_LOW("pal_sock_recv_fixed failed. Connection closed by peer.",
            DIMUTEX_NODE_READ_MSG_TIMEOUT_MS);
      }
      else if (ePAL_RET_OPERATION_TIMEDOUT == e_pal_ret)
      {
         DMUT_LOG_LOW("pal_sock_recv_fixed timedout after %d ms: %d",
            DIMUTEX_NODE_READ_MSG_TIMEOUT_MS);
      }
      else
      {
         DMUT_LOG_LOW("pal_sock_recv_fixed for payload failed: %d, "
         "Header len: %d, Received: %d",
            e_pal_ret, px_msg_hdr->ui_msg_pay_len, ui_recv_len);
      }
      e_error = eDIMUTEX_RET_RESOURCE_FAILURE;
   }
   else
   {
      DMUT_LOG_HIGH("Read payload of %d bytes.", ui_recv_len);
      *pui_msg_size = ui_hdr_len + px_msg_hdr->ui_msg_pay_len;
      e_error = eDIMUTEX_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

void dimutex_node_log_status(
   DIMUTEX_CTXT_X *px_dimutex_ctxt)
{
#if 0
   uint32_t ui_index = 0;
   DIMUTEX_NODE_CTXT_X *px_node_ctxt = NULL;
   PAL_SOCK_HDL *phl_sock_hdl = NULL;

   if (NULL != px_dimutex_ctxt)
   {
      printf ("\n\n**********************************************************"
         "**********************************************************");
      printf ("\n**********************************************************"
         "**********************************************************");
      printf ("\n%6s | %32s | %10s | %5s | %10s\n",
         "ID", "DNS Name", "IP Address", "Port", "Connection");
      printf ("__________________________________________________________"
         "__________________________________________________________\n");
      for (ui_index = 0; ui_index < px_dimutex_ctxt->x_init_params.ui_no_nodes;
            ui_index++)
      {
         px_node_ctxt = &(px_dimutex_ctxt->x_nodes.xa_nodes[ui_index]);
         phl_sock_hdl = &(px_dimutex_ctxt->x_nodes.hla_node_sock[ui_index]);
         if (0 == ui_index)
         {
            printf ("%6s", "Leader");
         }
         else if (px_dimutex_ctxt->x_nodes.ui_this_node_idx == ui_index)
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

bool dimutex_check_all_nodes_have_joined (
   DIMUTEX_CTXT_X *px_dimutex_ctxt)
{
   bool b_joined = false;
   uint32_t ui_index = 0;
   DIMUTEX_NODE_CTXT_X *px_node_ctxt = NULL;

   if (NULL != px_dimutex_ctxt)
   {
      for (ui_index = 0; ui_index < px_dimutex_ctxt->x_init_params.ui_no_nodes;
            ui_index++)
      {
         if (px_dimutex_ctxt->x_nodes.ui_this_node_idx == ui_index)
         {
            continue;
         }
         px_node_ctxt = &(px_dimutex_ctxt->x_nodes.xa_nodes [ui_index]);
         if (eDIMUTEX_NODE_STATE_JOINED != px_node_ctxt->e_state)
         {
            break;
         }
      }

      if (ui_index == px_dimutex_ctxt->x_init_params.ui_no_nodes)
      {
         b_joined = true;
      }
   }
   else
   {
      DMUT_LOG_LOW("Invalid Args");
   }
   return b_joined;
}

DIMUTEX_RET_E dimutex_node_get_active_sock_index (
   DIMUTEX_CTXT_X *px_dimutex_ctxt,
   PAL_SOCK_HDL *phl_act_sock_hdl,
   uint32_t *pui_index)
{
   DIMUTEX_RET_E e_error = eDIMUTEX_RET_FAILURE;
   uint32_t ui_index = 0;

   if ((NULL == px_dimutex_ctxt) || (NULL == phl_act_sock_hdl)
      || (NULL == pui_index) || (NULL == *phl_act_sock_hdl))
   {
      DMUT_LOG_LOW("Invalid Args");
      e_error = eDIMUTEX_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   for (ui_index = 0; ui_index < px_dimutex_ctxt->x_init_params.ui_no_nodes;
         ui_index++)
   {
      if (px_dimutex_ctxt->x_nodes.hla_node_sock [ui_index]
         == *phl_act_sock_hdl)
      {
         break;
      }
   }

   if (ui_index < px_dimutex_ctxt->x_init_params.ui_no_nodes)
   {
      *pui_index = ui_index;
      e_error = eDIMUTEX_RET_SUCCESS;
   }
   else
   {
      DMUT_LOG_LOW("*********************************************************");
      DMUT_LOG_LOW("*************Fatal Error: Socket IDs Mismatch************");
      DMUT_LOG_LOW("*********************************************************");
      e_error = eDIMUTEX_RET_INVALID_ARGS;
   }
CLEAN_RETURN:
   return e_error;
}

DIMUTEX_RET_E dimutex_node_aggregate_stats (
   DIMUTEX_CTXT_X *px_dimutex_ctxt,
   uint32_t ui_node_idx,
   DIMUTEX_ALGO_STATS_X *px_stats)
{
   DIMUTEX_RET_E e_error = eDIMUTEX_RET_FAILURE;
   uint32_t ui_index = 0;
   DIMUTEX_ALGO_CS_ENTRY_STAT_X *px_stat_entry = NULL;
   DIMUTEX_NODE_CTXT_X *px_node_ctxt = NULL;
   uint32_t ui_first_20_total = 0;
   uint32_t ui_last_20_total = 0;
   uint32_t ui_req_grant_diff = 0;
   DIMUTEX_ALGO_STATS_X *px_stats_temp = NULL;

   if ((NULL == px_dimutex_ctxt) || (NULL == px_stats)
      || (ui_node_idx >= px_dimutex_ctxt->x_init_params.ui_no_nodes))
   {
      DMUT_LOG_LOW("Invalid Args");
      e_error = eDIMUTEX_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_stats_temp = &(px_dimutex_ctxt->x_stats.xa_algo_stats [ui_node_idx]);

   (void) pal_memmove (px_stats_temp, px_stats, sizeof(*px_stats_temp));

   for (ui_index = 0; ui_index < px_stats->ui_count; ui_index++)
   {
      px_stat_entry = &(px_stats->xa_cs_entry_stat [ui_index]);
      printf ("[STATS]Node %d: CS Entry: Current Count: %d, Req @: %d ms, "
         "Granted @: %d ms, Req Sent: %d, Reply Recvd: %d\n", ui_node_idx,
         (ui_index + 1), px_stat_entry->ui_cs_req_time_ms,
         px_stat_entry->ui_cs_grant_time_ms,
         px_stat_entry->ui_no_request_msgs_sent,
         px_stat_entry->ui_no_reply_msgs_received);
      DMUT_LOG_LOW("[STATS]Node %d: CS Entry: Current Count: %d, Req @: %d ms, "
      "Granted @: %d ms, Req Sent: %d, Reply Recvd: %d",
         ui_node_idx, (ui_index + 1), px_stat_entry->ui_cs_req_time_ms,
         px_stat_entry->ui_cs_grant_time_ms,
         px_stat_entry->ui_no_request_msgs_sent,
         px_stat_entry->ui_no_reply_msgs_received);

      if (0 == px_stats_temp->ui_min_msgs)
      {
         px_stats_temp->ui_min_msgs = (px_stat_entry->ui_no_request_msgs_sent
            + px_stat_entry->ui_no_reply_msgs_received);
      }

      if ((px_stat_entry->ui_no_request_msgs_sent
         + px_stat_entry->ui_no_reply_msgs_received)
         < px_stats_temp->ui_min_msgs)
      {
         px_stats_temp->ui_min_msgs = (px_stat_entry->ui_no_request_msgs_sent
            + px_stat_entry->ui_no_reply_msgs_received);
      }

      if ((px_stat_entry->ui_no_request_msgs_sent
         + px_stat_entry->ui_no_reply_msgs_received)
         > px_stats_temp->ui_max_msgs)
      {
         px_stats_temp->ui_max_msgs = (px_stat_entry->ui_no_request_msgs_sent
            + px_stat_entry->ui_no_reply_msgs_received);
      }

      ui_req_grant_diff = px_stat_entry->ui_cs_grant_time_ms
         - px_stat_entry->ui_cs_req_time_ms;

      if (ui_index < DIMUTEX_ALGO_CS_REQUEST_RATE_CHANGE_AFTER)
      {
         ui_first_20_total += ui_req_grant_diff;
      }
      else
      {
         ui_last_20_total += ui_req_grant_diff;
      }
   }

   px_stats_temp->ui_first_20_avg = ui_first_20_total
      / DIMUTEX_ALGO_CS_REQUEST_RATE_CHANGE_AFTER;
   px_stats_temp->ui_last_20_avg = ui_last_20_total
      / (px_stats_temp->ui_count - DIMUTEX_ALGO_CS_REQUEST_RATE_CHANGE_AFTER);
   px_stats_temp->ui_full_avg = (ui_first_20_total + ui_last_20_total)
      / px_stats_temp->ui_count;

   px_node_ctxt = &(px_dimutex_ctxt->x_nodes.xa_nodes[ui_node_idx]);
   DMUT_LOG_LOW("Setting node %d state to eDIMUTEX_NODE_STATE_ALGO_COMPLETE",
      ui_node_idx);
   px_node_ctxt->e_state = eDIMUTEX_NODE_STATE_ALGO_COMPLETE;

   for (ui_index = 0; ui_index < px_dimutex_ctxt->x_init_params.ui_no_nodes;
         ui_index++)
   {
      if (eDIMUTEX_NODE_STATE_ALGO_COMPLETE !=
            px_dimutex_ctxt->x_nodes.xa_nodes[ui_index].e_state)
      {
         break;
      }
   }

   if (ui_index == px_dimutex_ctxt->x_init_params.ui_no_nodes)
   {
      DMUT_LOG_LOW("All nodes have completed algorithm execution.");
      DMUT_LOG_LOW("Sending teardown to all nodes.");
      e_error = dimutex_node_send_algo_teardown_to_all (px_dimutex_ctxt);
      if (eDIMUTEX_RET_SUCCESS != e_error)
      {
         DMUT_LOG_LOW( "dimutex_node_send_algo_teardown_to_all failed: %d",
            e_error);
      }
      (void) dimutex_node_summarize_stats (px_dimutex_ctxt);
   }
   else
   {
      e_error = eDIMUTEX_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

static DIMUTEX_RET_E dimutex_node_summarize_stats (
   DIMUTEX_CTXT_X *px_dimutex_ctxt)
{
   DIMUTEX_RET_E e_error = eDIMUTEX_RET_FAILURE;
   uint32_t ui_index = 0;
   uint32_t ui_stats_index = 0;
   DIMUTEX_ALGO_STATS_X *px_node_stats = NULL;
   DIMUTEX_ALGO_CS_ENTRY_STAT_X *px_stat_entry = NULL;
   FILE *fp_stats_summary = NULL;

   uint32_t ui_odd_first_20_avg = 0;
   uint32_t ui_odd_last_20_avg = 0;
   uint32_t ui_odd_full_avg = 0;

   uint32_t ui_even_first_20_avg = 0;
   uint32_t ui_even_last_20_avg = 0;
   uint32_t ui_even_full_avg = 0;

   uint32_t ui_all_first_20_avg = 0;
   uint32_t ui_all_last_20_avg = 0;
   uint32_t ui_all_full_avg = 0;

   uint32_t ui_odd_first_20_total = 0;
   uint32_t ui_odd_last_20_total = 0;
   uint32_t ui_odd_full_total = 0;

   uint32_t ui_even_first_20_total = 0;
   uint32_t ui_even_last_20_total = 0;
   uint32_t ui_even_full_total = 0;

   uint32_t ui_all_first_20_total = 0;
   uint32_t ui_all_last_20_total = 0;
   uint32_t ui_all_full_total = 0;

   uint32_t ui_total_msgs_count = 0;

   if (NULL == px_dimutex_ctxt)
   {
      DMUT_LOG_LOW("Invalid Args");
      e_error = eDIMUTEX_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   fp_stats_summary = fopen ("dimutex_stats_summary.txt", "w+");
   if (NULL == fp_stats_summary)
   {
      DMUT_LOG_LOW("fopen failed");
      e_error = eDIMUTEX_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   printf ("Node:Request No:Requested:Granted:Elapsed:Sent:Received\n");
   fprintf (fp_stats_summary,
      "Node:Request No:Requested:Granted:Elapsed:Sent:Received\n");

   for (ui_index = 0; ui_index < px_dimutex_ctxt->x_init_params.ui_no_nodes;
         ui_index++)
   {
      px_node_stats = &(px_dimutex_ctxt->x_stats.xa_algo_stats [ui_index]);

      for (ui_stats_index = 0; ui_stats_index < px_node_stats->ui_count; ui_stats_index++)
      {
         px_stat_entry = &(px_node_stats->xa_cs_entry_stat [ui_stats_index]);
         printf ("%d:%d:%d:%d:%d:%d:%d\n", ui_index, ui_stats_index + 1,
            px_stat_entry->ui_cs_req_time_ms,
            px_stat_entry->ui_cs_grant_time_ms,
            (px_stat_entry->ui_cs_grant_time_ms
               - px_stat_entry->ui_cs_req_time_ms),
            px_stat_entry->ui_no_request_msgs_sent,
            px_stat_entry->ui_no_reply_msgs_received);

         fprintf (fp_stats_summary, "%d:%d:%d:%d:%d:%d:%d\n", ui_index,
            ui_stats_index + 1, px_stat_entry->ui_cs_req_time_ms,
            px_stat_entry->ui_cs_grant_time_ms,
            (px_stat_entry->ui_cs_grant_time_ms
               - px_stat_entry->ui_cs_req_time_ms),
            px_stat_entry->ui_no_request_msgs_sent,
            px_stat_entry->ui_no_reply_msgs_received);

         ui_total_msgs_count += px_stat_entry->ui_no_request_msgs_sent;
         ui_total_msgs_count += px_stat_entry->ui_no_request_msgs_sent;
      }
   }

   printf ("------------------------------------------------------------------"
      "------------------\n");
   printf ("| %5s | %12s | %12s | %12s | %12s | %12s |\n", "Node",
      "First 20 Avg", "Last 20 Avg", "Overall Avg", "Min Messages",
      "Max Messages");
   printf ("|-%5s-+-%12s-+-%12s-+-%12s-+-%12s-+-%12s-+\n", "-----",
      "------------", "------------", "------------", "------------",
      "------------");

   fprintf (fp_stats_summary,
      "------------------------------------------------------------------"
         "------------------\n");
   fprintf (fp_stats_summary, "| %5s | %12s | %12s | %12s | %12s | %12s |\n",
      "Node", "First 20 Avg", "Last 20 Avg", "Overall Avg", "Min Messages",
      "Max Messages");
   fprintf (fp_stats_summary, "|-%5s-+-%12s-+-%12s-+-%12s-+-%12s-+-%12s-+\n",
      "-----", "------------", "------------", "------------", "------------",
      "------------");
   for (ui_index = 0; ui_index < px_dimutex_ctxt->x_init_params.ui_no_nodes;
         ui_index++)
   {
      px_node_stats = &(px_dimutex_ctxt->x_stats.xa_algo_stats [ui_index]);

      printf ("| %5d | %12d | %12d | %12d | %12d | %12d |\n", ui_index,
         px_node_stats->ui_first_20_avg, px_node_stats->ui_last_20_avg,
         px_node_stats->ui_full_avg, px_node_stats->ui_min_msgs,
         px_node_stats->ui_max_msgs);

      fprintf (fp_stats_summary, "| %5d | %12d | %12d | %12d | %12d | %12d |\n",
         ui_index, px_node_stats->ui_first_20_avg,
         px_node_stats->ui_last_20_avg, px_node_stats->ui_full_avg,
         px_node_stats->ui_min_msgs, px_node_stats->ui_max_msgs);

      if (0 == (ui_index % 2))
      {
         ui_even_first_20_total += px_node_stats->ui_first_20_avg;
         ui_even_last_20_total += px_node_stats->ui_last_20_avg;
         ui_even_full_total += (px_node_stats->ui_first_20_avg
            + px_node_stats->ui_last_20_avg);
      }
      else
      {
         ui_odd_first_20_total += px_node_stats->ui_first_20_avg;
         ui_odd_last_20_total += px_node_stats->ui_last_20_avg;
         ui_odd_full_total += (px_node_stats->ui_first_20_avg
            + px_node_stats->ui_last_20_avg);
      }

      ui_all_first_20_total += px_node_stats->ui_first_20_avg;
      ui_all_last_20_total += px_node_stats->ui_last_20_avg;
      ui_all_full_total += (px_node_stats->ui_first_20_avg
         + px_node_stats->ui_last_20_avg);
   }
   printf ("------------------------------------------------------------------"
      "------------------\n");
   fprintf (fp_stats_summary,
      "------------------------------------------------------------------"
         "------------------\n");

   ui_even_first_20_avg = ui_even_first_20_total
      / (px_dimutex_ctxt->x_init_params.ui_no_nodes / 2);
   ui_even_last_20_avg = ui_even_last_20_total
      / (px_dimutex_ctxt->x_init_params.ui_no_nodes / 2);
   ui_even_full_avg = (ui_even_full_total
      / (px_dimutex_ctxt->x_init_params.ui_no_nodes / 2) / 2);

   ui_odd_first_20_avg = ui_odd_first_20_total
      / (px_dimutex_ctxt->x_init_params.ui_no_nodes / 2);
   ui_odd_last_20_avg = ui_odd_last_20_total
      / (px_dimutex_ctxt->x_init_params.ui_no_nodes / 2);
   ui_odd_full_avg = (ui_odd_full_total
      / (px_dimutex_ctxt->x_init_params.ui_no_nodes / 2) / 2);

   ui_all_first_20_avg = (ui_all_first_20_total
      / px_dimutex_ctxt->x_init_params.ui_no_nodes);
   ui_all_last_20_avg = (ui_all_last_20_total
      / px_dimutex_ctxt->x_init_params.ui_no_nodes);
   ui_all_full_avg = ((ui_all_full_total
      / px_dimutex_ctxt->x_init_params.ui_no_nodes) / 2);

   printf ("\n\n\n");
   printf ("************************************************************\n");
   printf ("***************Dimutex Run Statistics Summary***************\n");
   printf (
      "\nGeneral Information:\n"
      "\tConfigured Unit Time          : %d\n"
      "\tOdd Numbered Nodes First 20   : [10, 20] * %d ms\n"
      "\tOdd Numbered Nodes Last 20    : [10, 20] * %d ms\n"
      "\tEven Numbered Nodes First 20  : [10, 20] * %d ms\n"
      "\tEven Numbered Nodes Last 20   : [40, 50] * %d ms\n",
      px_dimutex_ctxt->x_init_params.ui_unit_time_ms,
      px_dimutex_ctxt->x_init_params.ui_unit_time_ms,
      px_dimutex_ctxt->x_init_params.ui_unit_time_ms,
      px_dimutex_ctxt->x_init_params.ui_unit_time_ms,
      px_dimutex_ctxt->x_init_params.ui_unit_time_ms);
   printf ("\nEven Nodes:\n"
      "\tFirst 20 Entries Average      : %d ms\n"
      "\tLast 20 Entries Average       : %d ms\n"
      "\tOverall Average               : %d ms\n",
      ui_even_first_20_avg, ui_even_last_20_avg, ui_even_full_avg);
   printf ("\nOdd Nodes:\n"
      "\tFirst 20 Entries Average      : %d ms\n"
      "\tLast 20 Entries Average       : %d ms\n"
      "\tOverall Average               : %d ms\n",
      ui_odd_first_20_avg, ui_odd_last_20_avg, ui_odd_full_avg);
   printf ("\nAll Nodes:\n"
      "\tFirst 20 Entries Average      : %d ms\n"
      "\tLast 20 Entries Average       : %d ms\n"
      "\tOverall Average               : %d ms\n"
      "\tTotal Message Count           : %d\n",
      ui_all_first_20_avg, ui_all_last_20_avg, ui_all_full_avg,
      ui_total_msgs_count);
   printf ("************************************************************\n");

   fprintf (fp_stats_summary,
      "\n\n\n");
   fprintf (fp_stats_summary,
      "************************************************************\n");
   fprintf (fp_stats_summary,
      "***************Dimutex Run Statistics Summary***************\n");
   fprintf (fp_stats_summary,
      "\nGeneral Information:\n"
      "\tConfigured Unit Time          : %d\n"
      "\tOdd Numbered Nodes First 20   : [10, 20] * %d ms\n"
      "\tOdd Numbered Nodes Last 20    : [10, 20] * %d ms\n"
      "\tEven Numbered Nodes First 20  : [10, 20] * %d ms\n"
      "\tEven Numbered Nodes Last 20   : [40, 50] * %d ms\n",
      px_dimutex_ctxt->x_init_params.ui_unit_time_ms,
      px_dimutex_ctxt->x_init_params.ui_unit_time_ms,
      px_dimutex_ctxt->x_init_params.ui_unit_time_ms,
      px_dimutex_ctxt->x_init_params.ui_unit_time_ms,
      px_dimutex_ctxt->x_init_params.ui_unit_time_ms);
   fprintf (fp_stats_summary,
      "\nEven Nodes:\n"
      "\tFirst 20 Entries Average      : %d ms\n"
      "\tLast 20 Entries Average       : %d ms\n"
      "\tOverall Average               : %d ms\n",
      ui_even_first_20_avg, ui_even_last_20_avg, ui_even_full_avg);
   fprintf (fp_stats_summary,
      "\nOdd Nodes:\n"
      "\tFirst 20 Entries Average      : %d ms\n"
      "\tLast 20 Entries Average       : %d ms\n"
      "\tOverall Average               : %d ms\n",
      ui_odd_first_20_avg, ui_odd_last_20_avg, ui_odd_full_avg);
   fprintf (fp_stats_summary,
      "\nAll Nodes:\n"
      "\tFirst 20 Entries Average      : %d ms\n"
      "\tLast 20 Entries Average       : %d ms\n"
      "\tOverall Average               : %d ms\n"
      "\tTotal Message Count           : %d\n",
      ui_all_first_20_avg, ui_all_last_20_avg, ui_all_full_avg,
      ui_total_msgs_count);
   fprintf (fp_stats_summary,
      "************************************************************\n");

   fflush (fp_stats_summary);
   fclose (fp_stats_summary);
   fp_stats_summary = NULL;

CLEAN_RETURN:
   return e_error;
}
