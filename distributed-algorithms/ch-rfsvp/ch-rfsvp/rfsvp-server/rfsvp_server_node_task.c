/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   rfsvp_server_node_task.c
 * \author sandeepprakash
 *
 * \date   26-Sep-2012
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
static RFSVP_RET_E rfsvp_server_node_generate_udp_ping (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt);

static RFSVP_RET_E rfsvp_server_node_init_mgr_node_details (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt);

static RFSVP_RET_E rfsvp_server_node_init_my_node_details (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt);

static RFSVP_RET_E rfsvp_server_node_main_task_init (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt);

static RFSVP_RET_E rfsvp_server_node_main_task_deinit (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt);

static RFSVP_RET_E rfsvp_server_node_handle_msgs (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   RFSVP_MSG_HDR_X *px_msg_hdr);

static RFSVP_RET_E rfsvp_server_node_handle_sock_act (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   RFSVP_MSG_HDR_X *px_msg_hdr);

/****************************** LOCAL FUNCTIONS *******************************/
void *rfsvp_server_node_main_task(
   void *p_thread_args)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   TASK_RET_E e_task_ret = eTASK_RET_FAILURE;
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt = NULL;
   MSGQ_DATA_X x_data = {NULL};
   RFSVP_MSG_HDR_X *px_msg_hdr = NULL;

   if (NULL == p_thread_args)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      goto CLEAN_RETURN;
   }

   px_rfsvp_server_ctxt = (RFSVP_SERVER_CTXT_X *) p_thread_args;

   e_error = rfsvp_server_node_main_task_init (px_rfsvp_server_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW("rfsvp_server_node_task_init failed: %d", e_error);
      goto CLEAN_RETURN;
   }
   RFSVP_S_LOG_MED("rfsvp_server_node_task_init success");

   while (task_is_in_loop (px_rfsvp_server_ctxt->hl_main_task_hdl))
   {
      e_task_ret = task_get_msg_from_q(px_rfsvp_server_ctxt->hl_main_task_hdl,
         &x_data, RFSVP_SERVER_MAIN_TASK_Q_WAIT_TIMEOUT);
      if ((eTASK_RET_SUCCESS == e_task_ret) && (NULL != x_data.p_data)
         && (0 != x_data.ui_data_size))
      {
         px_msg_hdr = (RFSVP_MSG_HDR_X *) x_data.p_data;

         e_error = rfsvp_server_node_handle_msgs (px_rfsvp_server_ctxt, px_msg_hdr);
         if (eRFSVP_RET_SUCCESS != e_error)
         {
            RFSVP_S_LOG_LOW("rfsvp_server_node_handle_msgs failed: %d", e_error);
         }
         else
         {
            rfsvp_server_node_log_status (px_rfsvp_server_ctxt);
         }
      }
      (void) rfsvp_server_node_generate_udp_ping (px_rfsvp_server_ctxt);
      RFSVP_S_LOG_FULL("RFSVP Server Main Task");
   }

   RFSVP_S_LOG_MED("Out of task loop");
   e_error = rfsvp_server_node_main_task_deinit (px_rfsvp_server_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW("rfsvp_server_node_task_deinit failed: %d", e_error);
   }
   RFSVP_S_LOG_MED("rfsvp_server_node_task_deinit success");
CLEAN_RETURN:
   RFSVP_S_LOG_MED("Notifying task exit");
   e_task_ret = task_notify_exit (px_rfsvp_server_ctxt->hl_main_task_hdl);
   if (eTASK_RET_SUCCESS != e_task_ret)
   {
      RFSVP_S_LOG_LOW("task_notify_exit failed: %d", e_task_ret);
   }
   else
   {
      RFSVP_S_LOG_MED("task_notify_exit success");
   }
   return p_thread_args;
}

static RFSVP_RET_E rfsvp_server_node_generate_udp_ping (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   uint32_t ui_curr_time_ms = 0;
   uint32_t ui_elapsed_time_ms = 0;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   ui_curr_time_ms = pal_get_system_time ();
   ui_elapsed_time_ms = ui_curr_time_ms
      - px_rfsvp_server_ctxt->ui_last_ping_time_ms;
   if (ui_elapsed_time_ms < RFSVP_SERVER_TASK_PING_PEERS_INTERVAL_MS)
   {
      e_error = eRFSVP_RET_SUCCESS;
      goto CLEAN_RETURN;
   }

   RFSVP_S_LOG_LOW("Generating Ping");
   e_error = rfsvp_server_node_send_udp_ping_to_all (px_rfsvp_server_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_HIGH("rfsvp_server_node_send_ping_to_all failed: %d", e_error);
   }
   px_rfsvp_server_ctxt->ui_last_ping_time_ms = pal_get_system_time ();
   px_rfsvp_server_ctxt->ui_ping_seq_no++;
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_server_node_handle_sock_act (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   RFSVP_MSG_HDR_X *px_msg_hdr)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_NODE_SOCK_ACT_DATA_X *px_sock_act_data = NULL;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_hdr))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_sock_act_data = (RFSVP_SERVER_NODE_SOCK_ACT_DATA_X *) px_msg_hdr;

   if (px_sock_act_data->hl_sock_hdl == px_rfsvp_server_ctxt->hl_mgr_sock_hdl)
   {
      RFSVP_S_LOG_MED("Got eRFSVP_SERVER_MSG_ID_SOCK_ACTIVITY on TCP socket");
      e_error = rfsvp_server_node_handle_tcp_conn_sock_act (
         px_rfsvp_server_ctxt, &(px_rfsvp_server_ctxt->hl_mgr_sock_hdl));
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_S_LOG_LOW(
            "rfsvp_server_node_handle_tcp_conn_sock_act failed: %d", e_error);
      }
   }
   else if (px_sock_act_data->hl_sock_hdl
      == px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl)
   {
      RFSVP_S_LOG_MED("Got eRFSVP_SERVER_MSG_ID_SOCK_ACTIVITY on a UDP socket");
      e_error = rfsvp_server_node_handle_udp_conn_sock_act (
         px_rfsvp_server_ctxt,
         &(px_rfsvp_server_ctxt->x_server_nodes.hl_udp_sock_hdl));
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_S_LOG_LOW(
            "rfsvp_server_node_handle_udp_conn_sock_act failed: %d", e_error);
      }
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
   pal_free(px_sock_act_data);
   px_sock_act_data = NULL;
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_server_node_handle_msgs (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   RFSVP_MSG_HDR_X *px_msg_hdr)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == px_msg_hdr))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   switch (px_msg_hdr->ui_msg_id)
   {
      case eRFSVP_SERVER_MSG_ID_SOCK_ACTIVITY:
      {
         RFSVP_S_LOG_FULL("Got eRFSVP_SERVER_MSG_ID_SOCK_ACTIVITY");
         e_error = rfsvp_server_node_handle_sock_act (px_rfsvp_server_ctxt,
            px_msg_hdr);
         if (eRFSVP_RET_SUCCESS != e_error)
         {
            RFSVP_S_LOG_LOW("rfsvp_server_node_handle_sock_act failed: %d", e_error);
         }
         break;
      }
      case eRFSVP_SERVER_MSG_ID_EXIT_TASK:
      {
         RFSVP_S_LOG_LOW("Got eRFSVP_SERVER_MSG_ID_EXIT_TASK");
         e_pal_ret = pal_sem_put (
            px_rfsvp_server_ctxt->x_init_params.hl_exit_sem_hdl);
         if (ePAL_RET_SUCCESS != e_pal_ret)
         {
            RFSVP_S_LOG_LOW("pal_sem_put failed: %d", e_pal_ret);
            e_error = eRFSVP_RET_RESOURCE_FAILURE;
         }
         else
         {
            RFSVP_S_LOG_LOW("pal_sem_put success");
            e_error = eRFSVP_RET_SUCCESS;
         }
         pal_free(px_msg_hdr);
         px_msg_hdr = NULL;
         break;
      }
      default:
      {
         RFSVP_S_LOG_LOW("Invalid Msg Id: %d", px_msg_hdr->ui_msg_id);
         e_error = eRFSVP_RET_INVALID_ARGS;
         break;
      }
   }

CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_server_node_init_mgr_node_details (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   PAL_SOCK_IN_ADDR_X x_in_addr = {0};
   RFSVP_MGR_NODE_CTXT_X *px_mgr = NULL;
   uint8_t uca_ip_addr_str[16] = {0};

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_mgr = &(px_rfsvp_server_ctxt->x_mgr_node);

   (void) pal_strncpy (px_mgr->uca_dns_name_str,
      px_rfsvp_server_ctxt->x_init_params.uca_mgr_host_name_str,
      sizeof(px_mgr->uca_dns_name_str));

   e_pal_ret = pal_gethostbyname (px_mgr->uca_dns_name_str,
      &x_in_addr);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (0 == x_in_addr.ui_ip_addr_no))
   {
      RFSVP_S_LOG_LOW("pal_gethostbyname failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }
   px_mgr->ui_ip_addr_ho = pal_ntohl (x_in_addr.ui_ip_addr_no);
   px_mgr->us_port_ho = px_rfsvp_server_ctxt->x_init_params.us_mgr_port_ho;

   (void) pal_get_ip_addr_str (&x_in_addr, uca_ip_addr_str,
      sizeof(uca_ip_addr_str));

   RFSVP_S_LOG_LOW("Manager Node Details:");
   RFSVP_S_LOG_LOW("DNS Host Name: %s", px_mgr->uca_dns_name_str);
   RFSVP_S_LOG_LOW("IP           : %s (ho: 0x%x, no: 0x%x)", uca_ip_addr_str,
      px_mgr->ui_ip_addr_ho, x_in_addr.ui_ip_addr_no);
   RFSVP_S_LOG_LOW("Port         : %d", px_mgr->us_port_ho);
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_server_node_init_my_node_details (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   PAL_SOCK_IN_ADDR_X x_in_addr = {0};
   RFSVP_SERVER_NODE_CTXT_X *px_myself = NULL;
   uint8_t uca_ip_addr_str[16] = {0};
   uint32_t ui_my_node_idx = 0;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   ui_my_node_idx = px_rfsvp_server_ctxt->x_init_params.ui_node_index;
   px_myself = &(px_rfsvp_server_ctxt->x_server_nodes.xa_nodes[ui_my_node_idx]);

   px_myself->ui_node_index = ui_my_node_idx;
   px_myself->us_udp_port_ho =
      (px_rfsvp_server_ctxt->x_init_params.us_listen_port_start_ho
         + ui_my_node_idx);
   e_pal_ret = pal_gethostname (px_myself->uca_dns_name_str,
      sizeof(px_myself->uca_dns_name_str));
   if (ePAL_RET_FAILURE == e_pal_ret)
   {
      RFSVP_S_LOG_LOW("pal_gethostname failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   e_pal_ret = pal_gethostbyname (px_myself->uca_dns_name_str,
      &x_in_addr);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (0 == x_in_addr.ui_ip_addr_no))
   {
      RFSVP_S_LOG_LOW("pal_gethostbyname failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }
   px_myself->ui_ip_addr_ho = pal_ntohl (x_in_addr.ui_ip_addr_no);

   (void) pal_get_ip_addr_str (&x_in_addr, uca_ip_addr_str,
      sizeof(uca_ip_addr_str));

   RFSVP_S_LOG_LOW("My (Node: %d) Node Details:", ui_my_node_idx);
   RFSVP_S_LOG_LOW("DNS Host Name: %s", px_myself->uca_dns_name_str);
   RFSVP_S_LOG_LOW("IP           : %s (ho: 0x%x, no: 0x%x)", uca_ip_addr_str,
      px_myself->ui_ip_addr_ho, x_in_addr.ui_ip_addr_no);
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_server_node_main_task_init (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   e_error = rfsvp_server_node_init_mgr_node_details (px_rfsvp_server_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW("rfsvp_server_node_init_mgr_node_details failed: %d",
         e_error);
      goto CLEAN_RETURN;
   }

   e_error = rfsvp_server_node_init_my_node_details (px_rfsvp_server_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW("rfsvp_server_node_init_my_node_details failed: %d",
         e_error);
      goto CLEAN_RETURN;
   }

   e_error = rfsvp_server_node_establish_conn_to_mgr (px_rfsvp_server_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW("rfsvp_server_node_establish_conn_to_mgr failed: %d",
         e_error);
   }
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_server_node_main_task_deinit (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   TASK_RET_E e_task_ret = eTASK_RET_FAILURE;
   MSGQ_RET_E e_msgq_ret = eMSGQ_RET_FAILURE;

   if (NULL == px_rfsvp_server_ctxt)
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      goto CLEAN_RETURN;
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

   if (NULL != px_rfsvp_server_ctxt->x_algo.hl_app_req_msgq_hdl)
   {
      e_msgq_ret = msgq_deinit(px_rfsvp_server_ctxt->x_algo.hl_app_req_msgq_hdl);
      if (eMSGQ_RET_SUCCESS != e_msgq_ret)
      {
         RFSVP_S_LOG_LOW("msgq_deinit failed: %d", e_msgq_ret);
      }
      px_rfsvp_server_ctxt->x_algo.hl_app_req_msgq_hdl = NULL;
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

   (void) rfsvp_server_node_cleanup_socks (px_rfsvp_server_ctxt);
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_server_post_msg_to_main_task_q (
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt,
   uint8_t *puc_data,
   uint32_t ui_data_len)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   TASK_RET_E e_task_ret = eTASK_RET_FAILURE;
   MSGQ_DATA_X x_data = {NULL};

   if ((NULL == px_rfsvp_server_ctxt) || (NULL == puc_data) || (0 == ui_data_len))
   {
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_data.p_data = puc_data;
   x_data.ui_data_size = ui_data_len;
   e_task_ret = task_add_msg_to_q(px_rfsvp_server_ctxt->hl_main_task_hdl,
      &x_data, RFSVP_SERVER_MAIN_TASK_Q_WAIT_TIMEOUT);
   if (eTASK_RET_SUCCESS != e_task_ret)
   {
      RFSVP_S_LOG_LOW("task_add_msg_to_q failed: %d", e_error);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }

CLEAN_RETURN:
   return e_error;
}

SOCKMON_RET_E rfsvp_server_node_sockmon_active_sock_cbk (
   SOCKMON_SOCK_ACTIVITY_STATUS_E e_status,
   PAL_SOCK_HDL hl_sock_hdl,
   void *p_app_data)
{
   SOCKMON_RET_E e_sockmon_ret = eSOCKMON_RET_FAILURE;
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_CTXT_X *px_rfsvp_server_ctxt = NULL;
   RFSVP_SERVER_NODE_SOCK_ACT_DATA_X *px_sock_act_data = NULL;

   if ((NULL == hl_sock_hdl) || (NULL == p_app_data))
   {
      RFSVP_S_LOG_LOW("Invalid Args");
      e_sockmon_ret = eSOCKMON_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_rfsvp_server_ctxt = (RFSVP_SERVER_CTXT_X *) p_app_data;

   RFSVP_S_LOG_HIGH("rfsvp_server_node_sockmon_active_sock_cbk called.");

   px_sock_act_data = pal_malloc (sizeof(RFSVP_SERVER_NODE_SOCK_ACT_DATA_X),
      NULL );

   px_sock_act_data->x_hdr.ui_msg_id = eRFSVP_SERVER_MSG_ID_SOCK_ACTIVITY;
   px_sock_act_data->x_hdr.ui_msg_pay_len = sizeof(*px_sock_act_data) -
         sizeof(px_sock_act_data->x_hdr);
   px_sock_act_data->hl_sock_hdl = hl_sock_hdl;
   e_error = rfsvp_server_post_msg_to_main_task_q (
      px_rfsvp_server_ctxt,
      (uint8_t *) px_sock_act_data,
      sizeof(*px_sock_act_data));
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_S_LOG_LOW("rfsvp_server_post_msg_to_q failed: %d", e_error);
      pal_free(px_sock_act_data);
      px_sock_act_data = NULL;
      e_sockmon_ret = eSOCKMON_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_sockmon_ret = eSOCKMON_RET_SUCCESS;
   }

CLEAN_RETURN:
   return e_sockmon_ret;
}
