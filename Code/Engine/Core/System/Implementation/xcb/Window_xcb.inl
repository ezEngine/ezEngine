#include <Core/System/Window.h>
#include <xcb/xcb.h>

static inline xcb_intern_atom_reply_t* intern_atom_helper(xcb_connection_t *conn, bool only_if_exists, const char *str)
{
	xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, only_if_exists, strlen(str), str);
	return xcb_intern_atom_reply(conn, cookie, NULL);
}

ezResult ezWindow::Initialize()
{
  EZ_LOG_BLOCK("ezWindow::Initialize", m_CreationDescription.m_Title.GetData());

  if (m_bInitialized)
  {
    Destroy().IgnoreResult();
  }
  
  EZ_ASSERT_RELEASE(m_CreationDescription.m_Resolution.HasNonZeroArea(), "The client area size can't be zero sized!");

  // xcb_connect always returns a non-NULL pointer to a xcb_connection_t,
  // even on failure. Callers need to use xcb_connection_has_error() to
  // check for failure. When finished, use xcb_disconnect() to close the
  // connection and free the structure.
  int scr = 0;
  m_WindowHandle.m_pConnection = xcb_connect(NULL, &scr);
  if( auto err = xcb_connection_has_error(m_WindowHandle.m_pConnection); err != 0 ) {
  	ezLog::Error("Could not connect to x11 via xcb. Error-Code '{}'", err);
	return EZ_FAILURE;
  }
  
  const xcb_setup_t *setup = xcb_get_setup(m_WindowHandle.m_pConnection);
  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
  // TODO respect ezWindowCreationDesc.m_iMonitor
  while (scr-- > 0)
  {
  	xcb_screen_next(&iter);
  }
  xcb_screen_t *pScreen = iter.data;
  
  m_WindowHandle.m_Window = xcb_generate_id(m_WindowHandle.m_pConnection);
  
  uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  
  uint32_t value_list[32];
  value_list[0] = pScreen->black_pixel;
  value_list[1] =
  	XCB_EVENT_MASK_KEY_RELEASE |
  	XCB_EVENT_MASK_KEY_PRESS |
  	XCB_EVENT_MASK_EXPOSURE |
  	XCB_EVENT_MASK_STRUCTURE_NOTIFY |
  	XCB_EVENT_MASK_POINTER_MOTION |
  	XCB_EVENT_MASK_BUTTON_PRESS |
  	XCB_EVENT_MASK_BUTTON_RELEASE |
	XCB_EVENT_MASK_FOCUS_CHANGE;
  
  if (m_CreationDescription.m_WindowMode == ezWindowMode::FullscreenBorderlessNativeResolution)
  {
  	m_CreationDescription.m_Resolution.width = pScreen->width_in_pixels;
  	m_CreationDescription.m_Resolution.height = pScreen->height_in_pixels;
  }
  
  xcb_create_window(m_WindowHandle.m_pConnection,
  	XCB_COPY_FROM_PARENT,
  	m_WindowHandle.m_Window, pScreen->root,
  	0, 0, m_CreationDescription.m_Resolution.width, m_CreationDescription.m_Resolution.height, 0,
  	XCB_WINDOW_CLASS_INPUT_OUTPUT,
  	pScreen->root_visual,
  	value_mask, value_list);
  
  /* Magic code that will send notification when window is destroyed */
  xcb_intern_atom_reply_t* reply = intern_atom_helper(m_WindowHandle.m_pConnection, true, "WM_PROTOCOLS");
  atom_wm_delete_window = intern_atom_helper(m_WindowHandle.m_pConnection, false, "WM_DELETE_WINDOW");
  
  xcb_change_property(m_WindowHandle.m_pConnection, XCB_PROP_MODE_REPLACE,
  	m_WindowHandle.m_Window, (*reply).atom, 4, 32, 1,
  	&(*atom_wm_delete_window).atom);
  
  xcb_change_property(m_WindowHandle.m_pConnection, XCB_PROP_MODE_REPLACE,
  	m_WindowHandle.m_Window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
  	m_CreationDescription.m_Title.GetElementCount(), m_CreationDescription.m_Title.GetData());
  
  free(reply);
  
  if (m_CreationDescription.m_WindowMode == ezWindowMode::FullscreenFixedResolution)
  {
  	xcb_intern_atom_reply_t *atom_wm_state = intern_atom_helper(m_WindowHandle.m_pConnection, false, "_NET_WM_STATE");
  	xcb_intern_atom_reply_t *atom_wm_fullscreen = intern_atom_helper(m_WindowHandle.m_pConnection, false, "_NET_WM_STATE_FULLSCREEN");
  	xcb_change_property(m_WindowHandle.m_pConnection,
  			XCB_PROP_MODE_REPLACE,
  			m_WindowHandle.m_Window, atom_wm_state->atom,
  			XCB_ATOM_ATOM, 32, 1,
  			&(atom_wm_fullscreen->atom));
  	free(atom_wm_fullscreen);
  	free(atom_wm_state);
  }
  
  xcb_map_window(m_WindowHandle.m_pConnection, m_WindowHandle.m_Window);
  xcb_flush (m_WindowHandle.m_pConnection);
  
  m_bInitialized = true;
  ezLog::Success("Created xcb window successfully. Resolution is {0}*{1}", GetClientAreaSize().width, GetClientAreaSize().height);
  
  return EZ_SUCCESS;
}

ezResult ezWindow::Destroy()
{
  if (!m_bInitialized)
    return EZ_SUCCESS;
	
  EZ_LOG_BLOCK("ezWindow::Destroy");
  
  if(m_WindowHandle.m_Window != 0)
  {
    xcb_destroy_window(m_WindowHandle.m_pConnection, m_WindowHandle.m_Window);
    m_WindowHandle.m_Window = 0;
  }
  
  if(m_WindowHandle.m_pConnection != nullptr)
  {
    xcb_disconnect(m_WindowHandle.m_pConnection);
	m_WindowHandle.m_pConnection = nullptr;
  }
  
  if(atom_wm_delete_window != nullptr)
  {
    free(atom_wm_delete_window);
	atom_wm_delete_window = nullptr;
  }
  
  m_bInitialized = false;
	
  return EZ_SUCCESS;
}

ezResult ezWindow::Resize(const ezSizeU32& newWindowSize)
{
  const uint32_t newWindowSizeValues[] = { newWindowSize.width, newWindowSize.height };
  xcb_void_cookie_t cookie = xcb_configure_window_checked(m_WindowHandle.m_pConnection, m_WindowHandle.m_Window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, newWindowSizeValues);
  
  if (xcb_generic_error_t* error = xcb_request_check(m_WindowHandle.m_pConnection, cookie))
  {
  	ezLog::Error("Could not resize window. Error-Code {}", error->error_code);
  	free(error);
  	return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezWindow::ProcessWindowMessages()
{
  if (!m_bInitialized)
    return;

  xcb_generic_event_t *event = nullptr;
  while((event = xcb_poll_for_event(m_WindowHandle.m_pConnection)))
  {
	switch (event->response_type & 0x7f)
	{
	case XCB_CLIENT_MESSAGE:
		if ((*(xcb_client_message_event_t*)event).data.data32[0] ==
			(*atom_wm_delete_window).atom) {
			ezLog::Info("Window delete message received");
			OnClickClose();
			continue;
		}
		break;
	case XCB_MOTION_NOTIFY:
	{
		// TODO 
		// xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;
		// handleMouseMove((int32_t)motion->event_x, (int32_t)motion->event_y);
		break;
	}
	break;
	case XCB_BUTTON_PRESS:
	{
		// TODO
		//
		// xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
		// if (press->detail == XCB_BUTTON_INDEX_1)
		// 	mouseButtons.left = true;
		// if (press->detail == XCB_BUTTON_INDEX_2)
		// 	mouseButtons.middle = true;
		// if (press->detail == XCB_BUTTON_INDEX_3)
		// 	mouseButtons.right = true;
	}
	break;
	case XCB_BUTTON_RELEASE:
	{
		// TODO
		// xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
		// if (press->detail == XCB_BUTTON_INDEX_1)
		// 	mouseButtons.left = false;
		// if (press->detail == XCB_BUTTON_INDEX_2)
		// 	mouseButtons.middle = false;
		// if (press->detail == XCB_BUTTON_INDEX_3)
		//	mouseButtons.right = false;
	}
	break;
	case XCB_KEY_PRESS:
	{
		// TODO
		/* const xcb_key_release_event_t *keyEvent = (const xcb_key_release_event_t *)event;
		switch (keyEvent->detail)
		{
			case KEY_W:

		}*/
	}
	break;
	case XCB_KEY_RELEASE:
	{
		// TODO
		/* const xcb_key_release_event_t *keyEvent = (const xcb_key_release_event_t *)event;
		switch (keyEvent->detail)
		{
			case KEY_W:
		}
		*/
	}
	break;
	case XCB_DESTROY_NOTIFY:
		ezLog::Info("Window destroy message received");
		Destroy().IgnoreResult();
		free(event);
		return;
	case XCB_FOCUS_IN:
		OnFocus(true);
		break;
	case XCB_FOCUS_OUT:
		OnFocus(false);
		break;
	case XCB_CONFIGURE_NOTIFY:
	{
		const xcb_configure_notify_event_t *cfgEvent = (const xcb_configure_notify_event_t *)event;
		if ((cfgEvent->width != m_CreationDescription.m_Resolution.width) || (cfgEvent->height != m_CreationDescription.m_Resolution.height))
		{
			if ((cfgEvent->width > 0) && (cfgEvent->height > 0))
			{
				OnResize(ezSizeU32(cfgEvent->width, cfgEvent->height));
			}
		}
		
		OnWindowMove(cfgEvent->x, cfgEvent->y);
	}
	break;
	default:
		break;
	}
	free(event);
  }
}

void ezWindow::OnResize(const ezSizeU32& newWindowSize)
{
  ezLog::Info("Window resized to ({0}, {1})", newWindowSize.width, newWindowSize.height);
}
