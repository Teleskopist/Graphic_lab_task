import torch
import numpy as np

"""
Source: https://github.com/NVlabs/svraster/blob/main/src/sparse_voxel_gears/io.py
"""

def dequantize_state_dict(state_dict):
    state_dict['_geo_grid_pts'] = dequantization(state_dict['_geo_grid_pts'])
    state_dict['_sh0'] = torch.cat(
        [dequantization(v) for v in state_dict['_sh0']], dim=1)
    state_dict['_shs'] = torch.cat(
        [dequantization(v) for v in state_dict['_shs']], dim=1)

def dequantization(quant_dict):
    return quant_dict['codebook'][quant_dict['index'].long()]

class Model:
    @property
    def num_voxels(self):
        return len(self.octpath)
    
    @property
    def num_grid_pts(self):
        return len(self._geo_grid_pts)
    
    def load(self, path):
        '''
        Load the saved models.
        '''
        self.loaded_path = path
        state_dict = torch.load(path, map_location="cpu", weights_only=False)

        if state_dict.get('quantized', False):
            dequantize_state_dict(state_dict)

        self.active_sh_degree = state_dict['active_sh_degree']

        self.scene_center = state_dict['scene_center']
        self.inside_extent = state_dict['inside_extent']
        self.scene_extent = state_dict['scene_extent']

        self.octpath = state_dict['octpath']
        self.octlevel = state_dict['octlevel'].to(torch.int8)

        self._geo_grid_pts = state_dict['_geo_grid_pts']
        self._sh0 = state_dict['_sh0']
        self._shs = state_dict['_shs']
        
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

def convert(input_path, output_path) :
    voxel_model = Model()
    voxel_model.load(input_path)
    bw = BinaryWriter(output_path)
    bw.write(voxel_model.active_sh_degree.to_bytes(1, byteorder="little"))
    bw.write(voxel_model.scene_center)
    bw.write(voxel_model.inside_extent)
    bw.write(voxel_model.scene_extent)
    bw.write(voxel_model.num_voxels.to_bytes(4, "little"))
    bw.write(voxel_model.num_grid_pts.to_bytes(4, "little"))
    bw.write(voxel_model.octpath)
    bw.write(voxel_model.octlevel)
    bw.write(voxel_model._geo_grid_pts)
    bw.write(voxel_model._sh0)
    bw.write(voxel_model._shs)
    bw.close()

if __name__ == "__main__" :
    import argparse
    parser = argparse.ArgumentParser(description="Converter from Torch to Binary of Radience Fields")
    parser.add_argument("input_path")
    parser.add_argument("-o", "--output")
    args = parser.parse_args()

    input_path = args.input_path
    output_path = args.output

    if output_path is None :
        output_path = input_path[:input_path.rfind(".")] + ".bin"

    convert(input_path, output_path)