import numpy as np
from datetime import datetime

import lane_detection as ln
import ema_filter

LANE_DEPARTURE_THRESHOLD = 0.5  # meters (50 cm)
offset_history = []
offset_filter = ema_filter.EMAFilter(alpha=0.2)

def video_pipeline(img, fill_area=False):
    img_ = ln.pipeline(img)
    img_ = ln.perspective_warp(img_)
    out_img, curves, lanes, ploty = ln.sliding_window(img_, draw_windows=False)
    curverad = ln.fit_curve(img, curves[0], curves[1])
    lane_curve = np.mean([curverad[0], curverad[1]])
    if fill_area:
        img = ln.draw_lane_area(img, curves[0], curves[1])

    # Vehicle offset from center (in meters)
    offset = offset_filter.update(curverad[2])
    offset_history.append(offset)
    if len(offset_history) > 10:
        offset_history.pop(0)

    average_offset = np.mean(offset_history, dtype=np.float32)
    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    if average_offset < -LANE_DEPARTURE_THRESHOLD:
        direction = "left"
    elif average_offset > LANE_DEPARTURE_THRESHOLD:
        direction = "right"
    else:
        direction = "centre"
    
    return average_offset, direction, timestamp