# Copyright (c) 2025, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.

"""
Source: https://github.com/NVlabs/svraster/blob/main/src/dataloader/data_pack.py
        https://github.com/NVlabs/svraster/blob/main/src/cameras.py
        https://github.com/NVlabs/svraster/blob/main/src/dataloader/reader_colmap_dataset.py
        https://github.com/NVlabs/svraster/blob/main/src/utils/camera_utils.py
"""

import os
import time
import numpy as np

import torch
import natsort
import pycolmap
from PIL import Image
from pathlib import Path
import concurrent.futures

def focal2fov(focal, pixels):
    return 2 * np.arctan(pixels / (2 * focal))

def read_colmap_dataset(source_path, image_dir_name, camera_creator):

    source_path = Path(source_path)

    # Parse colmap meta data
    sparse_path = source_path / "sparse" / "0"
    if not sparse_path.exists():
        sparse_path = source_path / "colmap" / "sparse" / "0"
    if not sparse_path.exists():
        raise Exception("Can not find COLMAP reconstruction.")

    sfm = pycolmap.Reconstruction(sparse_path)

    # Sort key by filename
    keys = natsort.natsorted(
        sfm.images.keys(),
        key = lambda k : sfm.images[k].name)

    # Load all images and cameras
    todo_lst = []
    for key in keys:

        frame = sfm.images[key]

        # Load image
        image_path = source_path / image_dir_name / frame.name
        if not image_path.exists():
            image_path = image_path.with_suffix('.png')
        if not image_path.exists():
            image_path = image_path.with_suffix('.jpg')
        if not image_path.exists():
            image_path = image_path.with_suffix('.JPG')
        if not image_path.exists():
            raise Exception(f"File not found: {str(image_path)}")
        image = Image.open(image_path)

        # Load camera intrinsic
        if frame.camera.model.name == "SIMPLE_PINHOLE":
            focal_x, cx, cy = frame.camera.params
            fovx = focal2fov(focal_x, frame.camera.width)
            fovy = focal2fov(focal_x, frame.camera.height)
            cx_p = cx / frame.camera.width
            cy_p = cy / frame.camera.height
        elif frame.camera.model.name == "PINHOLE":
            focal_x, focal_y, cx, cy = frame.camera.params
            fovx = focal2fov(focal_x, frame.camera.width)
            fovy = focal2fov(focal_y, frame.camera.height)
            cx_p = cx / frame.camera.width
            cy_p = cy / frame.camera.height
        else:
            assert False, "Colmap camera model not handled: only undistorted datasets (PINHOLE or SIMPLE_PINHOLE cameras) supported!"

        # Load camera extrinsic
        w2c = np.eye(4, dtype=np.float32)
        try:
            w2c[:3] = frame.cam_from_world().matrix()
        except:
            # Older version of pycolmap
            w2c[:3] = frame.cam_from_world.matrix()

        todo_lst.append(dict(
            image=image,
            w2c=w2c,
            fovx=fovx,
            fovy=fovy,
            cx_p=cx_p,
            cy_p=cy_p,
            image_name=image_path.name,
        ))

    # Load all cameras concurrently
    import torch
    torch.inverse(torch.eye(3, device="cuda"))  # Fix module lazy loading bug:
                                                # https://github.com/pytorch/pytorch/issues/90613

    with concurrent.futures.ThreadPoolExecutor() as executor:
        futures = [executor.submit(camera_creator, **todo) for todo in todo_lst]
        cam_lst = [f.result() for f in futures]

    return cam_lst

class MiniCam:
    def __init__(self,
            c2w, fovx, fovy,
            width, height,
            cx_p=None, cy_p=None,
            image_name="minicam"):

        self.image_name = image_name
        self.c2w = torch.tensor(c2w).clone()
        self.w2c = self.c2w.inverse()

        self.fovx = fovx
        self.fovy = fovy
        self.image_width = width
        self.image_height = height
        self.cx_p = (0.5 if cx_p is None else cx_p)
        self.cy_p = (0.5 if cy_p is None else cy_p)

    def __repr__(self):
        clsname = self.__class__.__name__
        fname = f"image_name='{self.image_name}'"
        res = f"HW=({self.image_height}x{self.image_width})"
        fov = f"fovx={np.rad2deg(self.fovx):.1f}deg"
        return f"{clsname}({fname}, {res}, {fov})"

class DataPack:

    def __init__(self,
                 source_path,
                 image_dir_name="images"):

        camera_creator = CameraCreator()

        sparse_path = os.path.join(source_path, "sparse")
        colmap_path = os.path.join(source_path, "colmap", "sparse")
        meta_path1 = os.path.join(source_path, "transforms_train.json")
        meta_path2 = os.path.join(source_path, "transforms.json")

        # Read images concurrently
        s_time = time.perf_counter()

        if os.path.exists(sparse_path) or os.path.exists(colmap_path):
            print("Read dataset in COLMAP format.")
            dataset = read_colmap_dataset(
                source_path=source_path,
                image_dir_name=image_dir_name,
                camera_creator=camera_creator)
        elif os.path.exists(meta_path1) or os.path.exists(meta_path2):
            print("Read dataset in NeRF format.")
            raise Exception("NeRF dataset is not supported!")
        else:
            raise Exception("Unknown scene type!")

        e_time = time.perf_counter()
        print(f"Read dataset in {e_time - s_time:.3f} seconds.")

        self._cameras = dataset

        self.source_path = source_path

        # ##############################
        # # Read additional dataset info
        # ##############################
        # # If the dataset suggested a scene bound
        # self.suggested_bounding = dataset.get('suggested_bounding', None)

        # # If the dataset provide a transformation to other coordinate
        # self.to_world_matrix = None
        # to_world_path = os.path.join(source_path, 'to_world_matrix.txt')
        # if os.path.isfile(to_world_path):
        #     self.to_world_matrix = np.loadtxt(to_world_path)

        # # If the dataset has a point cloud
        # self.point_cloud = dataset.get('point_cloud', None)

    def get_cameras(self):
        return self._cameras

# Function that create Camera instances while parsing dataset
class CameraCreator:
    def __call__(self,
                 image,
                 w2c,
                 fovx,
                 fovy,
                 cx_p=0.5,
                 cy_p=0.5,
                 image_name=""):

        # Load camera parameters only
        return MiniCam(
            c2w=np.linalg.inv(w2c),
            fovx=fovx, fovy=fovy,
            cx_p=cx_p, cy_p=cy_p,
            width=image.size[0],
            height=image.size[1],
            image_name=image_name)
        
class BinaryWriter:
    def __init__(self, path) :
        self.fd = open(path, "wb")

    def close(self) :
        self.fd.close()

    def write(self, data) :
        if isinstance(data, np.ndarray) :
            return self.write(data.tobytes())
        elif isinstance(data, torch.Tensor) :
            return self.write(data.numpy())
        else :
            return self.fd.write(data)

def export_colmap_cameras(input_path, output_path) :
    dp = DataPack(input_path)
    cameras = dp.get_cameras()
    bw = BinaryWriter(output_path)
    bw.write(len(cameras).to_bytes(4, "little"))
    for cam in cameras :
        bw.write(cam.c2w)
        bw.write(np.array([cam.fovx, cam.fovy], dtype=np.float32))
        bw.write(np.array([cam.image_width, cam.image_height], dtype=np.int32))
        bw.write(np.array([cam.cx_p, cam.cy_p], dtype=np.float32))
        utf_str = cam.image_name.encode("utf-8")
        bw.write(len(utf_str).to_bytes(4, "little"))
        bw.write(utf_str)
    bw.close()

if __name__ == "__main__" :
    import argparse
    parser = argparse.ArgumentParser(description="Colmap Dataset exporter")
    parser.add_argument("input_path")
    parser.add_argument("-o", "--output")
    args = parser.parse_args()

    input_path = args.input_path
    output_path = args.output

    if output_path is None :
        output_path = input_path + ".bin"

    export_colmap_cameras(input_path, output_path)

    """
    Binary file structure:
    CAMERAS_COUNT : uint32, le
    for i in range(CAMERAS_COUNT) :
        C2W_MATRIX[i] : float32[4][4]
        FOVX[i] : float32
        FOVY[i] : float32
        IMAGE_WIDTH[i] : uint32, le
        IMAGE_HEIGHT[i] : uint32, le
        CX_P[i] : float32
        CY_P[i] : float32
        STR_LEN[i] : uint32, le
        STR[i] : string[STR_LEN[i]], utf8
    """