#-------------------------------------------------------------------------------
# Import MagicaVoxel VOX files into a more compact run-length-encoded
# binary format.
#
# import genutil as util
from collections import OrderedDict
import struct 

Version = 1

#-------------------------------------------------------------------------------
class VoxImporter :
    def __init__(self, input, out_src, out_hdr) :
        self.namespace = 'Oryol'
        self.input = input
        self.out_src = out_src
        self.out_hdr = out_hdr
        self.voxFile = None
        self.size = [0, 0, 0]   # width, depth, height
        self.voxels = None
        self.palette = None

    def set_voxel(self, x, y, z, c) :
        offset = z + x*self.size[2] + y*self.size[2]*self.size[0]
        self.voxels[offset] = c

    def get_voxel(self, x, y, z) :
        offset = z + x*self.size[2] + y*self.size[2]*self.size[0]
        return self.voxels[offset]

    def load_chunk(self, f) :
        chunk_id = f.read(4)
        if len(chunk_id) != 4:
            return False
        print('Loading chunk {}...'.format(chunk_id))
        chunk_size = struct.unpack('<I', f.read(4))[0]
        children_size = struct.unpack('<I', f.read(4))[0]
        if chunk_id == 'MAIN' :
            while self.load_chunk(f) :
                pass
        elif chunk_id == 'SIZE' :
            self.size = struct.unpack('<III', f.read(4*3))
            print('  size: {}'.format(self.size))
        elif chunk_id == 'XYZI' :
            num = struct.unpack('<I', f.read(4))[0]
            print('  num voxels: {}'.format(num))
            self.voxels = [0] * self.size[0]*self.size[1]*self.size[2]
            data = f.read(num * 4)
            for i in range(0, num) :
                off = i*4
                x = ord(data[off+0])
                y = ord(data[off+1])
                z = ord(data[off+2])
                c = ord(data[off+3])
                self.set_voxel(x,y,z,c)
        elif chunk_id == 'RGBA' :
            self.palette = []
            data = f.read(256 * 4)
            for i in range(0, 256) :
                off = i*4
                r = ord(data[off+0])
                g = ord(data[off+1])
                b = ord(data[off+2])
                a = ord(data[off+3])
                self.palette.append((r,g,b,a))
        else :
            skip = f.read(chunk_size)
        return True
        
    def load(self) :
        with open(self.voxFile, 'rb') as f :
            magic = f.read(4)
            if magic != 'VOX ':
                print('{} is not a VOX file!'.format(self.voxFile))
            version = struct.unpack('<I', f.read(4))[0]
            print("VOX file '{}' version={}".format(self.voxFile, version))
            self.load_chunk(f)

    def reduce_palette(self) :
        pal = OrderedDict()
        pal[0] = (0,0,0,0)
        for x in range(0, self.size[0]) :
            for y in range(0, self.size[1]) :
                for z in range(0, self.size[2]) :
                    c = self.get_voxel(x, y, z)
                    if c != 0 :
                        if c not in pal :
                            pal[c] = self.palette[c]
                        self.set_voxel(x, y, z, pal.keys().index(c))
        self.palette = pal
        print('Palette reduced to {} entries'.format(len(self.palette)))

    def runlength_encode(self) :
        self.rle = []
        num = 0;
        col = self.voxels[0]
        for c in self.voxels :
            if (c != col) or (num == 255) :
                self.rle.append(num)
                self.rle.append(col)
                col = c
                num = 0
            num += 1
        print('RLE encoding reduced to {} bytes'.format(len(self.rle)))

    def write_header(self, f) :
        f.write('#pragma once\n')
        f.write('// #version:{}#\n'.format(Version))
        f.write('// machine generated, do not edit!\n')
        f.write('#include "Core/Types.h"\n')
        f.write('namespace {} {{\n'.format(self.namespace))
        f.write('struct Vox {\n')
        f.write('    static const int X = {};\n'.format(self.size[0]))
        f.write('    static const int Y = {};\n'.format(self.size[1]))
        f.write('    static const int Z = {};\n'.format(self.size[2]))
        f.write('    static const uint8_t Palette[{}][4];\n'.format(len(self.palette)))
        f.write('    static const uint8_t VoxelsRLE[{}];\n'.format(len(self.rle)))
        f.write('} VoxelData;\n')
        f.write('}\n')

    def write_source(self, f, absSourcePath) :
        path, hdrFileAndExt = os.path.split(absSourcePath)
        hdrFile, ext = os.path.splitext(hdrFileAndExt)
        f.write('// #version:{}# machine generated, do not edit!\n'.format(Version))
        f.write('#include "Pre.h"\n')
        f.write('#include "{}.h"\n'.format(hdrFile))
        f.write('namespace {} {{\n'.format(self.namespace))
        f.write('const uint8_t Vox::Palette[{}][4] = {{\n'.format(len(self.palette)))
        for i,c in enumerate(self.palette) :
            f.write('  {{ {}, {}, {}, {} }},\n'.format(c[0], c[1], c[2], c[3])
        f.write('}\n')

    def generate(self) :
        if util.isDirty(Version, [self.input, self.voxFile], [self.out_src, self.out_hdr]) :
            self.load()
            self.reduce_palette()
            self.runlength_encode(self)

#-------------------------------------------------------------------------------
if __name__ == '__main__' :
    vox = VoxImporter('','','')
    vox.voxFile = '/Users/floh/projects/voxel-data/junost.vox'
    vox.load()
    vox.reduce_palette()
    vox.runlength_encode()
