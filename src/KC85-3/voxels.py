import ImportVox as impvox

def generate(input, out_src, out_hdr) :

    v = impvox.VoxImporter(input, out_src, out_hdr)
    v.namespace = 'Emu'
    v.vox_file  = 'test.vox'
    v.generate()

