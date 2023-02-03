Import('RTT_ROOT')
Import('rtconfig')
# RT-Thread building script for component
from building import *

cwd     = GetCurrentDir()
src     = Glob('*.c')
#libs    = ['airkiss']
libpath = [cwd]
CPPPATH = [os.path.join(cwd)]
# 
group = DefineGroup('interactor', src, depend = [''], CPPPATH = CPPPATH,LIBPATH = libpath)
# objs = objs + group
Return('group')
