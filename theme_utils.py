import platform
import ctypes

def apply_dark_titlebar(window):
    """
    Forces the native Windows titlebar to use Dark Mode, matching the Graphite dark theme.
    """
    if platform.system() != "Windows":
        return

    try:
        # Get the HWND of the Qt Window
        hwnd = int(window.winId())
        
        set_window_attribute = ctypes.windll.dwmapi.DwmSetWindowAttribute
        set_window_attribute.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_void_p, ctypes.c_uint]
        set_window_attribute.restype = ctypes.c_int
        
        # Windows 11 and recent Windows 10 versions (Build 18985 and newer) use 20
        DWMWA_USE_IMMERSIVE_DARK_MODE = 20
        # Older Windows 10 versions (Build 17763 to 18984) use 19
        DWMWA_USE_IMMERSIVE_DARK_MODE_OLD = 19
        
        rendering_policy = ctypes.c_int(1)
        
        # Try new flag first
        res = set_window_attribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, ctypes.byref(rendering_policy), ctypes.sizeof(rendering_policy))
        if res != 0:
            # Fallback to old flag
            set_window_attribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_OLD, ctypes.byref(rendering_policy), ctypes.sizeof(rendering_policy))
            
        # Force the OS to redraw the window frame to apply the change instantly
        user32 = ctypes.windll.user32
        user32.SetWindowPos.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_uint]
        SWP_NOMOVE = 0x0002
        SWP_NOSIZE = 0x0001
        SWP_NOZORDER = 0x0004
        SWP_FRAMECHANGED = 0x0020
        user32.SetWindowPos(hwnd, None, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED)
            
    except Exception as e:
        print(f"Warning: Could not apply dark titlebar theme: {e}")
