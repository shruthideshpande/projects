/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   rfsvp_mgr_node_setup.c
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

/******************************** ENUMERATIONS ********************************/

/************************* STRUCTURE/UNION DATA TYPES *************************/

/************************ STATIC FUNCTION PROTOTYPES **************************/

/****************************** LOCAL FUNCTIONS *******************************/
RFSVP_RET_E rfsvp_mgr_node_handle_setup_join (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   PAL_SOCK_HDL *phl_act_sock_hdl,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_JOIN_X *px_join_cred = NULL;
   RFSVP_SERVER_NODE_CTXT_X *px_join_node = NULL;
   PAL_SOCK_HDL *phl_join_sock_hdl = NULL;
   uint32_t ui_join_index = 0;
   bool b_joined = false;

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == phl_act_sock_hdl)
      || (NULL == px_msg_header))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   px_join_cred = (NODE_MSG_JOIN_X *) px_msg_header;

   ui_join_index = px_join_cred->x_node_ctxt.ui_node_index;

   px_join_node =
      &(px_rfsvp_mgr_ctxt->x_server_nodes.xa_nodes [ui_join_index]);
   (void) pal_memmove(px_join_node, &(px_join_cred->x_node_ctxt),
      sizeof(*px_join_node));
   px_join_node->e_state = eRFSVP_SERVER_NODE_STATE_JOINED;
   px_rfsvp_mgr_ctxt->x_server_nodes.ui_no_nodes++;
   phl_join_sock_hdl = &(px_rfsvp_mgr_ctxt->x_server_nodes.hla_tcp_sock_hdl[ui_join_index]);
   *phl_join_sock_hdl = *phl_act_sock_hdl;
   *phl_act_sock_hdl = NULL;

   RFSVP_M_LOG_LOW("New Node Joined The System");
   RFSVP_M_LOG_LOW("Name      : %s", px_join_cred->x_node_ctxt.uca_dns_name_str);
   RFSVP_M_LOG_LOW("Index     : %d", px_join_cred->x_node_ctxt.ui_node_index);
   RFSVP_M_LOG_LOW("UDP Port  : %d", px_join_cred->x_node_ctxt.us_udp_port_ho);

   e_error = rfsvp_mgr_node_send_join_done_to_node (px_rfsvp_mgr_ctxt,
      ui_join_index);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_node_send_join_done_to_node to node %d failed: %d "
      "to node %d", ui_join_index, e_error);
      goto CLEAN_RETURN;
   }

   b_joined = rfsvp_mgr_check_all_nodes_have_joined (px_rfsvp_mgr_ctxt);
   if (true == b_joined)
   {
      RFSVP_M_LOG_LOW("Got required number of nodes: %d, Required: %d",
         px_rfsvp_mgr_ctxt->x_server_nodes.ui_no_nodes,
         px_rfsvp_mgr_ctxt->x_init_params.ui_no_nodes);
      e_error = rfsvp_mgr_node_send_establish_peers_to_node (px_rfsvp_mgr_ctxt,
         0);
      if (eRFSVP_RET_SUCCESS != e_error)
      {
         RFSVP_M_LOG_LOW(
            "rfsvp_mgr_node_send_establish_peers_to_node failed: %d "
            "to node %d", e_error, 0);
      }
   }
   else
   {
      RFSVP_M_LOG_LOW("Waiting for required number of nodes: %d, Required: %d",
         px_rfsvp_mgr_ctxt->x_server_nodes.ui_no_nodes,
         px_rfsvp_mgr_ctxt->x_init_params.ui_no_nodes);
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_mgr_node_handle_setup_establish_done (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   PAL_SOCK_HDL *phl_act_sock_hdl,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   uint32_t ui_index = 0;
   TASK_RET_E e_task_ret = eTASK_RET_FAILURE;
   TASK_CREATE_PARAM_X x_task_param = { { 0 } };

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == phl_act_sock_hdl)
      || (NULL == px_msg_header))
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
      RFSVP_M_LOG_LOW(
         "Got eNODE_MSG_ID_SETUP_ESTABLISH_DONE response from %d node",
         ui_index);

      if ((ui_index + 1) < px_rfsvp_mgr_ctxt->x_init_params.ui_no_nodes)
      {
         RFSVP_M_LOG_LOW("Sending establish peer to %d node", ui_index + 1);
         e_error = rfsvp_mgr_node_send_establish_peers_to_node (
            px_rfsvp_mgr_ctxt, (ui_index + 1));
         if (eRFSVP_RET_SUCCESS != e_error)
         {
            RFSVP_M_LOG_LOW(
               "rfsvp_mgr_node_send_establish_peers_to_node failed: %d "
               "to node %d", e_error, (ui_index + 1));
         }
      }
      else
      {
         RFSVP_M_LOG_LOW(
            "All nodes are done with establishing peer connections");
         x_task_param.b_msgq_needed = false;
         x_task_param.fn_task_routine = rfsvp_mgr_cmd_task;
         x_task_param.p_app_data = px_rfsvp_mgr_ctxt;
         e_task_ret = task_create(&(px_rfsvp_mgr_ctxt->hl_cmd_task_hdl),
            &x_task_param);
         if ((eTASK_RET_SUCCESS != e_task_ret)
            || (NULL == px_rfsvp_mgr_ctxt->hl_cmd_task_hdl))
         {
            RFSVP_M_LOG_LOW("task_create failed: %d, %p", e_task_ret,
               px_rfsvp_mgr_ctxt, px_rfsvp_mgr_ctxt->hl_cmd_task_hdl);
            e_error = eRFSVP_RET_RESOURCE_FAILURE;
         }
         else
         {
            e_task_ret = task_start (px_rfsvp_mgr_ctxt->hl_cmd_task_hdl);
            if (eTASK_RET_SUCCESS != e_task_ret)
            {
               RFSVP_M_LOG_LOW("task_start failed: %d", e_task_ret);
               e_task_ret = task_delete (px_rfsvp_mgr_ctxt->hl_cmd_task_hdl);
               if (eTASK_RET_SUCCESS != e_task_ret)
               {
                  RFSVP_M_LOG_LOW("task_delete failed: %d", e_task_ret);
               }
               px_rfsvp_mgr_ctxt->hl_cmd_task_hdl = NULL;
               e_error = eRFSVP_RET_RESOURCE_FAILURE;
            }
            else
            {
               RFSVP_M_LOG_MED("task_create success for hl_cmd_task_hdl");
               e_error = eRFSVP_RET_SUCCESS;
            }
         }
      }
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

RFSVP_RET_E rfsvp_mgr_node_handle_app_msg_cmd_rsp (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   PAL_SOCK_HDL *phl_act_sock_hdl,
   NODE_MSG_HDR_X *px_msg_header)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   uint32_t ui_index = 0;
   uint8_t *puc_cmd_str = NULL;
   uint8_t *puc_cmd_status_str = NULL;
   NODE_MSG_APP_CMD_RSP_X *px_rsp = NULL;
   uint8_t *puc_data_off = NULL;

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == phl_act_sock_hdl)
      || (NULL == px_msg_header))
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
      RFSVP_M_LOG_LOW("Got eNODE_MSG_ID_APP_MSG_CMD_RSP response from %d node",
         ui_index);
      px_rsp = (NODE_MSG_APP_CMD_RSP_X *) px_msg_header;

      switch (px_rsp->e_cmd)
      {
         case eRFSVP_SERVER_CMD_HALT:
         {
            puc_cmd_str = (uint8_t *) "eRFSVP_SERVER_CMD_HALT";
            break;
         }
         case eRFSVP_SERVER_CMD_WAIT:
         {
            puc_cmd_str = (uint8_t *) "eRFSVP_SERVER_CMD_WAIT";
            break;
         }
         case eRFSVP_SERVER_CMD_NODE_DOWN:
         {
            puc_cmd_str = (uint8_t *) "eRFSVP_SERVER_CMD_NODE_DOWN";
            break;
         }
         case eRFSVP_SERVER_CMD_NODE_UP:
         {
            puc_cmd_str = (uint8_t *) "eRFSVP_SERVER_CMD_NODE_UP";
            break;
         }
         case eRFSVP_SERVER_CMD_READ:
         {
            puc_cmd_str = (uint8_t *) "eRFSVP_SERVER_CMD_READ";
            break;
         }
         case eRFSVP_SERVER_CMD_WRITE:
         {
            puc_cmd_str = (uint8_t *) "eRFSVP_SERVER_CMD_WRITE";
            break;
         }
         default:
         {
            puc_cmd_str = (uint8_t *) "eRFSVP_SERVER_CMD_INVALID";
            break;
         }
      }

      switch (px_rsp->e_rsp_code)
      {
         case eNODE_MSG_CMD_RSP_CODE_SUCCESS:
         {
            puc_cmd_status_str = (uint8_t *) "eNODE_MSG_CMD_RSP_CODE_SUCCESS";
            break;
         }
         case eNODE_MSG_CMD_RSP_CODE_FAILURE:
         {
            puc_cmd_status_str = (uint8_t *) "eNODE_MSG_CMD_RSP_CODE_FAILURE";
            break;
         }
         case eNODE_MSG_CMD_RSP_CODE_ABORTED:
         {
            puc_cmd_status_str = (uint8_t *) "eNODE_MSG_CMD_RSP_CODE_ABORTED";
            break;
         }
         case eNODE_MSG_CMD_RSP_CODE_FILE_EMPTY:
         {
            puc_cmd_status_str =
               (uint8_t *) "eNODE_MSG_CMD_RSP_CODE_FILE_EMPTY";
            break;
         }
      }

      RFSVP_M_LOG_LOW("Response to %s: %s", puc_cmd_str, puc_cmd_status_str);

      if ((eRFSVP_SERVER_CMD_READ == px_rsp->e_cmd)
         && (eNODE_MSG_CMD_RSP_CODE_SUCCESS == px_rsp->e_rsp_code))
      {
         puc_data_off = (uint8_t *) &(px_rsp->ui_data_len);
         puc_data_off += sizeof(px_rsp->ui_data_len);
         RFSVP_M_LOG_LOW("Read Response: \"%s\"", puc_data_off);
      }
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
