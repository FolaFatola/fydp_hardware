import numpy as np
import cv2
import glob
import matplotlib.pyplot as plt
import pickle

# Dimensions of the checkerboard
CHECKERBOARD = (6,3)

def undistort_image():
    # Stores all 3D and 2D points from all images
    object_points = []
    image_points = []

    # Get 3D object points in the real world
    object_points_3d = np.zeros((CHECKERBOARD[0] * CHECKERBOARD[1], 3), np.float32)
    object_points_3d[:, :2] = np.mgrid[0:CHECKERBOARD[0], 0:CHECKERBOARD[1]].T.reshape(-1, 2)

    # Get directory for all calibration images
    images = glob.glob('camera_cal/*.jpg')

    for i, fname in enumerate(images):
        image = cv2.imread(fname)
        gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

        ret, corners = cv2.findChessboardCorners(gray, CHECKERBOARD, None)

        if ret == True:
            object_points.append(object_points_3d)
            criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)
            corners2 = cv2.cornerSubPix(gray, corners, (11, 11), (-1, -1), criteria)
            image_points.append(corners2)

    # Test undistortion
    image_size = (image.shape[1], image.shape[0])
    # Calibrate camera
    ret, matrix, dist, r_vecs, t_vecs = cv2.calibrateCamera(object_points, image_points, image_size, None, None)
    new_camera_matrix, roi = cv2.getOptimalNewCameraMatrix(matrix, dist, image_size, 1, image_size)
    dst = cv2.undistort(image, matrix, dist, None, new_camera_matrix)
    # Save camera calibration
    distortion_coefficients = {}
    distortion_coefficients['mtx'] = matrix
    distortion_coefficients['dist'] = dist
    pickle.dump(distortion_coefficients, open('camera_cal/cal_pickle.p', 'wb'))

def undistort(img, cal_dir='camera_cal/cal_pickle.p'):
    with open(cal_dir, mode='rb') as f:
        file = pickle.load(f)
    matrix = file['mtx']
    dist = file['dist']
    dst = cv2.undistort(img, matrix, dist, None, matrix)
    
    return dst