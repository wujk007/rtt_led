from building import *
Import('rtconfig')

src   = []
cwd   = GetCurrentDir()

# add led src files.
if GetDepend('PKG_USING_LED'):
    src += Glob('src/led.c')

if GetDepend('PKG_USING_LED_SAMPLE'):
    src += Glob('examples/led_sample.c')

# add led include path.
path  = [cwd + '/inc']

# add src and include to group.
group = DefineGroup('led', src, depend = ['PKG_USING_LED'], CPPPATH = path)

Return('group')
