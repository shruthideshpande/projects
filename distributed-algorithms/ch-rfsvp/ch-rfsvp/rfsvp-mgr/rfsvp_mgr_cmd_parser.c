/*******************************************************************************
 * Copyright (c) 2012, Sandeep Prakash <123sandy@gmail.com>
 * All rights reserved.
 *
 * \file   rfsvp_mgr_cmd_parser.c
 * \author sandeepprakash
 *
 * \date   26-Nov-2012
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
static RFSVP_RET_E rfsvp_mgr_cmd_parser_get_cmd_str (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   uint8_t *puc_cmd_buf,
   uint32_t ui_cmd_buf_len,
   uint8_t *puc_cmd_str,
   uint32_t ui_cmd_str_len);

static RFSVP_RET_E rfsvp_mgr_cmd_parser_get_cmd (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   uint8_t *puc_cmd_str,
   uint32_t ui_cmd_str_len,
   RFSVP_SERVER_CMD_E *pe_cmd);

static RFSVP_RET_E rfsvp_mgr_cmd_parser_interpret_cmd (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   RFSVP_SERVER_CMD_E e_cmd,
   uint8_t *puc_cmd_buf,
   uint32_t ui_cmd_buf_len);

/****************************** LOCAL FUNCTIONS *******************************/
RFSVP_RET_E rfsvp_mgr_cmd_parser_init (
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

   e_pal_ret = pal_fopen (&(px_rfsvp_mgr_ctxt->hl_cmd_file_hdl),
      px_rfsvp_mgr_ctxt->x_init_params.uca_cmd_filename_str,
      (const uint8_t *) "r");
   if ((ePAL_RET_SUCCESS != e_pal_ret)
      || (NULL == px_rfsvp_mgr_ctxt->hl_cmd_file_hdl))
   {
      RFSVP_M_LOG_LOW("pal_fopen failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_mgr_cmd_parser_get_next_command (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   uint8_t *puc_cmd_buf,
   uint32_t *pui_cmd_buf_len)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   PAL_RET_E e_pal_ret = ePAL_RET_FAILURE;

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == puc_cmd_buf)
      || (NULL == pui_cmd_buf_len) || (0 == *pui_cmd_buf_len))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   e_pal_ret = pal_freadline (px_rfsvp_mgr_ctxt->hl_cmd_file_hdl, puc_cmd_buf,
      *pui_cmd_buf_len, pui_cmd_buf_len);
   if (ePAL_RET_SUCCESS != e_pal_ret)
   {
      RFSVP_M_LOG_LOW("pal_freadline failed: %d", e_pal_ret);
      e_error = eRFSVP_RET_RESOURCE_FAILURE;
   }
   else
   {
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_mgr_cmd_parser_get_cmd_str (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   uint8_t *puc_cmd_buf,
   uint32_t ui_cmd_buf_len,
   uint8_t *puc_cmd_str,
   uint32_t ui_cmd_str_len)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   uint8_t *puc_cmd_ptr = 0;

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == puc_cmd_buf)
      || (0 == ui_cmd_buf_len) || (NULL == puc_cmd_str)
      || (0 == ui_cmd_str_len))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   puc_cmd_ptr = (uint8_t *) strtok((char *) puc_cmd_buf, " ");
   if (NULL == puc_cmd_ptr)
   {
      e_error = eRFSVP_RET_INVALID_ARGS;
   }
   else
   {
      (void) pal_strncpy(puc_cmd_str, puc_cmd_ptr, ui_cmd_str_len);
      e_error = eRFSVP_RET_SUCCESS;
   }
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_mgr_cmd_parser_get_cmd (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   uint8_t *puc_cmd_str,
   uint32_t ui_cmd_str_len,
   RFSVP_SERVER_CMD_E *pe_cmd)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   int32_t i_ret = -1;

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == puc_cmd_str)
      || (0 == ui_cmd_str_len) || (NULL == pe_cmd))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   *pe_cmd = eRFSVP_SERVER_CMD_INVALID;

   i_ret = pal_strncmp (puc_cmd_str, (uint8_t *) RFSVP_SERVER_CMD_STR_HALT,
      ui_cmd_str_len);
   if (0 == i_ret)
   {
      *pe_cmd = eRFSVP_SERVER_CMD_HALT;
      e_error = eRFSVP_RET_SUCCESS;
      goto CLEAN_RETURN;
   }

   i_ret = pal_strncmp (puc_cmd_str, (uint8_t *) RFSVP_SERVER_CMD_STR_WAIT,
      ui_cmd_str_len);
   if (0 == i_ret)
   {
      *pe_cmd = eRFSVP_SERVER_CMD_WAIT;
      e_error = eRFSVP_RET_SUCCESS;
      goto CLEAN_RETURN;
   }

   i_ret = pal_strncmp (puc_cmd_str, (uint8_t *) RFSVP_SERVER_CMD_STR_NODE_DOWN,
      ui_cmd_str_len);
   if (0 == i_ret)
   {
      *pe_cmd = eRFSVP_SERVER_CMD_NODE_DOWN;
      e_error = eRFSVP_RET_SUCCESS;
      goto CLEAN_RETURN;
   }

   i_ret = pal_strncmp (puc_cmd_str, (uint8_t *) RFSVP_SERVER_CMD_STR_NODE_UP,
      ui_cmd_str_len);
   if (0 == i_ret)
   {
      *pe_cmd = eRFSVP_SERVER_CMD_NODE_UP;
      e_error = eRFSVP_RET_SUCCESS;
      goto CLEAN_RETURN;
   }

   i_ret = pal_strncmp (puc_cmd_str, (uint8_t *) RFSVP_SERVER_CMD_STR_READ,
      ui_cmd_str_len);
   if (0 == i_ret)
   {
      *pe_cmd = eRFSVP_SERVER_CMD_READ;
      e_error = eRFSVP_RET_SUCCESS;
      goto CLEAN_RETURN;
   }

   i_ret = pal_strncmp (puc_cmd_str, (uint8_t *) RFSVP_SERVER_CMD_STR_WRITE,
      ui_cmd_str_len);
   if (0 == i_ret)
   {
      *pe_cmd = eRFSVP_SERVER_CMD_WRITE;
      e_error = eRFSVP_RET_SUCCESS;
   }
   else
   {
      e_error = eRFSVP_RET_INVALID_ARGS;
   }
CLEAN_RETURN:
   return e_error;
}

static RFSVP_RET_E rfsvp_mgr_cmd_parser_interpret_cmd (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   RFSVP_SERVER_CMD_E e_cmd,
   uint8_t *puc_cmd_buf,
   uint32_t ui_cmd_buf_len)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == puc_cmd_buf)
      || (0 == ui_cmd_buf_len))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }

   switch (e_cmd)
   {
      case eRFSVP_SERVER_CMD_HALT:
      {
         RFSVP_M_LOG_LOW("eRFSVP_SERVER_CMD_HALT");
         e_error = rfsvp_mgr_cmd_handle_cmd_halt (px_rfsvp_mgr_ctxt,
            puc_cmd_buf, ui_cmd_buf_len);
         break;
      }
      case eRFSVP_SERVER_CMD_WAIT:
      {
         RFSVP_M_LOG_LOW("eRFSVP_SERVER_CMD_WAIT");
         e_error = rfsvp_mgr_cmd_handle_cmd_wait (px_rfsvp_mgr_ctxt,
            puc_cmd_buf, ui_cmd_buf_len);
         break;
      }
      case eRFSVP_SERVER_CMD_NODE_DOWN:
      {
         RFSVP_M_LOG_LOW("eRFSVP_SERVER_CMD_NODE_DOWN");
         e_error = rfsvp_mgr_cmd_handle_cmd_node_down (px_rfsvp_mgr_ctxt,
            puc_cmd_buf, ui_cmd_buf_len);
         break;
      }
      case eRFSVP_SERVER_CMD_NODE_UP:
      {
         RFSVP_M_LOG_LOW("eRFSVP_SERVER_CMD_NODE_UP");
         e_error = rfsvp_mgr_cmd_handle_cmd_node_up (px_rfsvp_mgr_ctxt,
            puc_cmd_buf, ui_cmd_buf_len);
         break;
      }
      case eRFSVP_SERVER_CMD_READ:
      {
         RFSVP_M_LOG_LOW("eRFSVP_SERVER_CMD_READ");
         e_error = rfsvp_mgr_cmd_handle_cmd_read (px_rfsvp_mgr_ctxt,
            puc_cmd_buf, ui_cmd_buf_len);
         break;
      }
      case eRFSVP_SERVER_CMD_WRITE:
      {
         RFSVP_M_LOG_LOW("eRFSVP_SERVER_CMD_WRITE");
         e_error = rfsvp_mgr_cmd_handle_cmd_write (px_rfsvp_mgr_ctxt,
            puc_cmd_buf, ui_cmd_buf_len);
         break;
      }
      default:
      {
         e_error = eRFSVP_RET_INVALID_ARGS;
         break;
      }
   }
CLEAN_RETURN:
   return e_error;
}

#define RFSVP_MGR_CMD_STR_LEN       (10)

RFSVP_RET_E rfsvp_mgr_cmd_parser_interpret_cmd_line (
   RFSVP_MGR_CTXT_X *px_rfsvp_mgr_ctxt,
   uint8_t *puc_cmd_buf,
   uint32_t ui_cmd_buf_len)
{
   RFSVP_RET_E e_error = eRFSVP_RET_FAILURE;
   RFSVP_SERVER_CMD_E e_cmd = eRFSVP_SERVER_CMD_INVALID;
   uint8_t uca_cmd_str[RFSVP_MGR_CMD_STR_LEN] = {0};

   if ((NULL == px_rfsvp_mgr_ctxt) || (NULL == puc_cmd_buf)
      || (0 == ui_cmd_buf_len))
   {
      RFSVP_M_LOG_LOW("Invalid Args");
      e_error = eRFSVP_RET_INVALID_ARGS;
      goto CLEAN_RETURN;
   }
   RFSVP_M_LOG_LOW("Command: %s", puc_cmd_buf);

   e_error = rfsvp_mgr_cmd_parser_get_cmd_str (px_rfsvp_mgr_ctxt, puc_cmd_buf,
      ui_cmd_buf_len, uca_cmd_str, sizeof(uca_cmd_str));
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_cmd_parser_get_cmd_str failed: %d", e_error);
      goto CLEAN_RETURN;
   }

   e_error = rfsvp_mgr_cmd_parser_get_cmd (px_rfsvp_mgr_ctxt, uca_cmd_str,
      sizeof(uca_cmd_str), &e_cmd);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_cmd_parser_get_cmd failed: %d", e_error);
      goto CLEAN_RETURN;
   }

   e_error = rfsvp_mgr_cmd_parser_interpret_cmd (px_rfsvp_mgr_ctxt, e_cmd,
      puc_cmd_buf, ui_cmd_buf_len);
   if (eRFSVP_RET_SUCCESS != e_error)
   {
      RFSVP_M_LOG_LOW("rfsvp_mgr_cmd_parser_interpret_cmd failed: %d", e_error);
   }
CLEAN_RETURN:
   return e_error;
}

RFSVP_RET_E rfsvp_mgr_cmd_parser_deinit (
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

   if (NULL != px_rfsvp_mgr_ctxt->hl_cmd_file_hdl)
   {
      e_pal_ret = pal_fclose (px_rfsvp_mgr_ctxt->hl_cmd_file_hdl);
      if (ePAL_RET_SUCCESS != e_pal_ret)
      {
         RFSVP_M_LOG_LOW("pal_fclose failed: %d", e_pal_ret);
      }
      px_rfsvp_mgr_ctxt->hl_cmd_file_hdl = NULL;
   }
   e_error = eRFSVP_RET_SUCCESS;
CLEAN_RETURN:
   return e_error;
}
