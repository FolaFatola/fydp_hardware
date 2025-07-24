import numpy as np
import cv2

import camera_cal as c

left_a, left_b, left_c = [],[],[]
right_a, right_b, right_c = [],[],[]
YM_PER_PIX = 30.5 / 720 # In the warped image, a lane line takes up approx. 30.5 m
XM_PER_PIX = 3.7 / 720 # 3.7m is the approx. width of a lane

# Input img is expected to be in RGB
def pipeline(img, s_thresh=(100, 255), sx_thresh=(15, 255), l_thresh=(120, 255)):
    img = c.undistort(img)
    img = np.copy(img)
    # Convert to HLS color space
    hls = cv2.cvtColor(img, cv2.COLOR_RGB2HLS).astype(np.float32)
    l_channel = hls[:,:,1] # useful for gradients
    s_channel = hls[:,:,2] # pick up white and yellow lane lines
    # h_channel = hls[:,:,0]

    # Apply Sobel filter on x (gradient)
    sobelx = cv2.Sobel(l_channel, cv2.CV_64F, 1, 0, ksize=3)
    abs_sobelx = np.absolute(sobelx) # Take the absolute value of the derivative and scale to 8-bit range (0-255)
    if np.max(abs_sobelx) > 0:
        scaled_sobel = np.uint8(255 * abs_sobelx / np.max(abs_sobelx))
    else:
        scaled_sobel = np.zeros_like(abs_sobelx, dtype=np.uint8)
    
    # Binarize
    sxbinary = np.zeros_like(scaled_sobel)
    sxbinary[(scaled_sobel >= sx_thresh[0]) & (scaled_sobel <= sx_thresh[1])] = 1
    
    # Threshold color channel
    s_binary = np.zeros_like(s_channel)
    s_binary[(s_channel >= s_thresh[0]) & (s_channel <= s_thresh[1])] = 1

    l_binary = np.zeros_like(l_channel)
    l_binary[(l_channel >= l_thresh[0]) & (l_channel <= l_thresh[1])] = 1

    # Color thresholding for white/yellow using RGB and HLS
    yellow_mask = ((hls[:, :, 0] >= 15) & (hls[:, :, 0] <= 35) & (s_channel >= 100) & (l_channel >= 100))
    white_mask = ((img[:, :, 0] >= 200) & (img[:, :, 1] >= 200) & (img[:, :, 2] >= 200))

    color_mask = np.zeros_like(sxbinary)
    color_mask[yellow_mask | white_mask] = 1

    # Combine all masks
    combined_binary = np.zeros_like(sxbinary)
    combined_binary[((s_binary == 1) & (l_binary == 1)) | (sxbinary == 1) | (color_mask == 1)] = 1
    return combined_binary

def perspective_warp(img):
    # Should be 1280 x 720
    img_width, img_height = img.shape[1], img.shape[0]
    # Trapezoid points around lane lines
    src = np.float32([
        [img_width * 0.43, img_height * 0.65],
        [img_width * 0.58, img_height * 0.65],
        [img_width * 0.95, img_height * 0.95],
        [img_width * 0.05, img_height * 0.95]
    ])
    dst = np.float32([
        [img_width * 0.2, 0],
        [img_width * 0.8, 0],
        [img_width * 0.8, img_height],
        [img_width * 0.2, img_height]
    ])

    # Given src and dst points, calculate the perspective transform matrix
    M = cv2.getPerspectiveTransform(src, dst)
    # Warp the image using OpenCV warpPerspective()
    warped = cv2.warpPerspective(img, M, (img_width, img_height), flags=cv2.INTER_LINEAR)
    return warped

def inverse_perspective_warp(img):
    # Should be 1280 x 720
    img_width, img_height = img.shape[1], img.shape[0]
    # Trapezoid points around lane lines
    dst = np.float32([
        [img_width * 0.43, img_height * 0.65],
        [img_width * 0.58, img_height * 0.65],
        [img_width * 0.95, img_height * 0.95],
        [img_width * 0.05, img_height * 0.95]
    ])
    src = np.float32([
        [img_width * 0.2, 0],
        [img_width * 0.8, 0],
        [img_width * 0.8, img_height],
        [img_width * 0.2, img_height]
    ])
    img_size = (int(img_width), int(img_height))
    # Given src and dst points, calculate the perspective transform matrix
    M = cv2.getPerspectiveTransform(src, dst)
    warped = cv2.warpPerspective(img, M, img_size)
    return warped

def get_histogram(img):
    hist = np.sum(img[img.shape[0] // 2:, :], axis=0)
    return hist

def sliding_window(img, nwindows=9, margin=100, minpix = 50, draw_windows=True):
    histogram = get_histogram(img)
    out_image = np.dstack((img, img, img))*255

    # Find peaks of the left and right halves of the image
    midpoint = int(histogram.shape[0] // 2)
    leftx_base = np.argmax(histogram[:midpoint])
    rightx_base = np.argmax(histogram[midpoint:]) + midpoint
    
    # Set window height
    window_height = int(img.shape[0] / nwindows)
    # Identify the x and y positions of all nonzero pixels in the image
    nonzero = img.nonzero()
    nonzeroy = np.array(nonzero[0])
    nonzerox = np.array(nonzero[1])
    # Current positions for each window
    leftx_current = leftx_base
    rightx_current = rightx_base
    
    # Create empty lists to receive left and right lane pixel indices
    left_lane_inds = []
    right_lane_inds = []

    for window in range(nwindows):
        # Identify window boundaries in x and y (and right and left)
        win_y_low = img.shape[0] - (window + 1) * window_height
        win_y_high = img.shape[0] - window * window_height
        win_xleft_low = leftx_current - margin
        win_xleft_high = leftx_current + margin
        win_xright_low = rightx_current - margin
        win_xright_high = rightx_current + margin
        
        # Draw windows
        if draw_windows == True:
            cv2.rectangle(out_image, (win_xleft_low, win_y_low), (win_xleft_high, win_y_high), (0, 255, 0), 4) 
            cv2.rectangle(out_image, (win_xright_low, win_y_low), (win_xright_high, win_y_high), (0, 255, 0), 4) 

        # Identify the nonzero pixels in x and y within the window
        good_left_inds = ((nonzeroy >= win_y_low) & (nonzeroy < win_y_high) & (nonzerox >= win_xleft_low) & (nonzerox < win_xleft_high)).nonzero()[0]
        good_right_inds = ((nonzeroy >= win_y_low) & (nonzeroy < win_y_high) & (nonzerox >= win_xright_low) & (nonzerox < win_xright_high)).nonzero()[0]

        left_lane_inds.append(good_left_inds)
        right_lane_inds.append(good_right_inds)

        # If you found more than minpix pixels, recentre next window on their mean position
        if len(good_left_inds) > minpix:
            leftx_current = int(np.mean(nonzerox[good_left_inds]))
        if len(good_right_inds) > minpix:        
            rightx_current = int(np.mean(nonzerox[good_right_inds]))

    # Concatenate the arrays of indices
    try:
        left_lane_inds = np.concatenate(left_lane_inds)
        right_lane_inds = np.concatenate(right_lane_inds)
    except ValueError:
        pass

    # Extract left and right line pixel positions
    leftx = nonzerox[left_lane_inds]
    lefty = nonzeroy[left_lane_inds] 
    rightx = nonzerox[right_lane_inds]
    righty = nonzeroy[right_lane_inds] 

    # Fit a second order polynomial to each
    left_fit = np.polyfit(lefty, leftx, 2)
    right_fit = np.polyfit(righty, rightx, 2)
    
    left_a.append(left_fit[0])
    left_b.append(left_fit[1])
    left_c.append(left_fit[2])
    
    right_a.append(right_fit[0])
    right_b.append(right_fit[1])
    right_c.append(right_fit[2])

    left_fit_= np.empty(3)
    right_fit_ = np.empty(3)
    
    left_fit_[0] = np.mean(left_a[-10:])
    left_fit_[1] = np.mean(left_b[-10:])
    left_fit_[2] = np.mean(left_c[-10:])
    
    right_fit_[0] = np.mean(right_a[-10:])
    right_fit_[1] = np.mean(right_b[-10:])
    right_fit_[2] = np.mean(right_c[-10:])
    
    # Generate x and y values for plotting
    ploty = np.linspace(0, img.shape[0] - 1, img.shape[0] )
    left_fitx = left_fit_[0] * ploty ** 2 + left_fit_[1] * ploty + left_fit_[2]
    right_fitx = right_fit_[0] * ploty ** 2 + right_fit_[1] * ploty + right_fit_[2]

    out_image[nonzeroy[left_lane_inds], nonzerox[left_lane_inds]] = [255, 0, 100]
    out_image[nonzeroy[right_lane_inds], nonzerox[right_lane_inds]] = [0, 100, 255]
    
    return out_image, (left_fitx, right_fitx), (left_fit_, right_fit_), ploty

def fit_curve(img, leftx, rightx):
    ploty = np.linspace(0, img.shape[0] - 1, img.shape[0])
    y_eval = np.max(ploty)

    # Fit new polynomials to x,y in world space
    left_fit_cr = np.polyfit(ploty * YM_PER_PIX, leftx * XM_PER_PIX, 2)
    right_fit_cr = np.polyfit(ploty * YM_PER_PIX, rightx * XM_PER_PIX, 2)
    # Calculate the new radii of curvature
    left_curverad = ((1 + (2 * left_fit_cr[0] * y_eval * YM_PER_PIX + left_fit_cr[1]) ** 2) ** 1.5) / np.absolute(2 * left_fit_cr[0])
    right_curverad = ((1 + (2 * right_fit_cr[0] * y_eval * YM_PER_PIX + right_fit_cr[1]) ** 2) ** 1.5) / np.absolute(2 * right_fit_cr[0])

    # We assume that the car is in the middle of the lane
    car_pos = img.shape[1] / 2
    l_fit_x_int = left_fit_cr[0] * img.shape[0] ** 2 + left_fit_cr[1] * img.shape[0] + left_fit_cr[2]
    r_fit_x_int = right_fit_cr[0] * img.shape[0] ** 2 + right_fit_cr[1] * img.shape[0] + right_fit_cr[2]
    lane_center_position = (r_fit_x_int + l_fit_x_int) / 2
    # Convert to meters
    center = (car_pos - lane_center_position) * XM_PER_PIX / 10

    return (left_curverad, right_curverad, center)

# Testing only
def draw_lane_area(img, left_fit, right_fit):
    ploty = np.linspace(0, img.shape[0]-1, img.shape[0])
    color_img = np.zeros_like(img)
    
    left = np.array([np.transpose(np.vstack([left_fit, ploty]))])
    right = np.array([np.flipud(np.transpose(np.vstack([right_fit, ploty])))])
    points = np.hstack((left, right))
    
    cv2.fillPoly(color_img, np.int_(points), (0,200,255))
    inv_perspective = inverse_perspective_warp(color_img)
    inv_perspective = cv2.addWeighted(img, 1, inv_perspective, 0.7, 0)
    return inv_perspective