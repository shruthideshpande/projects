/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   rfsvp_mgr_node.c
 * \author sandeepprakash
 *
 * \date   26-Sep-2012
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
static RFSVP_RET_E rfsvp_mgr_node_init_my_node_details (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt);

static RFSVP_RET_E rfsvp_mgr_node_task_init (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt);

static RFSVP_RET_E rfsvp_mgr_node_task_deinit (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt);

static RFSVP_RET_E rfsvp_mgr_node_handle_msgs (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   RFSVP_MSG_HDR_X *px_msg_hdr);

static RFSVP_RET_E rfsvp_mgr_node_handle_listen_sock_act (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt);

static RFSVP_RET_E rfsvp_mgr_node_handle_msgs_from_nodes (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   PAL_SOCK_HDL *phl_act_sock_hdl,
   uint8_t *puc_msg_data,
   uint32_t ui_msg_size);

static RFSVP_RET_E rfsvp_mgr_node_handle_data_from_nodes (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   PAL_SOCK_HDL *phl_act_sock_hdl,
   uint8_t *puc_msg_data,
   uint32_t ui_msg_size);

static RFSVP_RET_E rfsvp_mgr_node_handle_conn_sock_act (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   PAL_SOCK_HDL *phl_act_sock_hdl);

static RFSVP_RET_E rfsvp_mgr_node_handle_sock_act (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   RFSVP_MSG_HDR_X *px_msg_hdr);

/****************************** LOCAL FUNCTIONS *******************************/
void *rfsvp_mgr_node_task(
   void *p_thread_args)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   TASK_RET_E e_task_ret = eTASK_RET_FAILURE;
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt = NULL;
   MSGQ_DATA_X x_data = {NULL};
   RFSVP_MSG_HDR_X *px_msg_hdr = NULL;

   if (NULL == p_thread_args)
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      goto CLEAN_RETURN;
   }

   px_rfsvp_mgr_ctxt = (RFSVP_MGR_CTXT_X *) p_thread_args;

   e_error = rfsvp_mgr_node_task_init (px_rfsvp_mgr_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_node_task_init failed: %d", e_error);
      goto CLEAN_RETURN;
   }
   RFSVP_M_LOG_MED("rfsvp_mgr_node_task_init success");

   while (task_is_in_loop (px_rfsvp_mgr_ctxt->hl_listner_task_hdl))
   {
      e_task_ret = task_get_msg_from_q(px_rfsvp_mgr_ctxt->hl_listner_task_hdl,
         &x_data, RFSVP_MGR_TASK_Q_WAIT_TIMEOUT);
      if ((eTASK_RET_SUCCESS == e_task_ret) && (NULL != x_data.p_data)
         && (0 != x_data.ui_data_size))
      {
         px_msg_hdr = (RFSVP_MSG_HDR_X *) x_data.p_data;

         e_error = rfsvp_mgr_node_handle_msgs (px_rfsvp_mgr_ctxt, px_msg_hdr);
         if (eRFSVP_RET_SUCCESS != e_error)
         {
            RFSVP_M_LOG_LOW("rfsvp_mgr_node_handle_msgs failed: %d", e_error);
         }
         else
         {
            rfsvp_mgr_node_log_status (px_rfsvp_mgr_ctxt);
         }
      }
      RFSVP_M_LOG_FULL("RFSVP Manager Listener Task");
   }

   RFSVP_M_LOG_MED("Out of task loop");
   e_error = rfsvp_mgr_node_task_deinit (px_rfsvp_mgr_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_node_task_deinit failed: %d", e_error);
   }
   RFSVP_M_LOG_MED("rfsvp_mgr_node_task_deinit success");
CLEAN_RETURN:
   RFSVP_M_LOG_MED("Notifying task exit");
   e_task_ret = task_notify_exit (px_rfsvp_mgr_ctxt->hl_listner_task_hdl);
   if (eTASK_RET_SUCCESS != e_task_ret)
   {
      RFSVP_M_LOG_LOW("task_notify_exit failed: %d", e_task_ret);
   }
   else
   {
      RFSVP_M_LOG_MED("task_notify_exit success");
   }
   return p_thread_args;
}

static RFSVP_RET_E rfsvp_mgr_node_handle_listen_sock_act (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   PAL_SOCK_ADDR_IN_X x_in_addr = {0};
   PAL_SOCK_HDL *phl_sock_hdl = NULL;
   uint32_t ui_index = 0;

   if (NULL == px_rfsvp_mgr_ctxt)
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   for (ui_index = 0; ui_index < RFSVP_MAX_SERVER_NODES; ui_index++)
   {
      if (NULL == px_rfsvp_mgr_ctxt->hla_temp_node_sock [ui_index])
      {
         RFSVP_M_LOG_LOW("Found empty sock context @ %d index", ui_index);
         break;
      }
   }

   if (ui_index >= RFSVP_MAX_SERVER_NODES)
   {
      RFSVP_M_LOG_LOW("No empty temp socket present.");
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   phl_sock_hdl = &(px_rfsvp_mgr_ctxt->hla_temp_node_sock [ui_index]);

   RFSVP_M_LOG_MED("New connection. Accepting it.");
   e_pal_ret = pal_sock_accept (px_rfsvp_mgr_ctxt->hl_listner_sock_hdl,
      &x_in_addr, phl_sock_hdl);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (NULL == *phl_sock_hdl))
   {
      RFSVP_M_LOG_MED ("pal_sock_accept failed: %d", e_pal_ret);
   }
   else
   {
      RFSVP_M_LOG_MED("New connection on listen socket.");
      e_error = rfsvp_mgr_node_handle_conn_sock_act (px_rfsvp_mgr_ctxt,
         phl_sock_hdl);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_M_LOG_LOW("rfsvp_mgr_node_handle_conn_sock_act failed: %d", e_error);
         e_pal_ret = pal_sock_close (*phl_sock_hdl);
         if (ePAL_RET_SUCCESS != e_pal_ret)
         {
            RFSVP_M_LOG_MED("pal_sock_close failed: %d", e_pal_ret);
         }
         *phl_sock_hdl = NULL;
      }
   }

   RFSVP_M_LOG_MED("Re-registering the listen socket.");
   e_error = rfsvp_mgr_node_register_sock (px_rfsvp_mgr_ctxt,
      px_rfsvp_mgr_ctxt->hl_listner_sock_hdl);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_node_register_sock failed: %d",
         e_error);
   }

CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_mgr_node_handle_msgs_from_nodes (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   PAL_SOCK_HDL *phl_act_sock_hdl,
   uint8_t *puc_msg_data,
   uint32_t ui_msg_size)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_HDR_X *px_msg_header = NULL;

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == phl_act_sock_hdl)
      || (NULL == puc_msg_data) || (0 == ui_msg_size))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_msg_header = (NODE_MSG_HDR_X *) px_rfsvp_mgr_ctxt->uca_temp_sock_buf;
   switch (px_msg_header->ui_msg_id)
   {
      /*
       * Node Setup Messages.
       */
      case eNODE_MSG_ID_SETUP_JOIN:
      {
         RFSVP_M_LOG_MED("Got eNODE_MSG_ID_SETUP_JOIN");
         e_error = rfsvp_mgr_node_handle_setup_join (px_rfsvp_mgr_ctxt,
            phl_act_sock_hdl, px_msg_header);
         break;
      }
      case eNODE_MSG_ID_SETUP_ESTABLISH_DONE:
      {
         RFSVP_M_LOG_MED("Got eNODE_MSG_ID_SETUP_ESTABLISH_DONE");
         e_error = rfsvp_mgr_node_handle_setup_establish_done (px_rfsvp_mgr_ctxt,
            phl_act_sock_hdl, px_msg_header);
         break;
      }
      case eNODE_MSG_ID_APP_MSG_CMD_RSP:
      {
         RFSVP_M_LOG_MED("Got eNODE_MSG_ID_APP_MSG_CMD_RSP");
         e_error = rfsvp_mgr_node_handle_app_msg_cmd_rsp (px_rfsvp_mgr_ctxt,
            phl_act_sock_hdl, px_msg_header);
         break;
      }
      default:
      {
         RFSVP_M_LOG_LOW("Invalid message: %d received.",
            px_msg_header->ui_msg_id);
         e_error = eRFSVP_RET_INVALID_ARGS;
         break;
      }
   }
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_mgr_node_handle_data_from_nodes (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   PAL_SOCK_HDL *phl_act_sock_hdl,
   uint8_t *puc_msg_data,
   uint32_t ui_msg_size)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == phl_act_sock_hdl)
      || (NULL == puc_msg_data) || (0 == ui_msg_size))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   e_error = rfsvp_mgr_node_register_sock (px_rfsvp_mgr_ctxt,
      *phl_act_sock_hdl);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_node_register_sock failed: %d", e_error);
   }
   else
   {
      e_error = rfsvp_mgr_node_handle_msgs_from_nodes (px_rfsvp_mgr_ctxt,
         phl_act_sock_hdl, puc_msg_data, ui_msg_size);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_M_LOG_LOW("rfsvp_mgr_node_handle_msgs_from_clients failed: %d",
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
            RFSVP_M_LOG_MED("pal_sock_close failed: %d", e_pal_ret);
         }
         *phl_act_sock_hdl = NULL;
      }
   }
   return e_error;
}

static RFSVP_RET_E rfsvp_mgr_node_handle_conn_sock_act (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   PAL_SOCK_HDL *phl_act_sock_hdl)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   uint32_t ui_recv_size = 0;

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == phl_act_sock_hdl))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   ui_recv_size = sizeof(px_rfsvp_mgr_ctxt->uca_temp_sock_buf);

   e_error = rfsvp_mgr_node_read_msg (*phl_act_sock_hdl,
      px_rfsvp_mgr_ctxt->uca_temp_sock_buf,
      sizeof(px_rfsvp_mgr_ctxt->uca_temp_sock_buf), &ui_recv_size);
   if ((eRFSVP_RET_SUCCESS == e_error)
      && (ui_recv_size >= sizeof(NODE_MSG_HDR_X)))
   {
      RFSVP_M_LOG_LOW("Received %d bytes.", ui_recv_size);
      e_error = rfsvp_mgr_node_handle_data_from_nodes (px_rfsvp_mgr_ctxt,
         phl_act_sock_hdl, px_rfsvp_mgr_ctxt->uca_temp_sock_buf, ui_recv_size);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_M_LOG_LOW("rfsvp_mgr_node_handle_data_from_clients failed: %d",
            e_error);
      }
   }
   else
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_node_read_msg failed: %d. Hence closing all sockets.",
         e_pal_ret);
      e_pal_ret = pal_sock_close(*phl_act_sock_hdl);
      if (ePAL_RET_SUCCESS != e_pal_ret)
      {
         RFSVP_M_LOG_LOW("pal_sock_close failed: %d", e_pal_ret);
      }
      *phl_act_sock_hdl = NULL;


      (void) rfsvp_mgr_node_cleanup_socks (px_rfsvp_mgr_ctxt);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_mgr_node_handle_sock_act (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   RFSVP_MSG_HDR_X *px_msg_hdr)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_MGR_SOCK_ACT_DATA_X *px_sock_act_data = NULL;
   uint32_t ui_index = 0;

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == px_msg_hdr))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_sock_act_data = (RFSVP_MGR_SOCK_ACT_DATA_X *) px_msg_hdr;

   if (px_sock_act_data->hl_sock_hdl
      == px_rfsvp_mgr_ctxt->hl_listner_sock_hdl)
   {
      e_error = rfsvp_mgr_node_handle_listen_sock_act (px_rfsvp_mgr_ctxt);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_M_LOG_LOW("rfsvp_mgr_node_handle_listen_sock_act failed: %d",
            e_error);
      }
   }
   else
   {
      for (ui_index = 0; ui_index < RFSVP_MAX_SERVER_NODES; ui_index++)
      {
         if (px_sock_act_data->hl_sock_hdl
            == px_rfsvp_mgr_ctxt->hla_temp_node_sock [ui_index])
         {
            RFSVP_M_LOG_LOW("Activity on socket @ %d temp index", ui_index);
            break;
         }
      }
      if (ui_index < RFSVP_MAX_SERVER_NODES)
      {
         e_error = rfsvp_mgr_node_handle_conn_sock_act (px_rfsvp_mgr_ctxt,
            &(px_rfsvp_mgr_ctxt->hla_temp_node_sock [ui_index]));
         if (eRFSVP_RET_SUCCESS != e_error)
         {
            RFSVP_M_LOG_LOW("rfsvp_mgr_node_handle_conn_sock_act failed: %d",
               e_error);
         }
      }
      else
      {
         for (ui_index = 0;
               ui_index < px_rfsvp_mgr_ctxt->x_init_params.ui_no_nodes;
               ui_index++)
         {
            if (px_sock_act_data->hl_sock_hdl
               == px_rfsvp_mgr_ctxt->x_server_nodes.hla_tcp_sock_hdl[ui_index])
            {
               RFSVP_M_LOG_LOW("Activity on socket from %d node", ui_index);
               break;
            }
         }
         if (ui_index < px_rfsvp_mgr_ctxt->x_init_params.ui_no_nodes)
         {
            e_error = rfsvp_mgr_node_handle_conn_sock_act (px_rfsvp_mgr_ctxt,
               &(px_rfsvp_mgr_ctxt->x_server_nodes.hla_tcp_sock_hdl[ui_index]));
            if (eRFSVP_RET_SUCCESS != e_error)
            {
               RFSVP_M_LOG_LOW("rfsvp_mgr_node_handle_conn_sock_act failed: %d",
                  e_error);
            }
         }
         else
         {
            e_error = eRFSVP_RET_SUCCESS;
         }
      }
   }
   pal_free(px_sock_act_data);
   px_sock_act_data = NULL;

CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_mgr_node_handle_msgs (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   RFSVP_MSG_HDR_X *px_msg_hdr)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == px_msg_hdr))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   switch (px_msg_hdr->ui_msg_id)
   {
      case eRFSVP_MGR_MSG_ID_SOCK_ACTIVITY:
      {
         RFSVP_M_LOG_MED("Got eRFSVP_MGR_MSG_ID_SOCK_ACTIVITY");
         e_error = rfsvp_mgr_node_handle_sock_act (px_rfsvp_mgr_ctxt,
            px_msg_hdr);
         if (eRFSVP_RET_SUCCESS != e_error)
         {
            RFSVP_M_LOG_LOW("rfsvp_mgr_node_handle_sock_act failed: %d", e_error);
         }
         break;
      }
      default:
      {
         RFSVP_M_LOG_LOW("Invalid Msg Id: %d", px_msg_hdr->ui_msg_id);
         e_error = eRFSVP_RET_INVALID_ARGS;
         break;
      }
   }

CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_mgr_node_init_my_node_details (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;
   PAL_SOCK_IN_ADDR_X x_in_addr = {0};
   RFSVP_MGR_NODE_CTXT_X *px_myself = NULL;
   uint8_t uca_ip_addr_str[16] = {0};

   if (NULL == px_rfsvp_mgr_ctxt)
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_myself = &(px_rfsvp_mgr_ctxt->x_mgr_node);

   e_pal_ret = pal_gethostname (px_myself->uca_dns_name_str,
      sizeof(px_myself->uca_dns_name_str));
   if (ePAL_RET_FAILURE == e_pal_ret)
   {
      RFSVP_M_LOG_LOW("pal_gethostname failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   e_pal_ret = pal_gethostbyname (px_myself->uca_dns_name_str,
      &x_in_addr);
   if ((ePAL_RET_SUCCESS != e_pal_ret) || (0 == x_in_addr.ui_ip_addr_no))
   {
      RFSVP_M_LOG_LOW("pal_gethostbyname failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }
   px_myself->ui_ip_addr_ho = pal_ntohl (x_in_addr.ui_ip_addr_no);
   px_myself->us_port_ho =
      px_rfsvp_mgr_ctxt->x_init_params.us_listen_port_start_ho;

   (void) pal_get_ip_addr_str (&x_in_addr, uca_ip_addr_str,
      sizeof(uca_ip_addr_str));

   RFSVP_M_LOG_LOW("My (Manager) Node Details:");
   RFSVP_M_LOG_LOW("DNS Host Name: %s", px_myself->uca_dns_name_str);
   RFSVP_M_LOG_LOW("IP           : %s (ho: 0x%x, no: 0x%x)", uca_ip_addr_str,
      px_myself->ui_ip_addr_ho, x_in_addr.ui_ip_addr_no);
   RFSVP_M_LOG_LOW("Port         : %d", px_myself->us_port_ho);
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_mgr_node_task_init (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;

   if (NULL == px_rfsvp_mgr_ctxt)
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   e_error = rfsvp_mgr_node_init_my_node_details (px_rfsvp_mgr_ctxt);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_node_init_my_node_details failed: %d",
         e_error);
      goto CLEAN_RETURN;
   }

   e_pal_ret = tcp_listen_sock_create (&(px_rfsvp_mgr_ctxt->hl_listner_sock_hdl),
      px_rfsvp_mgr_ctxt->x_mgr_node.us_port_ho);
   if ((ePAL_RET_SUCCESS != e_pal_ret)
      || (NULL == px_rfsvp_mgr_ctxt->hl_listner_sock_hdl))
   {
      RFSVP_M_LOG_LOW("tcp_listen_sock_create failed: %d, %p",
         e_pal_ret, px_rfsvp_mgr_ctxt->hl_listner_sock_hdl);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }
   e_error = rfsvp_mgr_node_register_sock (px_rfsvp_mgr_ctxt,
      px_rfsvp_mgr_ctxt->hl_listner_sock_hdl);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_node_register_sock failed: %d", e_error);
      e_pal_ret = tcp_listen_sock_delete(px_rfsvp_mgr_ctxt->hl_listner_sock_hdl);
      if (ePAL_RET_SUCCESS != e_pal_ret)
      {
         RFSVP_M_LOG_LOW("tcp_listen_sock_delete failed: %d", e_pal_ret);
      }
      px_rfsvp_mgr_ctxt->hl_listner_sock_hdl = NULL;
   }
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_mgr_node_task_deinit (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;

   if (NULL == px_rfsvp_mgr_ctxt)
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      goto CLEAN_RETURN;
   }

   (void) rfsvp_mgr_node_cleanup_socks (px_rfsvp_mgr_ctxt);

   e_error = rfsvp_mgr_node_deregister_sock (px_rfsvp_mgr_ctxt,
      px_rfsvp_mgr_ctxt->hl_listner_sock_hdl);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW ("rfsvp_mgr_node_deregister_sock failed: %d", e_error);
   }

   if (NULL != px_rfsvp_mgr_ctxt->hl_listner_sock_hdl)
   {
      e_pal_ret = tcp_listen_sock_delete (px_rfsvp_mgr_ctxt->hl_listner_sock_hdl);
      if (ePAL_RET_SUCCESS != e_pal_ret)
      {
         RFSVP_M_LOG_LOW("tcp_listen_sock_delete failed: %d", e_pal_ret);
      }
      px_rfsvp_mgr_ctxt->hl_listner_sock_hdl = NULL;
   }
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_mgr_post_msg_to_q (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   uint8_t *puc_data,
   uint32_t ui_data_len)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   TASK_RET_E e_task_ret = eTASK_RET_FAILURE;
   MSGQ_DATA_X x_data = {NULL};

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == puc_data) || (0 == ui_data_len))
   {
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_data.p_data = puc_data;
   x_data.ui_data_size = ui_data_len;
   e_task_ret = task_add_msg_to_q(px_rfsvp_mgr_ctxt->hl_listner_task_hdl,
      &x_data, RFSVP_MGR_TASK_Q_WAIT_TIMEOUT);
   if (eTASK_RET_SUCCESS != e_task_ret)
   {
      RFSVP_M_LOG_LOW("task_add_msg_to_q failed: %d", e_error);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }

CLEAN_RETURN:
   return e_error;
}

SOCKMON_RET_E rfsvp_mgr_node_sockmon_active_sock_cbk (
   SOCKMON_SOCK_ACTIVITY_STATUS_E e_status,
   PAL_SOCK_HDL hl_sock_hdl,
   void *p_app_data)
{
   SOCKMON_RET_E e_sockmon_ret = eSOCKMON_RET_FAILURE;
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt = NULL;
   RFSVP_MGR_SOCK_ACT_DATA_X *px_sock_act_data = NULL;

   if ((NULL == hl_sock_hdl) || (NULL == p_app_data))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_sockmon_ret = eSOCKMON_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_rfsvp_mgr_ctxt = (RFSVP_MGR_CTXT_X *) p_app_data;

   RFSVP_M_LOG_LOW("rfsvp_mgr_node_sockmon_active_sock_cbk called.");

   px_sock_act_data = pal_malloc (sizeof(RFSVP_MGR_SOCK_ACT_DATA_X),
      NULL );

   px_sock_act_data->x_hdr.ui_msg_id = eRFSVP_MGR_MSG_ID_SOCK_ACTIVITY;
   px_sock_act_data->x_hdr.ui_msg_pay_len = sizeof(*px_sock_act_data) -
         sizeof(px_sock_act_data->x_hdr);
   px_sock_act_data->hl_sock_hdl = hl_sock_hdl;
   e_error = rfsvp_mgr_post_msg_to_q (
      px_rfsvp_mgr_ctxt,
      (uint8_t *) px_sock_act_data,
      sizeof(*px_sock_act_data));
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_post_msg_to_q failed: %d", e_error);
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
