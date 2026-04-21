import os
import subprocess

if __name__ == '__main__':
    print(__file__)
    dir = os.path.dirname(__file__)
    os.chdir(dir)
    shaders = os.listdir(dir)
    shaders = [i for i in shaders if i.endswith('.vert') or i.endswith('.frag')]

    for i in shaders:
        subprocess.run(['glslangValidator', '-V', i, '-o', f'{i}.spv'])

