/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   rfsvp_mgr_cmd_interpret.c
 * \author sandeepprakash
 *
 * \date   02-Dec-2012
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
static RFSVP_RET_E rfsvp_mgr_get_dst_idx_for_cmd (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   uint8_t *puc_cmd_buf,
   uint32_t ui_cmd_buf_len,
   RFSVP_SERVER_CMD_E e_cmd,
   uint32_t *pui_index,
   uint32_t *pui_idx_num_chars);

/****************************** LOCAL FUNCTIONS *******************************/
static RFSVP_RET_E rfsvp_mgr_get_dst_idx_for_cmd (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   uint8_t *puc_cmd_buf,
   uint32_t ui_cmd_buf_len,
   RFSVP_SERVER_CMD_E e_cmd,
   uint32_t *pui_index,
   uint32_t *pui_idx_num_chars)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   uint8_t *puc_idx_ptr = NULL;
   uint32_t ui_cmd_str_len = 0;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == puc_cmd_buf)
      || (0 == ui_cmd_buf_len) || (NULL == pui_index) ||
      (NULL == pui_idx_num_chars))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   switch (e_cmd)
   {
      case eRFSVP_SERVER_CMD_HALT:
      {
         ui_cmd_str_len = pal_strnlen ((uint8_t *) RFSVP_SERVER_CMD_STR_HALT,
            sizeof(RFSVP_SERVER_CMD_STR_HALT));
         break;
      }
      case eRFSVP_SERVER_CMD_WAIT:
      {
         ui_cmd_str_len = pal_strnlen ((uint8_t *) RFSVP_SERVER_CMD_STR_WAIT,
            sizeof(RFSVP_SERVER_CMD_STR_WAIT));
         break;
      }
      case eRFSVP_SERVER_CMD_NODE_DOWN:
      {
         ui_cmd_str_len = pal_strnlen ((uint8_t *) RFSVP_SERVER_CMD_STR_NODE_DOWN,
            sizeof(RFSVP_SERVER_CMD_STR_NODE_DOWN));
         break;
      }
      case eRFSVP_SERVER_CMD_NODE_UP:
      {
         ui_cmd_str_len = pal_strnlen ((uint8_t *) RFSVP_SERVER_CMD_STR_NODE_UP,
            sizeof(RFSVP_SERVER_CMD_STR_NODE_UP));
         break;
      }
      case eRFSVP_SERVER_CMD_READ:
      {
         ui_cmd_str_len = pal_strnlen ((uint8_t *) RFSVP_SERVER_CMD_STR_READ,
            sizeof(RFSVP_SERVER_CMD_STR_READ));
         break;
      }
      case eRFSVP_SERVER_CMD_WRITE:
      {
         ui_cmd_str_len = pal_strnlen ((uint8_t *) RFSVP_SERVER_CMD_STR_WRITE,
            sizeof(RFSVP_SERVER_CMD_STR_WRITE));
         break;
      }
      default:
      {
         RFSVP_M_LOG_LOW("Invalid Cmd: %d", e_cmd);
         e_error = eRFSVP_RET_INVALID_ARGS;
         break;
      }
   }

   if (eRFSVP_RET_INVALID_ARGS == e_error)
   {
      goto CLEAN_RETURN;
   }

   puc_cmd_buf = puc_cmd_buf + ui_cmd_str_len; // For the cmd string.
   puc_cmd_buf = puc_cmd_buf + 1;              // For the space after cmd string
   puc_idx_ptr = (uint8_t *) strtok((char *) puc_cmd_buf, " ");
   if (NULL == puc_idx_ptr)
   {
      RFSVP_M_LOG_LOW("strtok failed: %p", puc_idx_ptr);
      e_error = eRFSVP_RET_INVALID_ARGS;
   }
   else
   {
      e_pal_ret = pal_atoi(puc_idx_ptr, (int32_t *) pui_index);
      if (ePAL_RET_SUCCESS != e_pal_ret)
      {
         RFSVP_M_LOG_LOW("pal_atoi failed: %d", e_pal_ret);
         e_error = eRFSVP_RET_RESOURCE_FAILURE;
      }
      else
      {
         if (*pui_index >= px_rfsvp_mgr_ctxt->x_init_params.ui_no_nodes)
         {
            RFSVP_M_LOG_LOW("Invalid Server Index: %d. Range: [%d-%d]",
               *pui_index, 0,
               (px_rfsvp_mgr_ctxt->x_init_params.ui_no_nodes - 1));
            e_error = eRFSVP_RET_INVALID_ARGS;
         }
         else
         {
            *pui_idx_num_chars = pal_strlen (puc_idx_ptr);
            e_error = eRFSVP_RET_SUCCESS;
         }
      }
   }
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_mgr_get_wait_duration (
   uint8_t *puc_cmd_buf,
   uint32_t ui_cmd_buf_len,
   RFSVP_SERVER_CMD_E e_cmd,
   int32_t *pi_wait_duration)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   uint8_t *puc_idx_ptr = NULL;
   uint32_t ui_cmd_str_len = 0;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;

   if ((NULL == puc_cmd_buf) || (0 == ui_cmd_buf_len)
      || (NULL == pi_wait_duration))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   ui_cmd_str_len = pal_strnlen ((uint8_t *) RFSVP_SERVER_CMD_STR_WAIT,
      sizeof(RFSVP_SERVER_CMD_STR_WAIT));
   puc_cmd_buf = puc_cmd_buf + ui_cmd_str_len; // For the cmd string.
   puc_cmd_buf = puc_cmd_buf + 1;              // For the space after cmd string
   puc_idx_ptr = (uint8_t *) strtok((char *) puc_cmd_buf, " ");
   if (NULL == puc_idx_ptr)
   {
      e_error = eRFSVP_RET_INVALID_ARGS;
   }
   else
   {
      e_pal_ret = pal_atoi(puc_idx_ptr, pi_wait_duration);
      if (ePAL_RET_SUCCESS != e_pal_ret)
      {
         e_error = eRFSVP_RET_RESOURCE_FAILURE;
      }
      else
      {
         e_error = eRFSVP_RET_SUCCESS;
      }
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_mgr_cmd_handle_cmd_halt (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   uint8_t *puc_cmd_buf,
   uint32_t ui_cmd_buf_len)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_APP_CMD_X x_cmd = {{0}};
   uint32_t ui_index = 0;
   uint32_t ui_idx_num_chars = 0;

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == puc_cmd_buf)
      || (0 == ui_cmd_buf_len))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_cmd.e_cmd = eRFSVP_SERVER_CMD_HALT;
   e_error = rfsvp_mgr_get_dst_idx_for_cmd (px_rfsvp_mgr_ctxt, puc_cmd_buf,
      ui_cmd_buf_len, x_cmd.e_cmd, &ui_index, &ui_idx_num_chars);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_get_dst_idx_for_cmd failed: %d", e_error);
      goto CLEAN_RETURN;
   }
   x_cmd.ui_server_idx = ui_index;
   x_cmd.x_hdr.ui_msg_id = eNODE_MSG_ID_APP_MSG_CMD;
   x_cmd.x_hdr.ui_msg_pay_len = sizeof(x_cmd) - sizeof(x_cmd.x_hdr);
   e_error = rfsvp_mgr_node_send_app_msg_cmd_to_node (px_rfsvp_mgr_ctxt,
      ui_index, &x_cmd);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_node_send_app_msg_cmd_to_node failed: %d",
         e_error);
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_mgr_cmd_handle_cmd_wait (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   uint8_t *puc_cmd_buf,
   uint32_t ui_cmd_buf_len)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   int32_t i_wait_duration;

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == puc_cmd_buf)
      || (0 == ui_cmd_buf_len))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }
   e_error = rfsvp_mgr_get_wait_duration (puc_cmd_buf, ui_cmd_buf_len,
      eRFSVP_SERVER_CMD_WAIT, &i_wait_duration);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_get_wait_duration failed: %d", e_error);
      goto CLEAN_RETURN;
   }

   if (i_wait_duration < -1)
   {
      RFSVP_M_LOG_LOW("Invalid wait duration: %d. Range: [%d-%d]",
         i_wait_duration, -1, LONG_MAX);
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   if (-1 == i_wait_duration)
   {
      RFSVP_M_LOG_LOW("Waiting for user action!!!");
      RFSVP_M_LOG_LOW("Please hit a key to continue...");
      (void) getchar ();
   }
   else
   {
      RFSVP_M_LOG_LOW("Waiting %d seconds", i_wait_duration);
      pal_sleep((uint32_t) (i_wait_duration * 1000));
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_mgr_cmd_handle_cmd_node_down (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   uint8_t *puc_cmd_buf,
   uint32_t ui_cmd_buf_len)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_APP_CMD_X x_cmd = {{0}};
   uint32_t ui_index = 0;
   uint32_t ui_idx_num_chars = 0;

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == puc_cmd_buf)
      || (0 == ui_cmd_buf_len))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_cmd.e_cmd = eRFSVP_SERVER_CMD_NODE_DOWN;
   e_error = rfsvp_mgr_get_dst_idx_for_cmd (px_rfsvp_mgr_ctxt, puc_cmd_buf,
      ui_cmd_buf_len, x_cmd.e_cmd, &ui_index, &ui_idx_num_chars);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_get_dst_idx_for_cmd failed: %d", e_error);
      goto CLEAN_RETURN;
   }
   x_cmd.ui_server_idx = ui_index;

   x_cmd.x_hdr.ui_msg_id = eNODE_MSG_ID_APP_MSG_CMD;
   x_cmd.x_hdr.ui_msg_pay_len = sizeof(x_cmd) - sizeof(x_cmd.x_hdr);

   e_error = rfsvp_mgr_node_send_app_msg_cmd_to_node (px_rfsvp_mgr_ctxt,
      ui_index, &x_cmd);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_node_send_app_msg_cmd_to_node failed: %d",
         e_error);
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_mgr_cmd_handle_cmd_node_up (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   uint8_t *puc_cmd_buf,
   uint32_t ui_cmd_buf_len)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_APP_CMD_X x_cmd = {{0}};
   uint32_t ui_index = 0;
   uint32_t ui_idx_num_chars = 0;

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == puc_cmd_buf)
      || (0 == ui_cmd_buf_len))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_cmd.e_cmd = eRFSVP_SERVER_CMD_NODE_UP;
   e_error = rfsvp_mgr_get_dst_idx_for_cmd (px_rfsvp_mgr_ctxt, puc_cmd_buf,
      ui_cmd_buf_len, x_cmd.e_cmd, &ui_index, &ui_idx_num_chars);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_get_dst_idx_for_cmd failed: %d", e_error);
      goto CLEAN_RETURN;
   }
   x_cmd.ui_server_idx = ui_index;

   x_cmd.x_hdr.ui_msg_id = eNODE_MSG_ID_APP_MSG_CMD;
   x_cmd.x_hdr.ui_msg_pay_len = sizeof(x_cmd) - sizeof(x_cmd.x_hdr);

   e_error = rfsvp_mgr_node_send_app_msg_cmd_to_node (px_rfsvp_mgr_ctxt,
      ui_index, &x_cmd);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_node_send_app_msg_cmd_to_node failed: %d",
         e_error);
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_mgr_cmd_handle_cmd_read (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   uint8_t *puc_cmd_buf,
   uint32_t ui_cmd_buf_len)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   NODE_MSG_APP_CMD_X x_cmd = {{0}};
   uint32_t ui_index = 0;
   uint32_t ui_idx_num_chars = 0;

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == puc_cmd_buf)
      || (0 == ui_cmd_buf_len))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   x_cmd.e_cmd = eRFSVP_SERVER_CMD_READ;
   e_error = rfsvp_mgr_get_dst_idx_for_cmd (px_rfsvp_mgr_ctxt, puc_cmd_buf,
      ui_cmd_buf_len, x_cmd.e_cmd, &ui_index, &ui_idx_num_chars);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_get_dst_idx_for_cmd failed: %d", e_error);
      goto CLEAN_RETURN;
   }
   x_cmd.ui_server_idx = ui_index;

   x_cmd.x_hdr.ui_msg_id = eNODE_MSG_ID_APP_MSG_CMD;
   x_cmd.x_hdr.ui_msg_pay_len = sizeof(x_cmd) - sizeof(x_cmd.x_hdr);

   e_error = rfsvp_mgr_node_send_app_msg_cmd_to_node (px_rfsvp_mgr_ctxt,
      ui_index, &x_cmd);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_node_send_app_msg_cmd_to_node failed: %d",
         e_error);
   }

CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_mgr_cmd_handle_cmd_write (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   uint8_t *puc_cmd_buf,
   uint32_t ui_cmd_buf_len)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   uint32_t ui_index = 0;
   uint32_t ui_alloc_size = 0;
   NODE_MSG_APP_CMD_X *px_cmd = NULL;
   uint8_t *puc_data_off = NULL;
   uint32_t ui_idx_num_chars = 0;

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == puc_cmd_buf)
      || (0 == ui_cmd_buf_len))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   ui_alloc_size = sizeof(NODE_MSG_APP_CMD_X) + ui_cmd_buf_len;
   px_cmd = pal_malloc (ui_alloc_size, NULL);
   if (NULL == px_cmd)
   {
      RFSVP_M_LOG_LOW("pal_malloc failed");
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
      goto CLEAN_RETURN;
   }

   px_cmd->e_cmd = eRFSVP_SERVER_CMD_WRITE;
   e_error = rfsvp_mgr_get_dst_idx_for_cmd (px_rfsvp_mgr_ctxt, puc_cmd_buf,
      ui_cmd_buf_len, px_cmd->e_cmd, &ui_index, &ui_idx_num_chars);
   if ((eRFSVP_RET_SUCCESS != e_error) || (0 == ui_idx_num_chars))
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_get_dst_idx_for_cmd failed: %d", e_error);
      goto CLEAN_RETURN;
   }
   px_cmd->ui_server_idx = ui_index;

   px_cmd->ui_data_len = ui_cmd_buf_len;
   puc_data_off = (uint8_t *) &(px_cmd->ui_data_len);
   puc_data_off += sizeof(px_cmd->ui_data_len);

   puc_cmd_buf += pal_strlen ((const uint8_t *) RFSVP_SERVER_CMD_STR_WRITE);
   ui_cmd_buf_len -= pal_strlen ((const uint8_t *) RFSVP_SERVER_CMD_STR_WRITE);
   puc_cmd_buf += 1; // For the space after the command string.
   ui_cmd_buf_len -= 1;
   puc_cmd_buf += ui_idx_num_chars;
   ui_cmd_buf_len -= ui_idx_num_chars;
   puc_cmd_buf += 1; // For the space after the index string.
   ui_cmd_buf_len -= 1;

   (void) pal_memmove(puc_data_off, puc_cmd_buf, ui_cmd_buf_len);

   px_cmd->x_hdr.ui_msg_id = eNODE_MSG_ID_APP_MSG_CMD;
   px_cmd->x_hdr.ui_msg_pay_len = ui_alloc_size - sizeof(px_cmd->x_hdr);

   e_error = rfsvp_mgr_node_send_app_msg_cmd_to_node (px_rfsvp_mgr_ctxt,
      ui_index, px_cmd);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_node_send_app_msg_cmd_to_node failed: %d",
         e_error);
   }
CLEAN_RETURN:
   return e_error;
}
